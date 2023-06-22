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

#ifndef NCA_NULLCHECKANALYSIS_H
#define NCA_NULLCHECKANALYSIS_H

#include <llvm/IR/Function.h>

using namespace llvm;

class NullCheckAnalysis {
private:
    std::function<bool(Value *, Value *)> mayAlias;

public:
    template<class AliasAnalysis>
    explicit NullCheckAnalysis(AliasAnalysis A) : mayAlias(A) {}

    void run(Function &F);

    void run(Module &M);

    bool mayNull(Value *Ptr, Instruction *Inst);
};

#endif // NCA_NULLCHECKANALYSIS_H
