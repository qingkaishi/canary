

#include "MemoryLeak/VFGReachable.h"
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckVFG.h"
#include "MemoryLeak/MLDReport.h"
#include "MemoryLeak/MLDVFG.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <tuple>
#include <vector>
#include <z3++.h>

static int BranchCounter = 1;

static int BBIDCounter = 0;
static std::map<BasicBlock *, int> BBID;

void VFGReachable::addBBID(BasicBlock *bb) {
    if (!BBID.count(bb)) {
        BBID[bb] = BBIDCounter++;
    }
}

VFGReachable::VFGReachable(llvm::Module &M, MLDVFG *MVFG, DyckAliasAnalysis *DAA)
    : MVFG(MVFG),
      DAA(DAA),
      GlobalSolver(GlobalContext) {
    // init branch condition for each terminator instruction of basic blocks respectively.
    for (auto &F : M) {
        auto CtrlFlowGraph = std::make_shared<CFG>(&F);
        CFGMap.insert({&F, CtrlFlowGraph});
        for (auto &BB : F.getBasicBlockList()) {
            // BBID[&BB] = BBIDCounter++;
            addBBID(&BB);
            Instruction *TerminatorInst = BB.getTerminator();
            if (TerminatorInst->getNumSuccessors() == 1) {
                BranchBBCond[&BB].push_back(GlobalContext.bool_val(true));
                continue;
            }
            if (auto BrInst = dyn_cast<BranchInst>(TerminatorInst)) {
                expr BrVar = GlobalContext.bool_const(("B" + std::to_string(BranchCounter++)).c_str());
                std::string inst;
                raw_string_ostream s(inst);
                s << *BrInst;
                std::cout << BranchCounter - 1 << "   " << inst << std::endl;
                BranchBBCond[&BB].push_back(BrVar);
                BranchBBCond[&BB].push_back(!BrVar);
            }
            else if (auto SwInst = dyn_cast<SwitchInst>(TerminatorInst)) {
                expr SwVar = GlobalContext.int_const(("I" + std::to_string(BranchCounter++)).c_str());
                expr Default = GlobalContext.bool_val(true);
                std::vector<expr> ev;
                for (int i = 1; i < SwInst->getNumSuccessors(); i++) {
                    auto CmpValue = SwInst->getOperand(2 * i);
                    auto IntValue = (int)*(dyn_cast<ConstantInt>(CmpValue)->getValue().getRawData());
                    expr IntConstExpr = GlobalContext.int_val(IntValue);
                    ev.push_back(SwVar == IntConstExpr);
                    Default = Default && SwVar != IntConstExpr;
                }
                ev.insert(ev.begin(), Default);
                BranchBBCond[&BB].insert(BranchBBCond[&BB].end(), ev.begin(), ev.end());
            }
            else {
                expr BrVar = GlobalContext.int_const(("I" + std::to_string(BranchCounter++)).c_str());
                for (int i = 0; i < TerminatorInst->getNumSuccessors(); i++) {
                    expr IntVal = GlobalContext.int_val(i);
                    BranchBBCond[&BB].push_back(BrVar == i);
                }
            }
        }
        FuncDomTree[&F] = PostDominatorTree(F);
    }
}

void VFGReachable::solveAllSlices() {
    std::vector<MLDReport> result;
    for (auto SliceIt = MVFG->begin(); SliceIt != MVFG->end(); SliceIt++) {
        CurrSlice = &SliceIt->second;
        result.push_back(solveReachable());
    }
    for (auto Report : result) {
        outs() << Report.toString() << "\n";
    }
}

const Call *VFGReachable::getCallSite(int id) {
    return this->DAA->getDyckCallGraph()->getCallSite(abs(id));
}

