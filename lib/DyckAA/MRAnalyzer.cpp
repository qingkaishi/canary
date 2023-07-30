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
#include "MRAnalyzer.h"

MRAnalyzer::MRAnalyzer(Module *M, DyckGraph *DG, DyckCallGraph *DCG) : M(M), DG(DG), DCG(DCG) {
}

MRAnalyzer::~MRAnalyzer() = default;

void MRAnalyzer::intraProcedureAnalysis() {

}

void MRAnalyzer::interProcedureAnalysis() {
    // todo
    // step 1, find scc in call graph & bottom-up analysis, considering each scc as a single node
    // scc iterator: Enumerate the SCCs of a directed graph in reverse topological order
    for (auto It = scc_begin(DCG), E = scc_end(DCG); It != E; ++It) {
        const auto &SCC = *It; // a vector of nodes in the same scc
    }

    // step 2, record the dyck vertices each function (cg node) references and modifies
}
