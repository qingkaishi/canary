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
#include "Support/ThreadPool.h"
#include "Support/TimeRecorder.h"


NullCheckAnalysis::~NullCheckAnalysis() {
    for (auto &It: AnalysisMap) delete It.second;
    decltype(AnalysisMap)().swap(AnalysisMap);
}

bool NullCheckAnalysis::mayNull(Value *Ptr, Instruction *Inst) {
    auto It = AnalysisMap.find(Inst->getFunction());
    if (It != AnalysisMap.end())
        return It->second->mayNull(Ptr, Inst);
    else return true;
}

void NullCheckAnalysis::run() {
    // record time
    TimeRecorder TR("Running NullCheckAnalysis");

    // allocate space for each function for thread safety
    for (auto &F: *M) if (!F.empty()) AnalysisMap[&F] = nullptr;

    // run local nca for each function concurrently
    for (auto &F: *M)
        if (!F.empty())
            ThreadPool::get()->enqueue([this, &F]() {
                auto *LNCA = new LocalNullCheckAnalysis(Driver, &F);
                AnalysisMap.at(&F) = LNCA;
                LNCA->run();
            });

    // wait for all tasks to finish
    ThreadPool::get()->wait();
}
