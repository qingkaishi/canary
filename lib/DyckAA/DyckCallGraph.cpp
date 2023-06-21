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

static cl::opt<bool>
        WithEdgeLabels("with-labels", cl::init(false), cl::Hidden,
                       cl::desc("Determine whether there are edge lables in the cg."));

void DyckCallGraph::dotCallGraph(const std::string &mIdentifier) {
    std::string dotfilename;
    dotfilename.append(mIdentifier);
    dotfilename.append(".maycg.dot");

    FILE *fout = fopen(dotfilename.data(), "w+");
    fprintf(fout, "digraph maycg {\n");

    auto fwIt = FunctionMap.begin();
    while (fwIt != FunctionMap.end()) {
        DyckCallGraphNode *fw = fwIt->second;
        fprintf(fout, "\tf%d[label=\"%s\"]\n", fw->getIndex(), fw->getLLVMFunction()->getName().data());
        fwIt++;
    }

    fwIt = FunctionMap.begin();
    while (fwIt != FunctionMap.end()) {
        DyckCallGraphNode *fw = fwIt->second;
        std::set<CommonCall *> *commonCalls = &(fw->getCommonCalls());
        auto comIt = commonCalls->begin();
        while (comIt != commonCalls->end()) {
            CommonCall *cc = *comIt;
            auto *callee = (Function *) cc->calledValue;

            if (FunctionMap.count(callee)) {
                if (WithEdgeLabels) {
                    Value *ci = cc->instruction;
                    std::string s;
                    raw_string_ostream rso(s);
                    if (ci != nullptr) {
                        rso << *(ci);
                    } else {
                        rso << "Hidden";
                    }
                    std::string &edgelabel = rso.str();
                    for (char &i: edgelabel) {
                        if (i == '\"') {
                            i = '`';
                        }

                        if (i == '\n') {
                            i = ' ';
                        }
                    }
                    fprintf(fout, "\tf%d->f%d[label=\"%s\"]\n", fw->getIndex(), FunctionMap[callee]->getIndex(),
                            edgelabel.data());
                } else {
                    fprintf(fout, "\tf%d->f%d\n", fw->getIndex(), FunctionMap[callee]->getIndex());
                }
            } else {
                errs() << "ERROR in printCG when print common function calls.\n";
                exit(-1);
            }
            comIt++;
        }

        std::set<PointerCall *> *fpCallsMap = &(fw->getPointerCalls());
        auto fpIt = fpCallsMap->begin();
        while (fpIt != fpCallsMap->end()) {
            PointerCall *pcall = *fpIt;
            std::set<Function *> *mayCalled = &((*fpIt)->mayAliasedCallees);

            char *edgeLabelData = nullptr;
            if (WithEdgeLabels) {
                std::string s;
                raw_string_ostream rso(s);
                if (pcall->instruction != nullptr) {
                    rso << *(pcall->instruction);
                } else {
                    rso << "Hidden";
                }
                std::string &edgelabel = rso.str(); // edge label is the call inst
                for (char &i: edgelabel) {
                    if (i == '\"') {
                        i = '`';
                    }

                    if (i == '\n') {
                        i = ' ';
                    }
                }
                edgeLabelData = const_cast<char *> (edgelabel.data());
            }
            auto mcIt = mayCalled->begin();
            while (mcIt != mayCalled->end()) {
                Function *mcf = *mcIt;
                if (FunctionMap.count(mcf)) {
                    if (WithEdgeLabels) {
                        fprintf(fout, "\tf%d->f%d[label=\"%s\"]\n", fw->getIndex(), FunctionMap[mcf]->getIndex(),
                                edgeLabelData);
                    } else {
                        fprintf(fout, "\tf%d->f%d\n", fw->getIndex(), FunctionMap[mcf]->getIndex());
                    }
                } else {
                    errs() << "ERROR in printCG when print fp calls.\n";
                    exit(-1);
                }

                mcIt++;
            }

            fpIt++;
        }


        fwIt++;
    }

    fprintf(fout, "}\n");
    fclose(fout);
}

void DyckCallGraph::printFunctionPointersInformation(const std::string &mIdentifier) {
    std::string dotfilename;
    dotfilename.append(mIdentifier);
    dotfilename.append(".fp.txt");

    FILE *fout = fopen(dotfilename.data(), "w+");

    auto fwIt = this->begin();
    while (fwIt != this->end()) {
        DyckCallGraphNode *fw = fwIt->second;

        std::set<PointerCall *> *fpCallsMap = &(fw->getPointerCalls());
        auto fpIt = fpCallsMap->begin();
        while (fpIt != fpCallsMap->end()) {
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
            std::set<Function *> *mayCalled = &((*(fpIt))->mayAliasedCallees);
            fprintf(fout, "%zd\n", mayCalled->size()); //number of functions

            // what functions?
            auto mcIt = mayCalled->begin();
            while (mcIt != mayCalled->end()) {
                // Function * mcf = *mcIt;
                //fprintf(fout, "%s\n", mcf->getName().data());

                mcIt++;
            }

            //fprintf(fout, "\n");

            fpIt++;
        }


        fwIt++;
    }

    fclose(fout);
}
