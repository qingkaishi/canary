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

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <cstdio>
#include <stack>

#include "AAAnalyzer.h"
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckCallGraph.h"
#include "MRAnalyzer.h"
#include "Support/TimeRecorder.h"

static cl::opt<bool> PrintAliasSetInformation("print-alias-set-info", cl::init(false), cl::Hidden,
                                              cl::desc("Output alias sets and their relations"));

static cl::opt<bool> DotCallGraph("dot-dyck-callgraph", cl::init(false), cl::Hidden,
                                  cl::desc("Calculate the program's call graph and output into a \"dot\" file."));

static cl::opt<bool> CountFP("count-fp", cl::init(false), cl::Hidden,
                             cl::desc("Calculate how many functions a function pointer may point to."));

char DyckAliasAnalysis::ID = 0;
static RegisterPass<DyckAliasAnalysis> X("dyckaa", "a unification based alias analysis");

DyckAliasAnalysis::DyckAliasAnalysis() : ModulePass(ID) {
    VFG = nullptr;
    CFLGraph = new DyckGraph;
    DyckCG = new DyckCallGraph;
}

DyckAliasAnalysis::~DyckAliasAnalysis() {
    delete VFG;
    delete DyckCG;
    delete CFLGraph;
}

void DyckAliasAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
}

const std::set<Value *> *DyckAliasAnalysis::getAliasSet(Value *Ptr) const {
    DyckGraphNode *V = CFLGraph->retrieveDyckVertex(Ptr).first;
    return (const std::set<Value *> *) V->getEquivalentSet();
}

bool DyckAliasAnalysis::mayAlias(Value *V1, Value *V2) const {
    return getAliasSet(V1)->count(V2);
}

DyckCallGraph *DyckAliasAnalysis::getCallGraph() const {
    return DyckCG;
}

DyckGraph *DyckAliasAnalysis::getDyckGraph() const {
    return CFLGraph;
}

bool DyckAliasAnalysis::runOnModule(Module &M) {
    TimeRecorder DyckAA("Running DyckAA");

    // alias analysis
    AAAnalyzer AA(&M, CFLGraph, DyckCG);
    AA.intraProcedureAnalysis();
    AA.interProcedureAnalysis();

    // mod/ref analysis
    MRAnalyzer MR(&M, CFLGraph, DyckCG);
    MR.intraProcedureAnalysis();
    MR.interProcedureAnalysis();

    /* call graph */
    if (DotCallGraph) {
        outs() << "Printing call graph...\n";
        DyckCG->dotCallGraph(M.getModuleIdentifier());
        outs() << "Done!\n\n";
    }

    if (CountFP) {
        outs() << "Printing function pointer information...\n";
        DyckCG->printFunctionPointersInformation(M.getModuleIdentifier());
        outs() << "Done!\n\n";
    }

    if (PrintAliasSetInformation) {
        outs() << "Printing alias set information...\n";
        this->printAliasSetInformation(M);
        outs() << "Done!\n\n";
    }

    DEBUG_WITH_TYPE("validate-dyckgraph", CFLGraph->validation(__FILE__, __LINE__));

    return false;
}

