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

#include "DyckAA/DyckVFG.h"

DyckVFG::DyckVFG(DyckAliasAnalysis *DAA, Module *M) {
    // create a VFG for each function
    std::map<Function *, DyckVFG *> LocalVFGMap;
    for (auto &F: *M) {
        if (F.empty()) continue;
        LocalVFGMap[&F] = new DyckVFG(DAA, &F);
    }

    // todo connect local VFGs, delete local VFGs
}

DyckVFG::DyckVFG(DyckAliasAnalysis *DAA, Function *F) {
    // todo
}

DyckVFG::~DyckVFG() {
    for (auto &It: ValueNodeMap) delete It.second;
}

DyckVFGNode *DyckVFG::getVFGNode(Value *V) {
    auto It = ValueNodeMap.find(V);
    if (It == ValueNodeMap.end()) return nullptr;
    return It->second;
}

DyckVFGNode *DyckVFG::getOrCreateVFGNode(Value *V) {
    auto It = ValueNodeMap.find(V);
    if (It == ValueNodeMap.end()) {
        auto *Ret = new DyckVFGNode(V);
        ValueNodeMap[V] = Ret;
        return Ret;
    }
    return It->second;
}
