/*
 *  Popeye lifts protocol source code in C to its specification in BNF
 *  Copyright (C) 2023 Qingkai Shi <qingkaishi@gmail.com>
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
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include "Support/Statistics.h"

Statistics::Statistics() = default;

void Statistics::run(Module &M) {
    unsigned NumInstructions = 0;
    unsigned NumPointerInstructions = 0;
    unsigned NumDerefInstructions = 0;
    for (auto &F: M) {
        if (F.empty()) continue;
        for (auto &I: instructions(F)) {
            if (I.isDebugOrPseudoInst()) continue;
            ++NumInstructions;
            for (unsigned K = 0; K < I.getNumOperands(); ++K) {
                if (I.getOperand(K)->getType()->isPointerTy()) {
                    ++NumPointerInstructions;
                    break;
                }
            }

            if (isa<LoadInst>(I) || isa<StoreInst>(I) || isa<AtomicCmpXchgInst>(I)
                || isa<AtomicRMWInst>(I) || isa<ExtractValueInst>(I) || isa<InsertValueInst>(I)) {
                ++NumDerefInstructions;
            } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                if (auto *Callee = CI->getCalledFunction()) {
                    if (Callee->empty()) {
                        for (unsigned K = 0; K < CI->getNumArgOperands(); ++K) {
                            if (CI->getArgOperand(K)->getType()->isPointerTy()) {
                                ++NumDerefInstructions;
                                break;
                            }
                        }
                    }
                } else {
                    ++NumDerefInstructions;
                }
            }
        }
    }
    outs() << "# instructions: " << NumInstructions
           << ", # ptr instructions: " << NumPointerInstructions
           << ", # deref instructions: " << NumDerefInstructions << "\n";
}
