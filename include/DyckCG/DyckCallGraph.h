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

#ifndef DYCKCALLGRAPH_H
#define DYCKCALLGRAPH_H

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include "DyckCallGraphNode.h"

#include <set>
#include <map>
#include <vector>
#include <cstdio>


using namespace llvm;
using namespace std;

class DyckCallGraph {
private:
    typedef std::map<Function *, DyckCallGraphNode *> FunctionMapTy;
    FunctionMapTy FunctionMap;

public:
    ~DyckCallGraph() {
        auto it = FunctionMap.begin();
        while (it != FunctionMap.end()) {
            delete (it->second);
            it++;
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

    DyckCallGraphNode *getOrInsertFunction(Function *f) {
        DyckCallGraphNode *parent = nullptr;
        if (!FunctionMap.count(f)) {
            parent = new DyckCallGraphNode(f);
            FunctionMap.insert(pair<Function *, DyckCallGraphNode *>(f, parent));
        } else {
            parent = FunctionMap[f];
        }
        return parent;
    }

    void dotCallGraph(const string &mIdentifier);

    void printFunctionPointersInformation(const string &mIdentifier);

};


#endif    /* DYCKCALLGRAPH_H */

