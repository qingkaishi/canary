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

#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <set>
#include "NCA/LocalNullCheckAnalysis.h"
#include "Support/API.h"

LocalNullCheckAnalysis::LocalNullCheckAnalysis(Function *F) : F(F) {
    for (unsigned K = 0; K < F->arg_size(); ++K) {
        auto *Arg = F->getArg(K);
        if (Arg->getType()->isPointerTy()) {
            IDPtrMap.push_back(Arg);
            PtrIDMap[Arg] = IDPtrMap.size() - 1;
        }
    }

    for (auto &B: *F) {
        for (auto &I: B) {
            for (unsigned K = 0; K < I.getNumOperands(); ++K) {
                auto Op = I.getOperand(K);
                if (!Op->getType()->isPointerTy()) continue;
                if (nonnull(Op)) continue;
                auto It = PtrIDMap.find(Op);
                if (It == PtrIDMap.end()) {
                    IDPtrMap.push_back(Op);
                    PtrIDMap[Op] = IDPtrMap.size() - 1;
                }
            }
        }
    }
}

bool LocalNullCheckAnalysis::nonnull(Value *V) {
    V = V->stripPointerCastsAndAliases();
    if (isa<GlobalValue>(V)) return true;
    if (auto CI = dyn_cast<Instruction>(V))
        return API::isMemoryAllocate(CI);
    return false;
}

bool LocalNullCheckAnalysis::mayNull(Value *Ptr, Instruction *Inst) {
    bool IsOperand = false;
    unsigned K = 0;
    for (; K < Inst->getNumOperands(); ++K) {
        if (Ptr == Inst->getOperand(K)) {
            IsOperand = true;
            break;
        }
    }
    assert(IsOperand && "Ptr must be an operand of Inst!");
    if (!Ptr->getType()->isPointerTy() || nonnull(Ptr)) return false; // not used as a ptr type
    if (K >= 32) return true; // more than 32 operands in this weird instruction
    auto It = InstNonNullMap.find(Inst);
    assert(It != InstNonNullMap.end());
    return !(It->second & (1 << K));
}

void LocalNullCheckAnalysis::run() {
    outs() << "running on " << F->getName() << " (ptrsize: " << IDPtrMap.size() << "; blocksize: " << F->size() << ")\n";

    // 1. init a map from each instruction to a set of nonnull pointers
    init();

    // 2. fixed-point algorithm for null check analysis
    nca();

    // 3. tag possible null pointers at each instruction
    tag();
}

void LocalNullCheckAnalysis::init() {
    for (auto &B: *F) {
        for (auto &I: B) {
            if (I.isTerminator()) {
                for (unsigned K = 0; K < I.getNumSuccessors(); ++K) {
                    DataflowFacts[{&I, K}] = BitVector(IDPtrMap.size());
                }
            } else {
                DataflowFacts[{&I, 0}] = BitVector(IDPtrMap.size());
            }
        }
    }
    DataflowFacts[{nullptr, 0}] = BitVector(IDPtrMap.size());
}

void LocalNullCheckAnalysis::tag() {
    for (auto &B: *F) for (auto &I: B) InstNonNullMap[&I] = 0;

    BitVector ResultOfMerging;
    std::vector<Edge> IncomingEdges;
    for (auto &B: *F) {
        for (auto &I: B) {
            IncomingEdges.clear();
            if (&I == &*B.begin()) {
                for (auto PIt = pred_begin(&B), PE = pred_end(&B); PIt != PE; ++PIt) {
                    auto Term = (*PIt)->getTerminator();
                    for (unsigned K = 0; K < Term->getNumSuccessors(); ++K) {
                        if (Term->getSuccessor(K) == I.getParent()) {
                            IncomingEdges.emplace_back(Term, K);
                        }
                    }
                }
            } else {
                IncomingEdges.emplace_back(I.getPrevNode(), 0);
            }
            BitVector *NonNulls;
            if (IncomingEdges.empty()) {
                NonNulls = &DataflowFacts.at({nullptr, 0});
            } else if (IncomingEdges.size() == 1) {
                NonNulls = &DataflowFacts.at(IncomingEdges[0]);
            } else {
                merge(IncomingEdges, ResultOfMerging);
                NonNulls = &ResultOfMerging;
            }
            auto &Orig = InstNonNullMap[&I];
            for (auto K = 0; K < I.getNumOperands(); ++K) {
                auto OpK = I.getOperand(K);
                auto It = PtrIDMap.find(OpK);
                if (It == PtrIDMap.end()) continue;
                auto OpKMustNonNull = NonNulls->test(It->second);
                if (OpKMustNonNull) {
                    Orig = Orig | (1 << K);
                }
            }
        }
    }
}

void LocalNullCheckAnalysis::merge(std::vector<Edge> &IncomingEdges, BitVector &Result) {
    auto It = IncomingEdges.begin();
    Result = DataflowFacts.at(*It++);
    while (It != IncomingEdges.end()) {
        auto &NextSet = DataflowFacts.at(*It);
        Result &= NextSet;
        It++;
    }
}

