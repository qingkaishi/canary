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

#ifndef DYCKAA_MRANALYZER_H
#define DYCKAA_MRANALYZER_H

#include <llvm/Pass.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/InlineAsm.h>
#include <map>
#include <set>
#include <unordered_map>

#include "DyckAA/DyckCallGraph.h"
#include "DyckAA/DyckGraph.h"

using namespace llvm;

class MRAnalyzer {
private:
    Module *M;
    DyckGraph *DG;
    DyckCallGraph *DCG;

public:
    MRAnalyzer(Module *, DyckGraph *, DyckCallGraph *);

    ~MRAnalyzer();

    void intraProcedureAnalysis();

    void interProcedureAnalysis();
};

#endif //DYCKAA_MRANALYZER_H