MLDReport VFGReachable::solveReachable() {
    if (CurrSlice->sinks_begin() == CurrSlice->sinks_end()) {
        return MLDReport(CurrSlice->getSource()->getValue(), MLDReport::NeverFree);
    }
    std::queue<DyckVFGNode *> WorkList;
    WorkList.push(CurrSlice->getSource());
    std::map<DyckVFGNode *, expr> VFGNodeCond;
    VFGNodeCond.insert({CurrSlice->getSource(), getTrueCond()});
    while (!WorkList.empty()) {
        DyckVFGNode *Curr = WorkList.front();
        WorkList.pop();
        BasicBlock *CurrBB = getBasciBlock(Curr->getValue());

        assert(CurrBB && "A value which contained by any BB comes out.\n");
        expr ParentCond = VFGNodeCond.at(Curr);
        for (auto Edge = Curr->begin(); Edge != Curr->end(); Edge++) {
            BBCond.clear();
            if (!CurrSlice->contains(Edge->first))
                continue;
            expr Cond = getTrueCond();
            // outs() << *Curr->getValue() << "    " << *Edge->first->getValue() << "\n";
            if (Edge->second == 0) {
                BasicBlock *SuccBB = getBasciBlock(Edge->first->getValue());
                assert(SuccBB && "A value which contained by any BB comes out.\n");
                Cond = computerIntraCond(CurrBB, SuccBB);
            }
            else if (Edge->second > 0) {
                BasicBlock *SuccBB = getBasciBlock(Edge->first->getValue());
                assert(SuccBB && "A value which contained by any BB comes out.\n");
                Cond = computerCallCond(CurrBB, SuccBB, getCallSite(Edge->second));
            }
            else if (Edge->second < 0) {
                BasicBlock *SuccBB = getBasciBlock(Edge->first->getValue());
                assert(SuccBB && "A value which contained by any BB comes out.\n");
                Cond = computerRetCond(CurrBB, SuccBB, getCallSite(Edge->second));
            }
            // std::string v;
            // raw_string_ostream s(v);
            // s << *Edge->first->getValue();
            // std::cout << "\033[34m Cond :" << Cond << "  " << v << "\033[0m" << std::endl;
            Cond = Cond && ParentCond;
            if (VFGNodeCond.count(Edge->first)) {
                expr UpdateCond = VFGNodeCond.at(Edge->first) || Cond;

                z3::solver Solver(GlobalContext);
                Solver.add(UpdateCond == Cond);
                if (Solver.check() == z3::unsat) {
                    VFGNodeCond.erase(Edge->first);
                    VFGNodeCond.insert({Edge->first, UpdateCond});
                    // std::cout << "update" << v;
                    WorkList.push(Edge->first);
                }
            }
            else {
                VFGNodeCond.insert({Edge->first, Cond});
                WorkList.push(Edge->first);
            }
        }
    }

    expr FinalGoal = getFalseCond();
    for (auto Sink = CurrSlice->sinks_begin(); Sink != CurrSlice->sinks_end(); Sink++) {
        FinalGoal = FinalGoal || VFGNodeCond.at(*Sink);
    }

    z3::solver FinalSolver(GlobalContext);
    std::cout << "\033[35mFinalGoal" << FinalGoal << "\033[0m" << std::endl;
    FinalSolver.add(FinalGoal == getFalseCond());
    z3::check_result Result = FinalSolver.check();
    MLDReport::ReportType Report = MLDReport::NeverFree;
    if (Result == z3::sat) {
        Report = MLDReport::PartialFree;
    }
    else if (Result == z3::unsat) {
        Report = MLDReport::SafeFree;
    }
    return MLDReport(CurrSlice->getSource()->getValue(), Report);
}

