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

#include <cstdio>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/LineIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

#include "AAAnalyzer.h"
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckCallGraph.h"
#include "DyckAA/DyckGraph.h"
#include "Support/API.h"
#include "Support/RecursiveTimer.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"

static cl::opt<bool> PrintAliasSetInformation("print-alias-set-info", cl::init(false), cl::Hidden,
                                              cl::desc("Output alias sets and their relations"));

static cl::opt<bool> DotCallGraph("dot-dyck-callgraph", cl::init(false), cl::Hidden,
                                  cl::desc("Calculate the program's call graph and output into a \"dot\" file."));

static cl::opt<bool> CountFP("count-fp", cl::init(false), cl::Hidden,
                             cl::desc("Calculate how many functions a function pointer may point to."));

bool PrintCSourceFunctions = false;

static cl::opt<bool, true> PrintCSourceFunctionsFlag(
    "print-c-source-functions", cl::Hidden,
    cl::desc("Works for collecting the functions which may return dynamic allocated memory pointers."),
    cl::location(PrintCSourceFunctions));

static cl::opt<std::string> CSourceFunctions("c-source-functions", cl::value_desc("filename"), cl::Hidden,
                                             cl::desc("path to file which contains the names of c source functions."));

char DyckAliasAnalysis::ID = 0;
static RegisterPass<DyckAliasAnalysis> X("dyckaa", "a unification based alias analysis");

DyckAliasAnalysis::DyckAliasAnalysis()
    : ModulePass(ID) {
    DyckPTG = new DyckGraph;
    DyckCG = new DyckCallGraph;
}

DyckAliasAnalysis::~DyckAliasAnalysis() {
    delete DyckCG;
    delete DyckPTG;
}

void DyckAliasAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
}

const std::set<Value *> *DyckAliasAnalysis::getAliasSet(Value *Ptr) const {
    DyckGraphNode *V = DyckPTG->retrieveDyckVertex(Ptr).first;
    return (const std::set<Value *> *)V->getEquivalentSet();
}

bool DyckAliasAnalysis::mayAlias(Value *V1, Value *V2) const {
    return getAliasSet(V1)->count(V2);
}

bool DyckAliasAnalysis::mayNull(Value *V) const {
    auto *DyckNode = DyckPTG->findDyckVertex(V);
    if (!DyckNode)
        return false;
    return DyckNode->containsNull();
}

DyckCallGraph *DyckAliasAnalysis::getDyckCallGraph() const {
    return DyckCG;
}

DyckGraph *DyckAliasAnalysis::getDyckGraph() const {
    return DyckPTG;
}

bool DyckAliasAnalysis::runOnModule(Module &M) {
    RecursiveTimer DyckAA("Running DyckAA");
    readCSourceFunctions();
    // alias analysis
    AAAnalyzer AA(&M, DyckPTG, DyckCG);
    AA.intraProcedureAnalysis();
    AA.interProcedureAnalysis();

    // a post-processing procedure
    for (auto *DyckNode : DyckPTG->getVertices()) {
        auto *AliasSet = (const std::set<Value *> *)DyckNode->getEquivalentSet();
        if (!AliasSet)
            continue;
        for (auto *V : *AliasSet) {
            if (!isa<ConstantPointerNull>(V))
                continue;
            DyckNode->setContainsNull();
            break;
        }
    }

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
        this->printAliasSetInformation();
        outs() << "Done!\n\n";
    }
    if (PrintCSourceFunctions) {
        this->printCSourceFunctions();
    }

    DEBUG_WITH_TYPE("validate-dyckgraph", DyckPTG->validation(__FILE__, __LINE__));

    return false;
}

