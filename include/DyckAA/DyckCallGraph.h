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

#ifndef DYCKAA_DYCKCALLGRAPH_H
#define DYCKAA_DYCKCALLGRAPH_H

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CaptureTracking.h>
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/MemoryBuiltins.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include <set>
#include <map>
#include <vector>
#include <cstdio>

#include "DyckAA/DyckCallGraphNode.h"

using namespace llvm;

class DyckCallGraph {
private:
    typedef std::map<Function *, DyckCallGraphNode *> FunctionMapTy;
    FunctionMapTy FunctionMap;

public:
    ~DyckCallGraph() {
        auto It = FunctionMap.begin();
        while (It != FunctionMap.end()) {
            delete (It->second);
            It++;
        }
        FunctionMap.clear();
    }

public:

    FunctionMapTy::iterator begin() {
        return FunctionMap.begin();
    }

    FunctionMapTy::iterator end() {
        return FunctionMap.end();
    }

    size_t size() const {
        return FunctionMap.size();
    }

    DyckCallGraphNode *getOrInsertFunction(Function *Func) {
        DyckCallGraphNode *Parent = nullptr;
        if (!FunctionMap.count(Func)) {
            Parent = new DyckCallGraphNode(Func);
            FunctionMap.insert(std::pair<Function *, DyckCallGraphNode *>(Func, Parent));
        } else {
            Parent = FunctionMap[Func];
        }
        return Parent;
    }

    void dotCallGraph(const std::string &ModuleIdentifier);

    void printFunctionPointersInformation(const std::string &ModuleIdentifier);

};


#endif // DYCKAA_DYCKCALLGRAPH_H

