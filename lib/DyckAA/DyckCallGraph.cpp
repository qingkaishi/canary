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

#include "DyckAA/DyckCallGraph.h"

static cl::opt<bool> WithEdgeLabels("with-labels", cl::init(false), cl::Hidden,
                                    cl::desc("Determine whether there are edge lables in the cg."));

void DyckCallGraph::dotCallGraph(const std::string &ModuleIdentifier) {
    std::string DotFileName;
    DotFileName.append(ModuleIdentifier);
    DotFileName.append(".maycg.dot");

    FILE *FOut = fopen(DotFileName.data(), "w+");
    fprintf(FOut, "digraph maycg {\n");

    auto FWIt = FunctionMap.begin();
    while (FWIt != FunctionMap.end()) {
        DyckCallGraphNode *FW = FWIt->second;
        fprintf(FOut, "\tf%d[label=\"%s\"]\n", FW->getIndex(), FW->getLLVMFunction()->getName().data());
        FWIt++;
    }

    FWIt = FunctionMap.begin();
    while (FWIt != FunctionMap.end()) {
        DyckCallGraphNode *FW = FWIt->second;
        std::set<CommonCall *> *CommonCalls = &(FW->getCommonCalls());
        auto CCIt = CommonCalls->begin();
        while (CCIt != CommonCalls->end()) {
            CommonCall *CC = *CCIt;
            auto *Callee = (Function *) CC->CalledValue;

            if (FunctionMap.count(Callee)) {
                if (WithEdgeLabels) {
                    Value *CI = CC->Inst;
                    std::string S;
                    raw_string_ostream RSO(S);
                    if (CI != nullptr) {
                        RSO << *(CI);
                    } else {
                        RSO << "Hidden";
                    }
                    std::string &EdgeLabelStr = RSO.str();
                    for (char &C: EdgeLabelStr) {
                        if (C == '\"') {
                            C = '`';
                        }

                        if (C == '\n') {
                            C = ' ';
                        }
                    }
                    fprintf(FOut, "\tf%d->f%d[label=\"%s\"]\n", FW->getIndex(), FunctionMap[Callee]->getIndex(),
                            EdgeLabelStr.data());
                } else {
                    fprintf(FOut, "\tf%d->f%d\n", FW->getIndex(), FunctionMap[Callee]->getIndex());
                }
            } else {
                errs() << "ERROR in printCG when print common function calls.\n";
                exit(-1);
            }
            CCIt++;
        }

        std::set<PointerCall *> *FPCallsMap = &(FW->getPointerCalls());
        auto FPIt = FPCallsMap->begin();
        while (FPIt != FPCallsMap->end()) {
            PointerCall *PC = *FPIt;
            std::set<Function *> *MayCalled = &((*FPIt)->MayAliasedCallees);

            char *EdgeLabelData = nullptr;
            if (WithEdgeLabels) {
                std::string S;
                raw_string_ostream RSO(S);
                if (PC->Inst != nullptr) {
                    RSO << *(PC->Inst);
                } else {
                    RSO << "Hidden";
                }
                std::string &EdgeLabelStr = RSO.str(); // edge label is the call inst
                for (char &C: EdgeLabelStr) {
                    if (C == '\"') {
                        C = '`';
                    }

                    if (C == '\n') {
                        C = ' ';
                    }
                }
                EdgeLabelData = const_cast<char *> (EdgeLabelStr.data());
            }
            auto MCIt = MayCalled->begin();
            while (MCIt != MayCalled->end()) {
                Function *MCF = *MCIt;
                if (FunctionMap.count(MCF)) {
                    if (WithEdgeLabels) {
                        fprintf(FOut, "\tf%d->f%d[label=\"%s\"]\n", FW->getIndex(), FunctionMap[MCF]->getIndex(),
                                EdgeLabelData);
                    } else {
                        fprintf(FOut, "\tf%d->f%d\n", FW->getIndex(), FunctionMap[MCF]->getIndex());
                    }
                } else {
                    errs() << "ERROR in printCG when print fp calls.\n";
                    exit(-1);
                }
                MCIt++;
            }
            FPIt++;
        }
        FWIt++;
    }

    fprintf(FOut, "}\n");
    fclose(FOut);
}

void DyckCallGraph::printFunctionPointersInformation(const std::string &ModuleIdentifier) {
    std::string Dotfilename;
    Dotfilename.append(ModuleIdentifier);
    Dotfilename.append(".fp.txt");

    FILE *FOut = fopen(Dotfilename.data(), "w+");

    auto FWIt = this->begin();
    while (FWIt != this->end()) {
        DyckCallGraphNode *FW = FWIt->second;

        std::set<PointerCall *> *FPCallsMap = &(FW->getPointerCalls());
        auto FPIt = FPCallsMap->begin();
        while (FPIt != FPCallsMap->end()) {
            /*Value * callInst = fpIt->first;
            std::string s;
            raw_string_ostream rso(s);
            rso << *(callInst);
            string& edgelabel = rso.str();
            for (unsigned int i = 0; i < edgelabel.length(); i++) {
                if (edgelabel[i] == '\"') {
                    edgelabel[i] = '`';
                }

                if (edgelabel[i] == '\n') {
                    edgelabel[i] = ' ';
                }
            }
            fprintf(fout, "CallInst: %s\n", edgelabel.data()); //call inst
             */
            std::set<Function *> *MayCalled = &((*(FPIt))->MayAliasedCallees);
            fprintf(FOut, "%zd\n", MayCalled->size()); //number of functions

            // what functions?
            auto MCIt = MayCalled->begin();
            while (MCIt != MayCalled->end()) {
                // Function * mcf = *mcIt;
                //fprintf(fout, "%s\n", mcf->getName().data());

                MCIt++;
            }

            //fprintf(fout, "\n");

            FPIt++;
        }


        FWIt++;
    }

    fclose(FOut);
}