void VFGReachable::computerIntraCond(BasicBlock *src, BasicBlock *dst, std::vector<int> visited,
                                     std::vector<std::pair<BasicBlock *, int>> pathCondition) {
    int srcID = BBID[src];
    int dstID = BBID[dst];
    // outs() << src->getName() << "\n";
    // std::cout << "caonima" << std::endl;
    visited[srcID] += 1;
    if (src == dst) {
        expr FinalCond = getTrueCond();
        for (auto BBIdx : pathCondition) {
            FinalCond = FinalCond && BranchBBCond[BBIdx.first][BBIdx.second];
        }
        if (BBCond.find(dst) != BBCond.end()) {
            // std::cout << "\033[35mFinalCond: " << (FinalCond || BBCond.at(dst)) << "\033[0m" << std::endl;
            FinalCond = FinalCond || BBCond.at(dst );
            BBCond.erase(dst);
            BBCond.insert(std::make_pair(dst, FinalCond));
        }
        else
            BBCond.insert({dst, FinalCond});
        // std::cout << "\033[36mdst: " << BBCond.at(dst) << "\033[0m" << std::endl;
    }
    else if (visited[srcID] == 1) {
        int idx = 0;
        for (auto Succ : successors(src)) {
            int SuccID = BBID[Succ];
            if (visited[SuccID] <= 1 && reachable(Succ, dst)) {
                pathCondition.push_back({src, idx});
                // std::cout << BranchBBCond[src][idx] << std::endl;
                computerIntraCond(Succ, dst, visited, pathCondition);
                pathCondition.pop_back();
            }
            idx++;
        }
    }
    else if (visited[srcID] == 2) {
        if (succ_size(src) == 2) {
            for (auto Succ : successors(src)) {
                int SuccID = BBID[Succ];
                if (visited[SuccID] == 0 && reachable(Succ, dst)) {
                    computerIntraCond(Succ, dst, visited, pathCondition);
                }
            }
        }
    }
    visited[srcID] -= 1;
    return;
}

expr VFGReachable::computerIntraCond(BasicBlock *src, BasicBlock *dst) {
    // outs() << src->getParent() << " " << dst->getParent() << "\n";
    assert(src->getParent() == dst->getParent() && "Two Basic Blocks are not in the same function");
    BBCond.clear();
    if (FuncDomTree[src->getParent()].dominates(dst, src)) {
        return getTrueCond();
    }
    if (!reachable(src, dst)) {
        return getFalseCond();
    }
    BBCond.insert({src, getTrueCond()});
    std::vector<int> visited(BBID.size(), 0);
    std::vector<std::pair<BasicBlock *, int>> pathCond;
    computerIntraCond(src, dst, visited, pathCond);
    return BBCond.at(dst);
}

expr VFGReachable::computerCallCond(BasicBlock *src, BasicBlock *dst, const Call *CallSite) {
    BasicBlock *mid = CallSite->getInstruction()->getParent();
    assert(mid && "CallSite is also a Instruction and owned by a Basic Block!\n");

    expr src_to_mid = getTrueCond();
    if (src->getParent() == mid->getParent()) {
        src_to_mid = computerIntraCond(src, mid);
    }

    BasicBlock *entry = &dst->getParent()->getEntryBlock();
    if (!entry) {
        outs() << "Reach a declared fucntion and stop the search\n";
        return src_to_mid;
    }
    assert(entry && "Entry is abtained by fucntion");
    expr entry_to_dst = getTrueCond();
    if (entry->getParent() == dst->getParent()) {
        expr entry_to_dst = computerIntraCond(entry, dst);
    }
    return src_to_mid && entry_to_dst;
}
expr VFGReachable::computerRetCond(BasicBlock *src, BasicBlock *dst, const Call *CallSite) {
    expr src_to_ret = getFalseCond();
    auto RetBBIt = DAA->getDyckCallGraph()->getFunction(src->getParent())->return_bb_begin();
    auto RetBBItEnd = DAA->getDyckCallGraph()->getFunction(src->getParent())->return_bb_end();
    if (RetBBIt == RetBBItEnd) {
        src_to_ret = getTrueCond();
    }
    else {
        for (; RetBBIt != RetBBItEnd; RetBBIt++) {
            expr src_to_ret_i = getTrueCond();
            if (src->getParent() == (*RetBBIt)->getParent()) {
                expr src_to_ret_i = computerIntraCond(src, *RetBBIt);
            }
            src_to_ret = (src_to_ret || src_to_ret_i);
        }
    }

    BasicBlock *mid = CallSite->getInstruction()->getParent();
    expr mid_to_dst = getTrueCond();
    if (mid->getParent() == dst->getParent()) {
        expr mid_to_dst = computerIntraCond(mid, dst);
    }
    return src_to_ret && mid_to_dst;
}
