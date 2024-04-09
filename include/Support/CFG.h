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

#ifndef SUPPORT_CFG_H
#define SUPPORT_CFG_H

#include <bitset>
#include <llvm/ADT/BitVector.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <vector>

using namespace llvm;

class CFG {
private:
    typedef BitVector ReachableVec;
    ReachableVec AnalyzedVec;
    ReachableVec *ReachableVecPtr;

    /// ID mapping
    std::vector<BasicBlock *> ID2BB;
    std::map<BasicBlock *, int> BB2ID;

public:
    explicit CFG(Function *);

    ~CFG();

    bool reachable(BasicBlock *, BasicBlock *);

    bool reachable(Instruction *, Instruction *);

private:
    void analyze(BasicBlock *);
};

typedef std::shared_ptr<CFG> CFGRef;

#endif //SUPPORT_CFG_H
