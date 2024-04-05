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

#include "DyckAA/DyckGraphNode.h"
#include "Support/CFG.h"
#include "Support/MapIterators.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

using namespace llvm;

class DyckAliasAnalysis;

class DyckModRefAnalysis;

class Call;

class DyckVFGNode {
  private:
    /// the value this node represents
    Value *V;

    /// labeled edge, 0 - epsilon, pos - call, neg - return
    /// @{
    using EdgeSetTy = std::set<std::pair<DyckVFGNode *, int>>;
    EdgeSetTy Targets;
    EdgeSetTy Sources;
    /// @}

  public:
    explicit DyckVFGNode(Value *V)
        : V(V) {}

    void addTarget(DyckVFGNode *N, int L = 0) {
        assert(N);
        this->Targets.emplace(N, L);
        N->Sources.emplace(this, L);
    }

    Value *getValue() const {
        return V;
    }

    Function *getFunction() const;

    EdgeSetTy::const_iterator begin() const {
        return Targets.begin();
    }

    EdgeSetTy::const_iterator end() const {
        return Targets.end();
    }

    EdgeSetTy::const_iterator in_begin() const {
        return Sources.begin();
    }

    EdgeSetTy::const_iterator in_end() const {
        return Sources.end();
    }
};

class DyckVFG {
  private:
    std::unordered_map<Value *, DyckVFGNode *> ValueNodeMap;

  public:
    DyckVFG(DyckAliasAnalysis *DAA, DyckModRefAnalysis *DMRA, Module *M);

    ~DyckVFG();

    DyckVFGNode *getVFGNode(Value *) const;

    value_iterator<std::unordered_map<Value *, DyckVFGNode *>::iterator> node_begin() {
        return {ValueNodeMap.begin()};
    }

    value_iterator<std::unordered_map<Value *, DyckVFGNode *>::iterator> node_end() {
        return {ValueNodeMap.end()};
    }

  private:
    DyckVFGNode *getOrCreateVFGNode(Value *);

    void connect(DyckAliasAnalysis *DAA, DyckModRefAnalysis *, Call *, Function *, CFG *);

    void buildLocalVFG(DyckAliasAnalysis *DAA, CFG *DMRA, Function *F) const;
    void connectInsertExtractIndirectFlow(std::map<DyckGraphNode *, std::vector<ExtractValueInst *>>,
                                          std::map<DyckGraphNode *, std::vector<InsertValueInst *>>,
                                          std::function<bool(Instruction *, Instruction *)> Reachable,
                                          int CallId);

    void buildLocalVFG(Function &);
};

#endif // DyckAA_DYCKVFG_H
