/*
 *  Canary features a fast unification-based alias analysis for C programs
 *  Copyright (C) 2021 Qingkai Shi <qingkaishi@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "DyckAA/DyckVFG.h"
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckGraph.h"
#include "DyckAA/DyckGraphNode.h"
#include "DyckAA/DyckModRefAnalysis.h"
#include "Support/API.h"
#include "Support/CFG.h"
#include "Support/RecursiveTimer.h"
#include "Support/ThreadPool.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <functional>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <map>
#include <vector>

static std::map<Function *, std::map<DyckGraphNode *, std::vector<LoadInst *>>> FuncLoadMap;

static std::map<Function *, std::map<DyckGraphNode *, std::vector<StoreInst *>>> FuncStoreMap;
static std::map<Function *, std::map<DyckGraphNode *, std::vector<InsertValueInst *>>> FuncInsertValueMap;
static std::map<Function *, std::map<DyckGraphNode *, std::vector<ExtractValueInst *>>> FuncExtractValueMap;

static void collectInst(Function &F, DyckAliasAnalysis *DAA, DyckModRefAnalysis *DMRA);

DyckVFG::DyckVFG(DyckAliasAnalysis *DAA, DyckModRefAnalysis *DMRA, Module *M) {
    // create a VFG for each function
    std::map<Function *, CFGRef> LocalCFGMap;
    for (auto &F : *M) {
        if (F.empty())
            continue;
        LocalCFGMap[&F] = nullptr;
        buildLocalVFG(F);
        collectInst(F, DAA, DMRA);
    }

    for (auto &F : *M) {
        if (F.empty())
            continue;
        ThreadPool::get()->enqueue([this, DAA, &F, &LocalCFGMap]() {
            auto LocalCFG = std::make_shared<CFG>(&F);
            LocalCFGMap.at(&F) = LocalCFG;
            buildLocalVFG(DAA, LocalCFG.get(), &F);
        });
    }
    ThreadPool::get()->wait();

    // connect local VFGs
    auto *DyckCG = DAA->getDyckCallGraph();
    for (auto &F : *M) {
        if (F.empty())
            continue;
        auto *CtrlFlow = LocalCFGMap.at(&F).get();
        auto *CGNode = DyckCG->getFunction(&F);
        if (!CGNode)
            continue;
        for (auto &I : instructions(F)) {
            auto *CI = dyn_cast<CallInst>(&I);
            if (!CI)
                continue;
            auto *TheCall = CGNode->getCall(CI);
            if (auto *CC = dyn_cast_or_null<CommonCall>(TheCall)) {
                auto *Callee = dyn_cast<Function>(CC->getCalledFunction());
                assert(Callee);
                if (Callee->empty() && Callee->arg_empty()) {
                    continue;
                }
                connect(DAA, DMRA, TheCall, Callee, CtrlFlow);
            }
            else if (auto *PC = dyn_cast_or_null<PointerCall>(TheCall)) {
                for (Function *Callee : *PC) {
                    if (Callee->empty() && Callee->arg_empty())
                        continue;
                    connect(DAA, DMRA, TheCall, Callee, CtrlFlow);
                }
            }
        }
    }
}

static void collectInst(Function &F, DyckAliasAnalysis *DAA, DyckModRefAnalysis *DMRA) {

    DyckGraph *DG = DAA->getDyckGraph();
    for (auto &Inst : instructions(F)) {
        if (auto *Load = dyn_cast<LoadInst>(&Inst)) {
            auto *Ptr = Load->getPointerOperand();
            auto *DV = DG->findDyckVertex(Ptr);
            if (DV)
                FuncLoadMap[&F][DV].push_back(Load);
        }
        else if (auto *Store = dyn_cast<StoreInst>(&Inst)) {
            auto *Ptr = Store->getPointerOperand();
            auto *DV = DG->findDyckVertex(Ptr);
            if (DV)
                FuncStoreMap[&F][DV].push_back(Store);
        }
        else if (auto *InsertValue = dyn_cast<InsertValueInst>(&Inst)) {
            auto *DV = DG->findDyckVertex(InsertValue);
            if (DV)
                FuncInsertValueMap[&F][DV].push_back(InsertValue);
        }
        else if (auto *ExtractValue = dyn_cast<ExtractValueInst>(&Inst)) {
            auto *DV = DG->findDyckVertex(ExtractValue->getAggregateOperand());
            if (DV)
                FuncExtractValueMap[&F][DV].push_back(ExtractValue);
        }
    }
}

void DyckVFG::buildLocalVFG(Function &F) {
    // direct value flow through cast, gep-0-0, select, phi
    for (auto &I : instructions(F)) {
        if (isa<CastInst>(I) || isa<PHINode>(I)) {
            auto *ToNode = getOrCreateVFGNode(&I);
            for (unsigned K = 0; K < I.getNumOperands(); ++K) {
                auto *From = I.getOperand(K);
                auto *FromNode = getOrCreateVFGNode(From);
                FromNode->addTarget(ToNode);
            }
        }
        else if (isa<SelectInst>(I)) {
            auto *ToNode = getOrCreateVFGNode(&I);
            for (unsigned K = 1; K < I.getNumOperands(); ++K) {
                auto *From = I.getOperand(K);
                auto *FromNode = getOrCreateVFGNode(From);
                FromNode->addTarget(ToNode);
            }
        }
        else if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
            bool VF = true;
            for (auto &Index : GEP->indices()) {
                if (auto *CI = dyn_cast<ConstantInt>(&Index)) {
                    // I = gep ptr, idx1, idx2 ... iff when all idx{i} are equal to zero, value of ptr flows into I.
                    // In fact, the gep operation will genenrate a new value, but how can we track the alias realation of two
                    // value generated by gep operation?
                    // why not alias the two value in alias analysis ?
                    if (CI->getSExtValue() != 0) {
                        VF = false;
                        break;
                    }
                }
            }
            if (VF) {
                auto *ToNode = getOrCreateVFGNode(&I);
                auto *FromNode = getOrCreateVFGNode(GEP->getPointerOperand());
                FromNode->addTarget(ToNode);
            }
        }
        else if (isa<LoadInst>(I)) {
            getOrCreateVFGNode(&I);
            getOrCreateVFGNode(I.getOperand(0));
        }
        else if (isa<StoreInst>(I)) {
            getOrCreateVFGNode(I.getOperand(0));
            getOrCreateVFGNode(I.getOperand(1));
        }
        else if (isa<InsertValueInst>(&I)) {
            auto IVInst = dyn_cast<InsertValueInst>(&I);
            auto AggValue = IVInst->getAggregateOperand();
            if (!isa<UndefValue>(AggValue)) {
                getOrCreateVFGNode(AggValue);
            }
            auto InvertValueOperand = IVInst->getInsertedValueOperand();
            getOrCreateVFGNode(InvertValueOperand);
            getOrCreateVFGNode(IVInst);
        }
        else if (isa<ExtractValueInst>(&I)) {
            auto EVInst = dyn_cast<ExtractValueInst>(&I);
            getOrCreateVFGNode(EVInst);
            getOrCreateVFGNode(EVInst->getAggregateOperand());
        }
        else if(isa<CallInst>(&I) && API::isHeapAllocate(&I)){
            getOrCreateVFGNode(&I);
        }
    }
}

void DyckVFG::buildLocalVFG(DyckAliasAnalysis *DAA, CFG *CtrlFlow, Function *F) const {
    // indirect value flow through load/store
    auto *DG = DAA->getDyckGraph();
    std::map<DyckGraphNode *, std::vector<LoadInst *>> &LoadMap = FuncLoadMap[F];                         // ptr -> load
    std::map<DyckGraphNode *, std::vector<StoreInst *>> &StoreMap = FuncStoreMap[F];                      // ptr -> store
    std::map<DyckGraphNode *, std::vector<InsertValueInst *>> &InsertValueMap = FuncInsertValueMap[F];    // ptr -> InsertValue
    std::map<DyckGraphNode *, std::vector<ExtractValueInst *>> &ExtractValueMap = FuncExtractValueMap[F]; // ptr -> ExtractValue
    // match load and store:
    // if alias(load's ptr, store's ptr) and store -> load in CFG, add store's value -> load's value in VFG
    for (auto &LoadIt : LoadMap) {
        auto *DyckNode = LoadIt.first;
        auto &Loads = LoadIt.second;
        auto StoreIt = StoreMap.find(DyckNode);
        if (StoreIt == StoreMap.end())
            continue;
        auto &Stores = StoreIt->second;
        for (auto *Load : Loads) {
            auto *LdNode = getVFGNode(Load);
            assert(LdNode);
            for (auto *Store : Stores) {
                if (CtrlFlow->reachable(Store, Load)) {
                    auto *StNode = getVFGNode(Store->getValueOperand());
                    assert(StNode);
                    StNode->addTarget(LdNode);
                }
            }
        }
    }

    for (auto ExtractValueIt : ExtractValueMap) {
        DyckGraphNode *DNode = ExtractValueIt.first;
        auto &EVInstSet = ExtractValueIt.second;
        auto InsertValueIt = InsertValueMap.find(DNode);
        if (InsertValueIt == InsertValueMap.end())
            continue;
        auto &IVInstSet = InsertValueIt->second;
        for (auto ExtractValue : EVInstSet) {
            auto *ExtractNode = getVFGNode(ExtractValue);
            assert(ExtractNode);
            for (auto *InsertValue : IVInstSet) {
                if (CtrlFlow->reachable(InsertValue, ExtractValue)) {
                    bool AllIdxEq = true;
                    bool AllIdxZero = true;
                    if (ExtractValue->getNumIndices() != InsertValue->getNumIndices()) {
                        AllIdxEq = false;
                        for (auto i : ExtractValue->getIndices()) {
                            AllIdxZero &= !!i;
                        }
                    }
                    else {
                        for (auto i = ExtractValue->getIndices().begin(), j = InsertValue->getIndices().begin();
                             i != ExtractValue->getIndices().end() && j != InsertValue->getIndices().end(); i++, j++) {
                            AllIdxEq &= (*i) == (*j);
                        }
                    }
                    if (AllIdxEq || AllIdxZero) {
                        auto *InsertNode = getVFGNode(InsertValue->getInsertedValueOperand());
                        InsertNode->addTarget(ExtractNode);
                    }
                }
            }
        }
    }
}

DyckVFG::~DyckVFG() {
    for (auto &It : ValueNodeMap)
        delete It.second;
}

DyckVFGNode *DyckVFG::getVFGNode(Value *V) const {
    auto It = ValueNodeMap.find(V);
    if (It == ValueNodeMap.end())
        return nullptr;
    return It->second;
}

DyckVFGNode *DyckVFG::getOrCreateVFGNode(Value *V) {
    auto It = ValueNodeMap.find(V);
    if (It == ValueNodeMap.end()) {
        auto *Ret = new DyckVFGNode(V);
        ValueNodeMap[V] = Ret;
        return Ret;
    }
    return It->second;
}

static void collectValues(std::set<DyckGraphNode *>::iterator Begin, std::set<DyckGraphNode *>::iterator End,
                          std::vector<std::set<Value *>> &CallerVals, std::vector<std::set<Value *>> &CalleeVals, Call *C,
                          Function *Callee, CFG *Ctrl) {
    auto *Caller = C->getInstruction()->getFunction();
    int Idx = 0;
    for (auto It = Begin; It != End; ++It) {
        CalleeVals.emplace_back();
        CallerVals.emplace_back();
        auto *N = *It;
        auto *ValSet = (std::set<Value *> *)N->getEquivalentSet();
        for (auto *V : *ValSet) {
            if (auto *Arg = dyn_cast<Argument>(V)) {
                if (Arg->getParent() == Callee)
                    CalleeVals[Idx].insert(Arg);
                else if (Arg->getParent() == Caller)
                    CallerVals[Idx].insert(Arg);
            }
            else if (auto *Inst = dyn_cast<Instruction>(V)) {
                if (Inst->getFunction() == Callee)
                    CalleeVals[Idx].insert(Inst);
                else if (Inst->getFunction() == Caller && Ctrl->reachable(Inst, C->getInstruction()))
                    CallerVals[Idx].insert(Inst);
            }
        }
    }
}

void DyckVFG::connectInsertExtractIndirectFlow(std::map<DyckGraphNode *, std::vector<ExtractValueInst *>> ExtractValueMap,
                                               std::map<DyckGraphNode *, std::vector<InsertValueInst *>> InsertValueMap,
                                               std::function<bool(Instruction *, Instruction *)> Reachable, int CallId) {
    for (auto ExtractValueIt : ExtractValueMap) {
        DyckGraphNode *DNode = ExtractValueIt.first;
        auto &EVInstSet = ExtractValueIt.second;
        auto InsertValueIt = InsertValueMap.find(DNode);
        if (InsertValueIt == InsertValueMap.end())
            continue;
        auto &IVInstSet = InsertValueIt->second;
        for (auto ExtractValue : EVInstSet) {
            auto *ExtractNode = getVFGNode(ExtractValue);
            assert(ExtractNode);
            for (auto *InsertValue : IVInstSet) {
                if (Reachable(InsertValue, ExtractValue)) {
                    bool AllIdxEq = true;
                    bool AllIdxZero = true;
                    if (ExtractValue->getNumIndices() != InsertValue->getNumIndices()) {
                        AllIdxEq = false;
                        for (auto i : ExtractValue->getIndices()) {
                            AllIdxZero &= !!i;
                        }
                    }
                    else {
                        for (auto i = ExtractValue->getIndices().begin(), j = InsertValue->getIndices().begin();
                             i != ExtractValue->getIndices().end() && j != InsertValue->getIndices().end(); i++, j++) {
                            AllIdxEq &= ((*i) == (*j));
                            AllIdxZero &= (*i == 0 && *j == 0);
                        }
                    }
                    if (AllIdxEq || AllIdxZero) {
                        auto *InsertNode = getVFGNode(InsertValue->getInsertedValueOperand());
                        InsertNode->addTarget(ExtractNode, CallId);
                    }
                }
            }
        }
    }
}

void DyckVFG::connect(DyckAliasAnalysis *DAA, DyckModRefAnalysis *DMRA, Call *C, Function *Callee, CFG *Ctrl) {
    // connect direct inputs
    for (unsigned K = 0; K < C->numArgs(); ++K) {
        if (K >= Callee->arg_size())
            continue; // ignore var args
        auto *Actual = C->getArg(K);
        auto *ActualNode = getOrCreateVFGNode(Actual);
        auto *Formal = Callee->getArg(K);
        auto *FormalNode = getOrCreateVFGNode(Formal);
        ActualNode->addTarget(FormalNode, C->id());
    }
    // connect direct outputs
    if (!C->getInstruction()->getType()->isVoidTy()) {
        auto *ActualRet = getOrCreateVFGNode(C->getInstruction());
        for (auto &Inst : instructions(Callee)) {
            auto *RetInst = dyn_cast<ReturnInst>(&Inst);
            if (!RetInst)
                continue;
            if (RetInst->getNumOperands() != 1)
                continue;
            auto *FormalRet = getOrCreateVFGNode(Inst.getOperand(0));
            FormalRet->addTarget(ActualRet, -C->id());
        }
    }
    // if this function does not contain any basicblock
    if (Callee->empty()) {
        return;
    }
    // this callee does not contain mod/refs except for formal parameters/rets
    if (!DMRA->count(Callee))
        return;

    // connect indirect inputs
    //  1. get refs, get ref values (in caller, C is reachable from these values, and callee)
    //  2. connect ref values (caller) -> ref values (callee)
    std::vector<std::set<Value *>> RefCallerValues, RefCalleeValues;
    collectValues(DMRA->ref_begin(Callee), DMRA->ref_end(Callee), RefCallerValues, RefCalleeValues, C, Callee, Ctrl);
    for (int i = 0; i < RefCalleeValues.size(); i++)
        for (auto *CallerVal : RefCallerValues[i])
            for (auto *CalleeVal : RefCalleeValues[i])
                getOrCreateVFGNode(CallerVal)->addTarget(getOrCreateVFGNode(CalleeVal), C->id());

    // connect indirect outputs
    //  1. get mods, get mod values (in caller and callee)
    //  2. connect ref values (callee) -> ref values (caller)
    std::vector<std::set<Value *>> ModCallerValues, ModCalleeValues;
    collectValues(DMRA->mod_begin(Callee), DMRA->mod_end(Callee), ModCallerValues, ModCalleeValues, C, Callee, Ctrl);
    for (int i = 0; i < RefCalleeValues.size(); i++)
        for (auto *CalleeVal : ModCalleeValues[i])
            for (auto *CallerVal : ModCallerValues[i])
                getOrCreateVFGNode(CalleeVal)->addTarget(getOrCreateVFGNode(CallerVal), -C->id());
    // connect indirect inputs through field accesses
    auto I2CallSite = [&C, &Ctrl](Instruction *I, Instruction *E) -> bool { return Ctrl->reachable(I, C->getInstruction()); };
    
    connectInsertExtractIndirectFlow(FuncExtractValueMap[Callee], FuncInsertValueMap[C->getInstruction()->getFunction()],
                                     I2CallSite, C->id());
    // connect indirect outputs through filed accesses

    auto CallSite2E = [&C, &Ctrl](Instruction *I, Instruction *E) -> bool { return Ctrl->reachable(C->getInstruction(), E); };

    connectInsertExtractIndirectFlow(FuncExtractValueMap[C->getInstruction()->getFunction()], FuncInsertValueMap[Callee],
                                     CallSite2E, -C->id());
}

Function *DyckVFGNode::getFunction() const {
    if (!V)
        return nullptr;
    if (auto *Arg = dyn_cast<Argument>(V)) {
        return Arg->getParent();
    }
    else if (auto *Inst = dyn_cast<Instruction>(V)) {
        return Inst->getFunction();
    }
    return nullptr;
}