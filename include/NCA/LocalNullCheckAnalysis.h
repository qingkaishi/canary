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

#ifndef NCA_LOCALNULLCHECKANALYSIS_H
#define NCA_LOCALNULLCHECKANALYSIS_H

#include <llvm/ADT/BitVector.h>
#include <llvm/IR/Instruction.h>
#include <unordered_map>

using namespace llvm;

typedef std::pair<Instruction *, unsigned> Edge;

class LocalNullCheckAnalysis {
private:
    /// map an instruction to a mask, if ith bit of mask is set, it must not be null pointer
    std::unordered_map<Instruction *, int32_t> InstNonNullMap;

    ///
    std::unordered_map<Value *, size_t> PtrIDMap;

    ///
    std::vector<Value *> IDPtrMap;

    ///
    std::map<Edge, BitVector> DataflowFacts;

    /// the function we analyze
    Function *F;

public:
    explicit LocalNullCheckAnalysis(Function *F);

    /// \p Ptr must be an operand of \p Inst
    /// return true if \p Ptr at \p Inst may be a null pointer
    bool mayNull(Value *Ptr, Instruction *Inst);

    void run();

private:
    void nca();

    void merge(std::vector<Edge> &, BitVector &);

    void transfer(Edge, const BitVector &, BitVector &);

    void tag();

    void init();

    bool nonnull(Value *);
};

#endif //NCA_LOCALNULLCHECKANALYSIS_H
