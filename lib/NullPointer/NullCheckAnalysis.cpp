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

#include <llvm/IR/Module.h>
#include "NullPointer/LocalNullCheckAnalysis.h"
#include "NullPointer/NullCheckAnalysis.h"
#include "NullPointer/NullFlowAnalysis.h"
#include "Support/ThreadPool.h"
#include "Support/TimeRecorder.h"

static cl::opt<unsigned> Round("nca-round", cl::init(1), cl::Hidden, cl::desc("# rounds"));

char NullCheckAnalysis::ID = 0;
static RegisterPass<NullCheckAnalysis> X("nca", "soundly checking if a pointer may be nullptr.");

NullCheckAnalysis::~NullCheckAnalysis() {
    for (auto &It: AnalysisMap) delete It.second;
    decltype(AnalysisMap)().swap(AnalysisMap);
}

void NullCheckAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<NullFlowAnalysis>();
}

bool NullCheckAnalysis::runOnModule(Module &M) {
    // record time
    TimeRecorder TR("Running NullCheckAnalysis");

    // get the null flow analysis
    auto *NFA = &getAnalysis<NullFlowAnalysis>();

    // allocate space for each function for thread safety
    for (auto &F: M) if (!F.empty()) AnalysisMap[&F] = nullptr;

    unsigned Count = 0;
    do {
        for (auto &F: M) {
            if (F.empty()) continue;
            ThreadPool::get()->enqueue([this, NFA, &F]() {
                auto *&LNCA = AnalysisMap.at(&F);
                if (!LNCA) LNCA = new LocalNullCheckAnalysis(NFA, &F);
                LNCA->run();
            });
        }
        ThreadPool::get()->wait(); // wait for all tasks to finish
    } while (++Count < Round.getValue() && NFA->recompute());

    return false;
}

bool NullCheckAnalysis::mayNull(Value *Ptr, Instruction *Inst) {
    auto It = AnalysisMap.find(Inst->getFunction());
    if (It != AnalysisMap.end())
        return It->second->mayNull(Ptr, Inst);
    else return true;
}
