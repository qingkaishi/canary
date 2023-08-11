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

#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckGraph.h"
#include "DyckAA/DyckGraphNode.h"
#include "DyckAA/DyckModRefAnalysis.h"
#include "DyckAA/DyckVFG.h"
#include "Support/CFG.h"

DyckVFG::DyckVFG(DyckAliasAnalysis *DAA, DyckModRefAnalysis *DMRA, Module *M) {
    // create a VFG for each function
    std::map<Function *, CFG *> LocalCFGMap;
    for (auto &F: *M) {
        if (F.empty()) continue;
        auto *NewCFG = new CFG(&F);
        buildLocalVFG(DAA, NewCFG, &F);
        LocalCFGMap[&F] = NewCFG;
    }

    // connect local VFGs
    auto *DyckCG = DAA->getDyckCallGraph();
    for (auto &F: *M) {
        if (F.empty()) continue;
        auto *CtrlFlow = LocalCFGMap.at(&F);
        auto *CGNode = DyckCG->getFunction(&F);
        if (!CGNode) continue;
        for (auto &I: instructions(F)) {
            auto *CI = dyn_cast<CallInst>(&I);
            if (!CI) continue;
            auto *TheCall = CGNode->getCall(CI);
            if (auto *CC = dyn_cast_or_null<CommonCall>(TheCall)) {
                auto *Callee = dyn_cast<Function>(CC->getCalledFunction());
                assert(Callee);
                if (Callee->empty()) continue;
                connect(DMRA, TheCall, Callee, CtrlFlow);
            } else if (auto *PC = dyn_cast_or_null<PointerCall>(TheCall)) {
                for (Function *Callee: *PC) {
                    if (Callee->empty()) continue;
                    connect(DMRA, TheCall, Callee, CtrlFlow);
                }
            }
        }
    }

    for (auto &It: LocalCFGMap) delete It.second;
}

