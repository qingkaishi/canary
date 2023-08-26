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

#include <llvm/IR/InstIterator.h>
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckValueFlowAnalysis.h"
#include "NullPointer/NullFlowAnalysis.h"
#include "Support/API.h"
#include "Support/RecursiveTimer.h"

char NullFlowAnalysis::ID = 0;
static RegisterPass<NullFlowAnalysis> X("nfa", "null value flow");

NullFlowAnalysis::NullFlowAnalysis() : ModulePass(ID), VFG(nullptr) {
}

NullFlowAnalysis::~NullFlowAnalysis() = default;

void NullFlowAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<DyckValueFlowAnalysis>();
    AU.addRequired<DyckAliasAnalysis>();
}

bool NullFlowAnalysis::runOnModule(Module &M) {
    RecursiveTimer DyckVFA("Running NFA");
    auto *VFA = &getAnalysis<DyckValueFlowAnalysis>();
    VFG = VFA->getDyckVFGraph();
    auto *DAA = &getAnalysis<DyckAliasAnalysis>();

    // init non-null nodes and edges, and then call recompute
    auto MustNotNull = [DAA](Value *V) -> bool {
        V = V->stripPointerCastsAndAliases();
        if (isa<GlobalValue>(V)) return true;
        if (auto CI = dyn_cast<Instruction>(V))
            return API::isMemoryAllocate(CI);
        return !DAA->mayNull(V);
    };
    for (auto &F : M) {
        for (auto &I : instructions(&F)) {
            if (MustNotNull(&I)) {
                if (auto INode = VFG->getVFGNode(&I)) {
                    NonNullNodes.insert(INode);
                    NonNullEdges.emplace(INode, nullptr);
                }
            }
            for (unsigned K = 0; K < I.getNumOperands(); ++K) {
                auto *Op = I.getOperand(K);
                if (MustNotNull(Op)) {
                    if (auto OpNode = VFG->getVFGNode(Op)) {
                        NonNullNodes.insert(OpNode);
                        NonNullEdges.emplace(OpNode, nullptr);
                    }
                }
            }
        }
    }

    recompute();
    return false;
}

bool NullFlowAnalysis::recompute() {
    std::set<std::pair<DyckVFGNode *, DyckVFGNode *>> WorkingNonNulLEdges;
    WorkingNonNulLEdges.swap(NonNullEdges); // clear NonNulLEdges for next round

    // todo
    //  1. find all (new, to avoid duplicate computation) non-null vfg-edges;
    //  2. propagate all (new) non-null vfg-edges
    //  (we do not use nodes, because a node may not be null via only one edge)
    return false;
}

void NullFlowAnalysis::add(Value *V1, Value *V2) {
    if (!V1) return;
    auto *V1N = VFG->getVFGNode(V1);
    if (!V1N) return;
    DyckVFGNode *V2N = nullptr;
    if (V2) {
        V2N = VFG->getVFGNode(V2);
        if (!V2N) return;
    }
    NonNullEdges.emplace(V1N, V2N);
}

bool NullFlowAnalysis::notNull(Value *V) const {
    assert(V);
    auto *N = VFG->getVFGNode(V);
    if (!N) return true;
    return NonNullNodes.count(N);
}
