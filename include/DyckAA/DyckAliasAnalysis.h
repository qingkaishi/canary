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

#ifndef DYCKAA_DYCKALIASANALYSIS_H
#define DYCKAA_DYCKALIASANALYSIS_H

#include <llvm/Pass.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/InlineAsm.h>
#include <set>
#include "DyckAA/DyckCallGraph.h"
#include "DyckAA/DyckGraphEdgeLabel.h"
#include "DyckAA/DyckGraph.h"
#include "DyckAA/DyckVFG.h"

using namespace llvm;

class DyckAliasAnalysis : public ModulePass {
private:
    DyckGraph *CFLGraph;
    DyckCallGraph *DyckCG;
    DyckVFG *VFG;

public:
    static char ID;

    DyckAliasAnalysis();

    ~DyckAliasAnalysis() override;

    bool runOnModule(Module &M) override;

    void getAnalysisUsage(AnalysisUsage &AU) const override;

    const std::set<Value *> *getAliasSet(Value *Ptr) const;

    bool mayAlias(Value *V1, Value *V2) const;

    /// get the call graph
    DyckCallGraph *getCallGraph() const;

    /// get the dyck-cfl graph
    DyckGraph *getDyckGraph() const;

    /// return a value flow graph created based on the alias analysis
    DyckVFG *getOrCreateValueFlowGraph(Module &M);

private:
    /// Three kinds of information will be printed.
    /// 1. Alias Sets will be printed to the console
    /// 2. The relation of Alias Sets will be output into "alias_rel.dot"
    /// 3. The evaluation results will be output into "distribution.log"
    ///     The summary of the evaluation will be printed to the console
    void printAliasSetInformation(Module &M);
};

#endif // DYCKAA_DYCKALIASANALYSIS_H
