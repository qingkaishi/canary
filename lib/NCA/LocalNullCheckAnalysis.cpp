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
#include <llvm/IR/Intrinsics.h>
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

    // todo init unreachable edges by calling label(Edge)
}

bool LocalNullCheckAnalysis::nonnull(Value *V) {
    V = V->stripPointerCastsAndAliases();
    if (isa<GlobalValue>(V)) return true;
    if (auto CI = dyn_cast<Instruction>(V))
        return API::isMemoryAllocate(CI);
    return false;
}

bool LocalNullCheckAnalysis::mayNull(Value *Ptr, Instruction *Inst) {
    // not used as a ptr type or must be nonnull
    if (!Ptr->getType()->isPointerTy() || nonnull(Ptr)) return false;

    // ptrs in unreachable blocks are considered nonnull
    bool AllPredUnreachable = true;
    if (Inst == &Inst->getParent()->front()) {
        for (auto PIt = pred_begin(Inst->getParent()), PE = pred_end(Inst->getParent()); PIt != PE; ++PIt) {
            auto *PredTerm = (*PIt)->getTerminator();
            for (unsigned J = 0; J < PredTerm->getNumSuccessors(); ++J) {
                if (PredTerm->getSuccessor(J) != Inst->getParent()) continue;
                if (!UnreachableEdges.count({PredTerm, J})) {
                    AllPredUnreachable = false;
                    break;
                }
            }
            if (!AllPredUnreachable) break;
        }
    } else {
        AllPredUnreachable = UnreachableEdges.count({Inst->getPrevNode(), 0});
    }
    if (AllPredUnreachable) return false;

    // check nca results
    bool IsOperand = false;
    unsigned K = 0;
    for (; K < Inst->getNumOperands(); ++K) {
        if (Ptr == Inst->getOperand(K)) {
            IsOperand = true;
            break;
        }
    }
    assert(IsOperand && "Ptr must be an operand of Inst!");
    if (K >= 32) return true; // more than 32 operands in this weird instruction
    auto It = InstNonNullMap.find(Inst);
    assert(It != InstNonNullMap.end());
    return !(It->second & (1 << K));
}

void LocalNullCheckAnalysis::run() {
    outs() << "running on " << F->getName() << " (# ptrs: " << IDPtrMap.size() << "; # blocks: " << F->size() << ")\n";

    // 1. init a map from each instruction to a set of nonnull pointers
    init();

    // 2. fixed-point algorithm for null check analysis
    nca();

    // 3. tag possible null pointers at each instruction
    tag();

    // 4. label unreachable edges
    label();
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
    auto BrNo = E.second;
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
            if (auto *Callee = CI->getCalledFunction()) {
                if (Callee->isIntrinsic() && Callee->getIntrinsicID() >= Intrinsic::memcpy
                    && Callee->getIntrinsicID() <= Intrinsic::memset_element_unordered_atomic) {
                    for (unsigned K = 0; K < CI->getNumArgOperands(); ++K) {
                        auto Op = CI->getArgOperand(K);
                        if (!Op->getType()->isPointerTy()) continue;
                        Set(Op);
                    }
                } else if (API::isMemoryAllocate(CI)) Set(Inst);
            } else {
                Set(CI->getCalledOperand());
            }
        }
            break;
        case Instruction::ICmp: {
            auto CmpInst = (ICmpInst *) Inst;
            auto Op0 = Inst->getOperand(0);
            auto Op1 = Inst->getOperand(1);
            if (CmpInst->getPredicate() == CmpInst::ICMP_EQ && BrNo == 1
                || CmpInst->getPredicate() == CmpInst::ICMP_NE && BrNo == 0) {
                if (isa<ConstantPointerNull>(Op0)) Set(Op1);
                else if (isa<ConstantPointerNull>(Op1)) Set(Op0);
            }
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
        WorkList.pop_back();
        if (UnreachableEdges.count(Edge)) continue;

        auto EdgeInst = Edge.first;
        auto &EdgeFact = DataflowFacts.at(Edge); // this loop iteration re-compute EdgeFact

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
                    if (UnreachableEdges.count({NextInst, K})) continue;
                    WorkList.emplace_back(NextInst, K);
                }
            } else if (!UnreachableEdges.count({NextInst, 0})) {
                WorkList.emplace_back(NextInst, 0);
            }
        }
    }
}

void LocalNullCheckAnalysis::label() {
    // todo label unreachable edges based on nca's result
}

void LocalNullCheckAnalysis::label(Edge) {
    // todo
}