void DyckAliasAnalysis::printAliasSetInformation() {
    /*if (InterAAEval)*/
    {
        std::set<DyckGraphNode *> &AllReps = DyckPTG->getVertices();

        outs() << "Printing distribution.log... ";
        outs().flush();
        FILE *Log = fopen("distribution.log", "w+");

        std::vector<unsigned long> AliasSetSizes;
        unsigned TotalSize = 0;
        auto It = AllReps.begin();
        while (It != AllReps.end()) {
            std::set<llvm::Value *> *AliasSet = (*It)->getEquivalentSet();

            unsigned long Size = 0;

            auto AsIt = AliasSet->begin();
            while (AsIt != AliasSet->end()) {
                auto *Val = (Value *)(*AsIt);
                if (Val->getType()->isPointerTy())
                    Size++;
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
        double PairNum = (((double)TotalSize - 1) / 2) * TotalSize;

        unsigned NoAliasNum = 0;
        for (unsigned K = 0; K < AliasSetSizes.size(); K++) {
            unsigned KSize = AliasSetSizes[K];
            for (unsigned J = K + 1; J < AliasSetSizes.size(); J++) {
                NoAliasNum = NoAliasNum + KSize * AliasSetSizes[J];
            }
        }
        // errs() << noAliasNum << "\n";

        double PercentOfNoAlias = NoAliasNum / (double)PairNum * 100;

        fclose(Log);
        outs() << "Done!\n";

        outs() << "===== Alias Analysis Evaluator Report =====\n";
        outs() << "   " << PairNum << " Total Alias Queries Performed\n";
        outs() << "   " << NoAliasNum << " no alias responses (" << (unsigned long)PercentOfNoAlias << "%)\n\n";
    }

    /*if (DotAliasSet) */
    {
        outs() << "Printing alias_rel.dot... ";
        outs().flush();

        FILE *AliasRel = fopen("alias_rel.dot", "w");
        fprintf(AliasRel, "digraph rel{\n");

        std::map<DyckGraphNode *, int> TheMap;
        int Idx = 0;
        std::set<DyckGraphNode *> &Reps = DyckPTG->getVertices();
        auto RepIt = Reps.begin();
        while (RepIt != Reps.end()) {
            Idx++;
            std::set<llvm::Value *> *EqSets = (*RepIt)->getEquivalentSet();
            std::string LabelDesc;
            raw_string_ostream LabelDescBuilder(LabelDesc);
            std::string LabelColor("black");
            if ((*RepIt)->isAliasOfHeapAlloc()) {
                LabelColor = "red";
            }
            for (auto Val : *EqSets) {
                assert(Val != nullptr && "Error: val is null in an equiv set!");
                if (isa<Function>(Val)) {
                    LabelDescBuilder << ((Function *)Val)->getName() << "\n";
                }
                else {
                    LabelDescBuilder << *Val << "\n";
                }
            }

            auto LabelDescIdx = LabelDesc.find('\"');
            while (LabelDescIdx != std::string::npos) {
                LabelDesc.insert(LabelDescIdx, "\\");
                LabelDescIdx = LabelDesc.find('\"', LabelDescIdx + 2);
            }
            fprintf(AliasRel, "a%d[color=\"%s\"label=\"%s\"];\n", Idx, LabelColor.c_str(), LabelDesc.c_str());
            TheMap.insert(std::pair<DyckGraphNode *, int>(*RepIt, Idx));
            RepIt++;
        }

        RepIt = Reps.begin();
        while (RepIt != Reps.end()) {
            DyckGraphNode *DGN = *RepIt;
            std::map<DyckGraphEdgeLabel *, std::set<DyckGraphNode *>> &OutVs = DGN->getOutVertices();

            auto OvIt = OutVs.begin();
            while (OvIt != OutVs.end()) {
                auto *Label = (DyckGraphEdgeLabel *)OvIt->first;
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
        std::set<DyckGraphNode *> &Reps = DyckPTG->getVertices();
        auto RepsIt = Reps.begin();
        while (RepsIt != Reps.end()) {
            Idx++;
            DyckGraphNode *Rep = *RepsIt;

            std::set<llvm::Value *> *ESet = Rep->getEquivalentSet();
            auto EIt = ESet->begin();
            while (EIt != ESet->end()) {
                auto *Val = (Value *)((*EIt));
                assert(Val != nullptr && "Error: val is null in an equiv set!");
                Log << "[" << Idx << "]";

                if (isa<Function>(Val)) {
                    Log << ((Function *)Val)->getName() << "\n";
                }
                else if (auto Inst = dyn_cast<Instruction>(Val)) {

                    Log << *Inst << "\n";
                }
                else if (auto Arg = dyn_cast<Argument>(Val)) {
                    Log << Arg->getParent()->getName() << *Arg << "\n";
                }
                else {
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

void DyckAliasAnalysis::printCSourceFunctions() {

    std::error_code EC;
    raw_fd_ostream c_marked_functions("c_mark_functions.log", EC);
    for (auto &CGNodeIt : *DyckCG) {
        DyckCallGraphNode *CGNode = CGNodeIt.second;
        // overlook fake function node and declared fucntion nodes .
        if (!CGNodeIt.first || CGNodeIt.first->isDeclaration()) {
            continue;
        }
        for (auto Ret : CGNode->getReturns()) {
            auto *RetInst = (ReturnInst *)Ret;
            // if one of return operands is alias of heap allocated memory,
            // the output the function's name.
            DyckGraphNode *GNode = DyckPTG->retrieveDyckVertex(Ret).first;
            if (GNode && GNode->isAliasOfHeapAlloc()) {
                c_marked_functions << CGNodeIt.first->getName() << "\n";
                break;
            }
        }
    }
}

void DyckAliasAnalysis::readCSourceFunctions() {
    outs() << CSourceFunctions;
    if (CSourceFunctions.getValue().empty())
        return;
    outs() << "<<<<<<<<<< read function names from file"
           << "\n";
    auto CSFBuffer = MemoryBuffer::getFile(CSourceFunctions.getValue());
    std::set<std::string> CSFName;
    for (auto It = line_iterator(*CSFBuffer->get()); !It.is_at_eof(); It++) {
        CSFName.insert((*It).str());
    }
    API::HeapAllocFunctions.insert(CSFName.begin(), CSFName.end());
}