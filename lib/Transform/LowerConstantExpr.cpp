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

#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include "Transform/LowerConstantExpr.h"

#include <set>
#include <map>

#define DEBUG_TYPE "LowerConstantExpr"

char LowerConstantExpr::ID = 0;
static RegisterPass<LowerConstantExpr> X(DEBUG_TYPE, "Converting constant expr to instructions");

void LowerConstantExpr::getAnalysisUsage(AnalysisUsage &AU) const {
}

static bool transform(Instruction &I) {
    bool Changed = false;
    auto *Phi = dyn_cast<PHINode>(&I);
    if (Phi) {
        std::map<std::pair<Value *, BasicBlock *>, Instruction *> IncomeMap;
        for (unsigned K = 0; K < Phi->getNumIncomingValues(); ++K) {
            auto *OpK = I.getOperand(K);
            if (auto *CE = dyn_cast<ConstantExpr>(OpK)) {
                Instruction *CEInst = IncomeMap[std::make_pair(CE, Phi->getIncomingBlock(K))];
                if (!CEInst) {
                    CEInst = CE->getAsInstruction();
                    IncomeMap[std::make_pair(CE, Phi->getIncomingBlock(K))] = CEInst;
                    CEInst->insertBefore(Phi->getIncomingBlock(K)->getTerminator());
                }
                transform(*CEInst);
                I.setOperand(K, CEInst);
                if (!Changed) Changed = true;
            }
        }
    } else {
        for (unsigned K = 0; K < I.getNumOperands(); ++K) {
            auto *OpK = I.getOperand(K);
            if (auto *CE = dyn_cast<ConstantExpr>(OpK)) {
                auto *CEInst = CE->getAsInstruction();
                CEInst->insertBefore(&I);
                transform(*CEInst);
                I.setOperand(K, CEInst);
                if (!Changed) Changed = true;
            }
        }
    }
    return Changed;
}

static bool transformCall(Instruction &I, const DataLayout &DL) {
    auto *CI = dyn_cast<CallInst>(&I);
    if (!CI) return false;
    auto Callee = CI->getCalledFunction();
    if (Callee) return false;
    auto *CalledOp = CI->getCalledOperand();
    if (isa<ConstantExpr>(CalledOp)) {
        auto *PossibleCallee = CalledOp->stripPointerCastsAndAliases();
        if (isa<Function>(PossibleCallee)) {
            auto *FuncTy = dyn_cast<FunctionType>(PossibleCallee->getType()->getPointerElementType());
            assert(FuncTy);
            if (FuncTy->getNumParams() != CI->arg_size()) return false;

            std::vector<Value *> Args;
            for (unsigned K = 0; K < CI->arg_size(); ++K) {
                auto *ArgK = CI->getArgOperand(K);
                if (ArgK->getType() == FuncTy->getParamType(K)) {
                    Args.push_back(ArgK);
                } else {
                    Value *CastVal = nullptr;
                    if (DL.getTypeSizeInBits(ArgK->getType()) == DL.getTypeSizeInBits(FuncTy->getParamType(K))) {
                        CastVal = BitCastInst::CreateBitOrPointerCast(ArgK, FuncTy->getParamType(K), "", &I);
                    } else if (DL.getTypeSizeInBits(ArgK->getType()) > DL.getTypeSizeInBits(FuncTy->getParamType(K))) {
                        CastVal = TruncInst::CreateTruncOrBitCast(ArgK, FuncTy->getParamType(K), "", &I);
                    } else {
                        CastVal = ZExtInst::CreateZExtOrBitCast(ArgK, FuncTy->getParamType(K), "", &I);
                    }
                    Args.push_back(CastVal);
                }
            }
            auto *NewCI = CallInst::Create(FuncTy, PossibleCallee, Args, "", &I);
            NewCI->setDebugLoc(CI->getDebugLoc());

            auto OriginalRetTy = CI->getType();
            if (OriginalRetTy == NewCI->getType()) {
                I.replaceAllUsesWith(NewCI);
            } else if (CI->getNumUses() == 0) {
                // call void bitcast (i32 (i32)* @reducerror to void (i32)*)(i32 %25) #18
                //  %27 = call i32 @reducerror(i32 %25)
                // void type is not sized, and nothing to repalce.
            } else if (DL.getTypeSizeInBits(OriginalRetTy) == DL.getTypeSizeInBits(NewCI->getType())) {
                auto *BCI = BitCastInst::CreateBitOrPointerCast(NewCI, OriginalRetTy, "", &I);
                I.replaceAllUsesWith(BCI);
            } else if (DL.getTypeSizeInBits(OriginalRetTy) > DL.getTypeSizeInBits(NewCI->getType())) {
                // NewCI to integer, sext, pointer or bitcast
                auto Int = NewCI->getType()->isIntegerTy() ? (Value *) NewCI :
                           CastInst::CreateBitOrPointerCast(NewCI,
                                                            IntegerType::get(I.getContext(),
                                                                             DL.getTypeSizeInBits(
                                                                                     NewCI->getType())),
                                                            "",
                                                            &I);
                auto Ext = CastInst::CreateSExtOrBitCast(Int,
                                                         IntegerType::get(I.getContext(),
                                                                          DL.getTypeSizeInBits(OriginalRetTy)),
                                                         "",
                                                         &I);
                auto Final = CastInst::CreateBitOrPointerCast(Ext, OriginalRetTy, "", &I);
                I.replaceAllUsesWith(Final);
            } else {
                assert(DL.getTypeSizeInBits(OriginalRetTy) < DL.getTypeSizeInBits(NewCI->getType()));
                // NewCI to integer, trunc, pointer or bitcast
                auto Int = NewCI->getType()->isIntegerTy() ? (Value *) NewCI :
                           CastInst::CreateBitOrPointerCast(NewCI,
                                                            IntegerType::get(I.getContext(),
                                                                             DL.getTypeSizeInBits(
                                                                                     NewCI->getType())),
                                                            "",
                                                            &I);
                auto Ext = CastInst::CreateTruncOrBitCast(Int,
                                                          IntegerType::get(I.getContext(),
                                                                           DL.getTypeSizeInBits(OriginalRetTy)),
                                                          "",
                                                          &I);
                auto Final = CastInst::CreateBitOrPointerCast(Ext, OriginalRetTy, "", &I);
                I.replaceAllUsesWith(Final);
            }
            return true;
        }
    }
    return false;
}

bool LowerConstantExpr::runOnModule(Module &M) {
    bool Changed = false;
    for (auto &F: M) {
        for (auto &B: F) {
            for (auto InstIt = B.begin(); InstIt != B.end();) {
                auto &Inst = *InstIt;
                auto Transformed = transformCall(Inst, M.getDataLayout());
                if (Transformed) {
                    if (!Changed) Changed = true;
                    InstIt = Inst.eraseFromParent();
                } else {
                    ++InstIt;
                }
            }
        }
    }

    for (auto &F: M) {
        for (auto &B: F) {
            for (auto &I: B) {
                auto Transformed = transform(I);
                if (!Changed && Transformed) Changed = Transformed;
            }
        }
    }

    if (verifyModule(M, &errs())) {
        llvm_unreachable("Error: LowerConstant fails...");
    }
    return Changed;
}