void LocalNullCheckAnalysis::transfer(Edge E, const BitVector &In, BitVector &Out) {
    // assume In, evaluate E's fact as Out
    Out = In;

    auto Set = [this, &Out](Value *Ptr) {
        size_t PtrID = UINT64_MAX;
        auto It = PtrIDMap.find(Ptr);
        if (It != PtrIDMap.end())
            PtrID = It->second;
        if (PtrID < Out.size()) Out.set(PtrID);
    };

    auto Count = [this, &In](Value *Ptr) -> bool {
        size_t PtrID = UINT64_MAX;
        auto It = PtrIDMap.find(Ptr);
        if (It != PtrIDMap.end())
            PtrID = It->second;
        if (PtrID < In.size())
            return In.test(PtrID);
        else
            return true;
    };

    // analyze each instruction type
    auto *Inst = E.first;
    switch (Inst->getOpcode()) {
        case Instruction::Load:
        case Instruction::Store:
        case Instruction::GetElementPtr:
            Set(getPointerOperand(Inst));
            break;
        case Instruction::Alloca:
            Set(Inst);
            break;
        case Instruction::AddrSpaceCast:
        case Instruction::BitCast:
            if (Count(Inst->getOperand(0))) Set(Inst);
            break;
        case Instruction::PHI:
        case Instruction::Select:
            if (Inst->getType()->isPointerTy()) {
                bool AllNonNull = true;
                for (unsigned K = 0; K < Inst->getNumOperands(); ++K) {
                    auto Op = Inst->getOperand(K);
                    if (!Op->getType()->isPointerTy()) continue;
                    if (!Count(Op)) {
                        AllNonNull = false;
                        break;
                    }
                }
                if (AllNonNull) Set(Inst);
            }
            break;
        case Instruction::Call: {
            auto *CI = (CallInst *) Inst;
            if (CI->getCalledFunction()) {
                if (API::isMemoryAllocate(CI)) Set(Inst);
            } else {
                Set(CI->getCalledOperand());
            }
        }
            break;
        case Instruction::ICmp: {
            // todo
        }
            break;
        default:
            break;
    }
}

void LocalNullCheckAnalysis::nca() {
    std::vector<Edge> WorkList;
    for (auto &B: *F)
        for (auto &I: B) {
            if (I.isTerminator()) {
                for (unsigned K = 0; K < I.getNumSuccessors(); ++K)
                    WorkList.emplace_back(&I, K);
            } else {
                WorkList.emplace_back(&I, 0);
            }
        }
    std::reverse(WorkList.begin(), WorkList.end());

    std::vector<Edge> IncomingEdges;
    BitVector ResultOfMerging;
    BitVector ResultOfTransfer;
    while (!WorkList.empty()) {
        auto Edge = WorkList.back();
        auto EdgeInst = Edge.first;
        auto &EdgeFact = DataflowFacts.at(Edge); // this loop iteration re-compute EdgeFact
        WorkList.pop_back();

        // 1. merge
        IncomingEdges.clear();
        if (EdgeInst == &(EdgeInst->getParent()->front())) {
            auto *B = EdgeInst->getParent();
            for (auto PIt = pred_begin(B), PE = pred_end(B); PIt != PE; ++PIt) {
                auto Term = (*PIt)->getTerminator();
                for (unsigned K = 0; K < Term->getNumSuccessors(); ++K) {
                    if (Term->getSuccessor(K) == B) {
                        IncomingEdges.emplace_back(Term, K);
                    }
                }
            }
        } else {
            IncomingEdges.emplace_back(EdgeInst->getPrevNode(), 0);
        }
        BitVector *NonNulls;
        if (IncomingEdges.empty()) {
            NonNulls = &DataflowFacts.at({nullptr, 0});
        } else if (IncomingEdges.size() == 1) {
            NonNulls = &DataflowFacts.at(IncomingEdges[0]);
        } else {
            merge(IncomingEdges, ResultOfMerging);
            NonNulls = &ResultOfMerging;
        }

        // 2. transfer
        ResultOfTransfer.clear();
        transfer(Edge, *NonNulls, ResultOfTransfer);

        // 3. add necessary ones to worklist
        if (ResultOfTransfer != EdgeFact) {
            EdgeFact.swap(ResultOfTransfer);
            auto NextInst = EdgeInst->isTerminator() ? &EdgeInst->getSuccessor(Edge.second)->front() :
                            EdgeInst->getNextNode();
            if (NextInst->isTerminator()) {
                for (unsigned K = 0; K < NextInst->getNumSuccessors(); ++K) {
                    WorkList.emplace_back(NextInst, K);
                }
            } else {
                WorkList.emplace_back(NextInst, 0);
            }
        }
    }
}