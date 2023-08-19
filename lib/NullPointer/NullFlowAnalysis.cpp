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

#include "DyckAA/DyckValueFlowAnalysis.h"
#include "NullPointer/NullFlowAnalysis.h"
#include "Support/TimeRecorder.h"

char NullFlowAnalysis::ID = 0;
static RegisterPass<NullFlowAnalysis> X("nfa", "null value flow");

NullFlowAnalysis::NullFlowAnalysis() : ModulePass(ID) {
    VFG = nullptr;
}

NullFlowAnalysis::~NullFlowAnalysis() = default;

void NullFlowAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<DyckValueFlowAnalysis>();
}

bool NullFlowAnalysis::runOnModule(Module &M) {
    TimeRecorder DyckVFA("Running NFA");
    auto *VFA = &getAnalysis<DyckValueFlowAnalysis>();
    VFG = VFA->getDyckVFGraph();
    recompute();
    return false;
}

void NullFlowAnalysis::recompute() {
    // todo
}
