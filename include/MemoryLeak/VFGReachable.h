
#ifndef MEMORYLEAK_MLDVFGREACHABLEAN_H
#define MEMORYLEAK_MLDVFGREACHABLEAN_H

#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckCallGraph.h"
#include "DyckAA/DyckGraph.h"
#include "DyckAA/DyckVFG.h"
#include "MemoryLeak/MLDReport.h"
#include "MemoryLeak/MLDVFG.h"
#include "Support/CFG.h"
#include "z3++.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstddef>
#include <map>
#include <utility>

using z3::context;
using z3::expr;
using z3::solver;

class VFGReachable {
    using BrCondMapTy = std::map<llvm::BasicBlock *, std::vector<expr>>;
    using FuncDomTreeMapTy = std::map<llvm::Function *, PostDominatorTree>;
    context GlobalContext;
    solver GlobalSolver;
    BrCondMapTy BranchBBCond;
    FuncDomTreeMapTy FuncDomTree;
    MLDVFG *MVFG;
    DyckVFG *DVFG;
    DyckAliasAnalysis *DAA;
    std::map<llvm::BasicBlock *, expr> BBCond;
    std::map<DyckVFGNode *, expr> VFGNodeCond;
    Slice *CurrSlice;

    std::map<llvm::Function *, CFGRef> CFGMap;

    void computerIntraCond(BasicBlock *src, BasicBlock *dst, std::vector<int> visited, std::vector<std::pair<BasicBlock *, int>> pathCond);
    expr computerIntraCond(BasicBlock *src, BasicBlock *dst);
    expr computerCallCond(BasicBlock *src, BasicBlock *dst, const Call *CallSite);
    expr computerRetCond(BasicBlock *src, BasicBlock *dst, const Call *CallSite);

    void addBBID(BasicBlock *bb);

    inline bool updateBBCond(BasicBlock *BB, expr NewCond) {
        if (BBCond.find(BB) == BBCond.end()) {
            BBCond.insert(std::make_pair(BB, NewCond));
            return true;
        }
        BBCond.insert(std::make_pair(BB, NewCond || BBCond.at(BB)));
        return true;
        // GlobalSolver.push();
        // GlobalSolver.add(NewCond == BBCond.at(BB));
        // z3::check_result res = GlobalSolver.check();
        // GlobalSolver.pop();
        // if (res == z3::sat) {
        //     return false;
        // }
        // else {
            
        // }
    }

    inline expr getTrueCond() {
        return GlobalContext.bool_val(true);
    }

    inline expr getFalseCond() {
        return GlobalContext.bool_val(false);
    }
    inline bool reachable(BasicBlock *b1, BasicBlock *b2) {
        // assert(b1->getParent() == b2->getParent() && "The two basic block should be in the same function");
        CFG* CFG = CFGMap.at(b1->getParent()).get();
        return CFG->reachable(b1, b2);
    }
    inline BasicBlock *getBasciBlock(Value *Val) {
        if (auto Inst = dyn_cast<Instruction>(Val)) {
            return Inst->getParent();
        }
        if (auto Arg = dyn_cast<Argument>(Val)) {
            return &(Arg->getParent()->getEntryBlock());
        }
        return nullptr;
    }
    const Call *getCallSite(int id);

  public:
    MLDReport solveReachable();
    void solveAllSlices();
    VFGReachable(llvm::Module &M, MLDVFG *MVFG, DyckAliasAnalysis *DAA);
    ~VFGReachable(){};
};

#endif