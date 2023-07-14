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

#include <llvm/IR/Dominators.h>
#include "DyckAA/DyckAliasAnalysis.h"
#include "NullPointer/LocalNullCheckAnalysis.h"
#include "NullPointer/NullBoosterPass.h"
#include "Support/ThreadPool.h"
#include "Support/TimeRecorder.h"

char NullBoosterPass::ID = 0;
static RegisterPass<NullBoosterPass> X("bona", "soundly checking if a pointer may be nullptr.");

NullBoosterPass::NullBoosterPass() : ModulePass(ID) {}

NullBoosterPass::~NullBoosterPass() = default;

void NullBoosterPass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<DyckAliasAnalysis>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.setPreservesAll();
}

bool NullBoosterPass::runOnModule(Module &M) {
    TimeRecorder TR("NCA");
    for (auto &F: M)
        if (!F.empty()) {
            ThreadPool::get()->enqueue([this, &F]() {
                LocalNullCheckAnalysis(this, &F).run();
            });
        }
    ThreadPool::get()->waitAll();
    return false;
}
