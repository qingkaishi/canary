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

#include <llvm/ADT/SCCIterator.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include "MRAnalyzer.h"

MRAnalyzer::MRAnalyzer(Module *M, DyckGraph *DG, DyckCallGraph *DCG) : M(M), DG(DG), DCG(DCG) {
}

MRAnalyzer::~MRAnalyzer() = default;

void MRAnalyzer::intraProcedureAnalysis() {

}

void MRAnalyzer::interProcedureAnalysis() {
    // step 1, find scc in call graph & bottom-up analysis, considering each scc as a single node
    // step 2, record the dyck vertices each function (cg node) references and modifies
    // scc iterator: Enumerate the SCCs of a directed graph in reverse topological order
    for (auto It = scc_begin(DCG), E = scc_end(DCG); It != E; ++It) {
        const auto &SCC = *It; // a vector of nodes in the same scc
        runOnSCC(SCC);
    }
}

void MRAnalyzer::runOnSCC(const std::vector<DyckCallGraphNode *> &SCC) {
    std::set<DyckGraphNode *> Refs;
    std::set<DyckGraphNode *> Mods;

    // todo compute a set of dyck nodes reachable from parameters (and returns)
    //  for a scc with multiple functions, we should find the interfaces to the external world
    std::set<DyckGraphNode *> ParReachableNodes;
    std::set<DyckGraphNode *> RetReachableNodes;

    for (auto *CGNode: SCC) {
        // for each instruction,
        // if it refs a node that is reachable from parameters add it to refs
        // if it mods a node that is reachable from parameters add it to mods
        auto *F = CGNode->getLLVMFunction();
        for (auto &I : instructions(F)) {
            for (unsigned K = 0; K < I.getNumOperands(); ++K) {
                auto *Ref = I.getOperand(K);
                auto *RefNode = DG->findDyckVertex(Ref);
                if (!RefNode) continue;
                // check if reachable from parameters
                if (ParReachableNodes.count(RefNode)) Refs.insert(RefNode);
            }
            if (F->onlyReadsMemory()) continue; // a read only function
            if (auto *SI = dyn_cast<StoreInst>(&I)) {
                auto *Ptr = SI->getPointerOperand();
                auto *PtrNode = DG->findDyckVertex(Ptr);
                if (!PtrNode) continue;
                auto *ModNode = PtrNode->getOutVertex(DG->getDereferenceEdgeLabel());
                // check if reachable from parameters and returns
                if (ParReachableNodes.count(ModNode) || RetReachableNodes.count(ModNode)) Mods.insert(ModNode);
            } else {
                // todo other instructions that may revise a memory
            }
        }
    }

    // todo attach Refs and Mods to the functions
}
