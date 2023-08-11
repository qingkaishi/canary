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
#include <map>
#include <set>
#include <unordered_map>
#include "Support/CFG.h"

using namespace llvm;

class DyckAliasAnalysis;
class DyckModRefAnalysis;

class DyckVFGNode {
private:
    Value *V;

    std::set<DyckVFGNode *> Targets;

public:
    explicit DyckVFGNode(Value *V) : V(V) {}

    void addTarget(DyckVFGNode *N) { Targets.insert(N); }

    Value *getValue() const { return V; }

    std::set<DyckVFGNode *>::const_iterator begin() const { return Targets.begin(); }

    std::set<DyckVFGNode *>::const_iterator end() const { return Targets.end(); }
};

class DyckVFG {
private:
    std::unordered_map<Value *, DyckVFGNode *> ValueNodeMap;

public:
    DyckVFG(DyckAliasAnalysis *DAA, DyckModRefAnalysis* DMRA, Module *M);

    ~DyckVFG();

    DyckVFGNode *getVFGNode(Value *) const;

private:
    DyckVFGNode *getOrCreateVFGNode(Value *);

    void connect(DyckModRefAnalysis *, Call*, Function*, CFG *);

    void buildLocalVFG(DyckAliasAnalysis *DAA, CFG* DMRA, Function *F);
};

#endif //DyckAA_DYCKVFG_H