void DyckVFG::buildLocalVFG(DyckAliasAnalysis *DAA, CFG *CtrlFlow, Function *F) {
    // direct value flow through cast, gep-0-0, select, phi
    for (auto &I: instructions(F)) {
        if (isa<CastInst>(I) || isa<PHINode>(I)) {
            auto *ToNode = getOrCreateVFGNode(&I);
            for (unsigned K = 0; K < I.getNumOperands(); ++K) {
                auto *From = I.getOperand(K);
                auto *FromNode = getOrCreateVFGNode(From);
                FromNode->addTarget(ToNode);
            }
        } else if (isa<SelectInst>(I)) {
            auto *ToNode = getOrCreateVFGNode(&I);
            for (unsigned K = 1; K < I.getNumOperands(); ++K) {
                auto *From = I.getOperand(K);
                auto *FromNode = getOrCreateVFGNode(From);
                FromNode->addTarget(ToNode);
            }
        } else if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
            bool VF = true;
            for (auto &Index: GEP->indices()) {
                if (auto *CI = dyn_cast<ConstantInt>(&Index)) {
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
    }

    // indirect value flow through load/store
    auto *DG = DAA->getDyckGraph();
    std::map<DyckGraphNode *, std::vector<LoadInst *>> LoadMap; // ptr -> load
    std::map<DyckGraphNode *, std::vector<StoreInst *>> StoreMap; // ptr -> store
    for (auto &I: instructions(F)) {
        if (auto *Load = dyn_cast<LoadInst>(&I)) {
            auto *Ptr = Load->getPointerOperand();
            auto *DV = DG->findDyckVertex(Ptr);
            if (DV) LoadMap[DV].push_back(Load);
        } else if (auto *Store = dyn_cast<StoreInst>(&I)) {
            auto *Ptr = Store->getPointerOperand();
            auto *DV = DG->findDyckVertex(Ptr);
            if (DV) StoreMap[DV].push_back(Store);
        }
    }
    // match load and store:
    // if alias(load's ptr, store's ptr) and store -> load in CFG, add store's value -> load's value in VFG
    for (auto &LoadIt: LoadMap) {
        auto *DyckNode = LoadIt.first;
        auto &Loads = LoadIt.second;
        auto StoreIt = StoreMap.find(DyckNode);
        if (StoreIt == StoreMap.end()) continue;
        auto &Stores = StoreIt->second;
        for (auto *Load: Loads) {
            auto *LdNode = getOrCreateVFGNode(Load);
            for (auto *Store: Stores)
                if (CtrlFlow->reachable(Store, Load)) getOrCreateVFGNode(Store->getValueOperand())->addTarget(LdNode);
        }
    }
}

DyckVFG::~DyckVFG() {
    for (auto &It: ValueNodeMap) delete It.second;
}

DyckVFGNode *DyckVFG::getVFGNode(Value *V) const {
    auto It = ValueNodeMap.find(V);
    if (It == ValueNodeMap.end()) return nullptr;
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
                          std::set<Value *> &CallerVals, std::set<Value *> &CalleeVals, Call *C, Function *Callee,
                          CFG *Ctrl) {
    auto *Caller = C->getInstruction()->getFunction();
    for (auto It = Begin; It != End; ++It) {
        auto *N = *It;
        auto *ValSet = (std::set<Value *> *) N->getEquivalentSet();
        for (auto *V: *ValSet) {
            if (auto *Arg = dyn_cast<Argument>(V)) {
                if (Arg->getParent() == Callee) CalleeVals.insert(Arg);
                else if (Arg->getParent() == Caller) CallerVals.insert(Arg);
            } else if (auto *Inst = dyn_cast<Instruction>(V)) {
                if (Inst->getFunction() == Callee) CalleeVals.insert(Inst);
                else if (Inst->getFunction() == Caller && Ctrl->reachable(Inst, C->getInstruction()))
                    CallerVals.insert(Inst);
            }
        }
    }
}

void DyckVFG::connect(DyckModRefAnalysis *DMRA, Call *C, Function *Callee, CFG *Ctrl) {
    // connect direct inputs
    for (unsigned K = 0; K < C->numArgs(); ++K) {
        if (K >= Callee->arg_size()) continue; // ignore var args
        auto *Actual = C->getArg(K);
        auto *ActualNode = getOrCreateVFGNode(Actual);
        auto *Formal = Callee->getArg(K);
        auto *FormalNode = getOrCreateVFGNode(Formal);
        ActualNode->addTarget(FormalNode);
    }
    // connect direct outputs
    if (!C->getInstruction()->getType()->isVoidTy()) {
        auto *ActualRet = getOrCreateVFGNode(C->getInstruction());
        for (auto &Inst: instructions(Callee)) {
            auto *RetInst = dyn_cast<ReturnInst>(&Inst);
            if (!RetInst) continue;
            if (RetInst->getNumOperands() != 1) continue;
            auto *FormalRet = getOrCreateVFGNode(Inst.getOperand(0));
            FormalRet->addTarget(ActualRet);
        }
    }

    // this callee does not contain mod/refs except for formal parameters/rets
    if (!DMRA->count(Callee)) return;

    // connect indirect inputs
    //  1. get refs, get ref values (in caller, C is reachable from these values, and callee)
    //  2. connect ref values (caller) -> ref values (callee)
    std::set<Value *> RefCallerValues, RefCalleeValues;
    collectValues(DMRA->ref_begin(Callee), DMRA->ref_end(Callee), RefCallerValues, RefCalleeValues, C, Callee, Ctrl);
    for (auto *CallerVal: RefCallerValues)
        for (auto *CalleeVal: RefCalleeValues)
            getOrCreateVFGNode(CallerVal)->addTarget(getOrCreateVFGNode(CalleeVal));

    // connect indirect outputs
    //  1. get mods, get mod values (in caller and callee)
    //  2. connect ref values (callee) -> ref values (caller)
    std::set<Value *> ModCallerValues, ModCalleeValues;
    collectValues(DMRA->mod_begin(Callee), DMRA->mod_end(Callee), ModCallerValues, ModCalleeValues, C, Callee, Ctrl);
    for (auto *CalleeVal: ModCalleeValues)
        for (auto *CallerVal: ModCallerValues)
            getOrCreateVFGNode(CalleeVal)->addTarget(getOrCreateVFGNode(CallerVal));
}
