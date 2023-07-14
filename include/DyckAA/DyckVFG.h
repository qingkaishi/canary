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

#ifndef DyckAA_DYCKVFG_H
#define DyckAA_DYCKVFG_H

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

using namespace llvm;

class DyckAliasAnalysis;

class DyckVFGNode {

};

class DyckVFG {
public:
    DyckVFG(DyckAliasAnalysis *DAA, Module *M);

    DyckVFG(DyckAliasAnalysis *DAA, Function *F);

    ~DyckVFG();
};

#endif //DyckAA_DYCKVFG_H