void DyckAliasAnalysis::printAliasSetInformation(Module &M) {
    /*if (InterAAEval)*/
    {
        std::set<DyckGraphNode *> &Allreps = CFLGraph->getVertices();

        outs() << "Printing distribution.log... ";
        outs().flush();
        FILE *Log = fopen("distribution.log", "w+");

        std::vector<unsigned long> AliasSetSizes;
        unsigned TotalSize = 0;
        auto It = Allreps.begin();
        while (It != Allreps.end()) {
            std::set<void *> *Aliasset = (*It)->getEquivalentSet();

            unsigned long Size = 0;

            auto AsIt = Aliasset->begin();
            while (AsIt != Aliasset->end()) {
                Value *Val = ((Value *) (*AsIt));
                if (Val->getType()->isPointerTy()) {
                    Size++;
                }

                AsIt++;
            }

            if (Size != 0) {
                TotalSize = TotalSize + Size;
                AliasSetSizes.push_back(Size);
                fprintf(Log, "%lu\n", Size);
            }

            It++;
        }
        errs() << TotalSize << "\n";
        double PairNum = (((double) TotalSize - 1) / 2) * TotalSize;

        unsigned NoAliasNum = 0;
        for (unsigned K = 0; K < AliasSetSizes.size(); K++) {
            unsigned KSize = AliasSetSizes[K];
            for (unsigned J = K + 1; J < AliasSetSizes.size(); J++) {
                NoAliasNum = NoAliasNum + KSize * AliasSetSizes[J];
            }
        }
        //errs() << noAliasNum << "\n";

        double PercentOfNoAlias = NoAliasNum / (double) PairNum * 100;

        fclose(Log);
        outs() << "Done!\n";

        outs() << "===== Alias Analysis Evaluator Report =====\n";
        outs() << "   " << PairNum << " Total Alias Queries Performed\n";
        outs() << "   " << NoAliasNum << " no alias responses (" << (unsigned long) PercentOfNoAlias << "%)\n\n";
    }

    /*if (DotAliasSet) */
    {
        outs() << "Printing alias_rel.dot... ";
        outs().flush();

        FILE *AliasRel = fopen("alias_rel.dot", "w");
        fprintf(AliasRel, "digraph rel{\n");

        std::map<DyckGraphNode *, int> TheMap;
        int Idx = 0;
        std::set<DyckGraphNode *> &Reps = CFLGraph->getVertices();
        auto RepIt = Reps.begin();
        while (RepIt != Reps.end()) {
            Idx++;
            fprintf(AliasRel, "a%d[label=%d];\n", Idx, Idx);
            TheMap.insert(std::pair<DyckGraphNode *, int>(*RepIt, Idx));
            RepIt++;
        }

        RepIt = Reps.begin();
        while (RepIt != Reps.end()) {
            DyckGraphNode *DGN = *RepIt;
            std::map<void *, std::set<DyckGraphNode *>> &OutVs = DGN->getOutVertices();

            auto OvIt = OutVs.begin();
            while (OvIt != OutVs.end()) {
                auto *Label = (DyckGraphEdgeLabel *) OvIt->first;
                std::set<DyckGraphNode *> *oVs = &OvIt->second;

                auto OIt = oVs->begin();
                while (OIt != oVs->end()) {
                    DyckGraphNode *Rep1 = DGN;
                    DyckGraphNode *Rep2 = (*OIt);

                    assert(TheMap.count(Rep1) && "ERROR in DotAliasSet (1)\n");
                    assert(TheMap.count(Rep2) && "ERROR in DotAliasSet (2)\n");

                    int Idx1 = TheMap[Rep1];
                    int Idx2 = TheMap[Rep2];
                    fprintf(AliasRel, "a%d->a%d[label=\"%s\"];\n", Idx1, Idx2, Label->getEdgeLabelDescription().data());

                    OIt++;
                }

                OvIt++;
            }

            TheMap.insert(std::pair<DyckGraphNode *, int>(*RepIt, Idx));
            RepIt++;
        }

        fprintf(AliasRel, "}\n");
        fclose(AliasRel);
        outs() << "Done!\n";
    }

    /*if (OutputAliasSet)*/
    {
        outs() << "Printing alias_sets.log... ";
        outs().flush();

        std::error_code EC;
        raw_fd_ostream Log("alias_sets.log", EC);

        Log << "================= Alias Sets ==================\n";
        Log << "===== {.} means pthread escaped alias set =====\n";

        int Idx = 0;
        std::set<DyckGraphNode *> &Reps = CFLGraph->getVertices();
        auto RepsIt = Reps.begin();
        while (RepsIt != Reps.end()) {
            Idx++;
            DyckGraphNode *Rep = *RepsIt;

            std::set<void *> *ESet = Rep->getEquivalentSet();
            auto EIt = ESet->begin();
            while (EIt != ESet->end()) {
                auto *Val = (Value *) ((*EIt));
                assert(Val != nullptr && "Error: val is null in an equiv set!");
                Log << "[" << Idx << "]";

                if (isa<Function>(Val)) {
                    Log << ((Function *) Val)->getName() << "\n";
                } else {
                    Log << *Val << "\n";
                }
                EIt++;
            }
            RepsIt++;
            Log << "\n------------------------------\n";
        }

        Log.flush();
        Log.close();
        outs() << "Done! \n";
    }
}

DyckVFG *DyckAliasAnalysis::getOrCreateValueFlowGraph(Module &M) {
    if (!VFG) VFG = new DyckVFG(this, &M);
    return VFG;
}
