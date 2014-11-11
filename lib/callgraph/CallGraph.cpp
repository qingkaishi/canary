/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "CallGraph.h"

static cl::opt<bool>
WithEdgeLabels("with-labels", cl::init(false), cl::Hidden,
        cl::desc("Determine whether there are edge lables in the cg."));

void CallGraph::dotCallGraph(const string& mIdentifier) {
    string dotfilename("");
    dotfilename.append(mIdentifier);
    dotfilename.append(".maycg.dot");

    FILE * fout = fopen(dotfilename.data(), "w+");
    fprintf(fout, "digraph maycg {\n");
    
    set<FunctionWrapper*>::iterator fwIt = callgraph_nodes.begin();
    while (fwIt != callgraph_nodes.end()) {
        FunctionWrapper* fw = *fwIt;
        fprintf(fout, "\tf%d[label=\"%s\"]\n", fw->getIndex(), fw->getLLVMFunction()->getName().data());
        fwIt++;
    }

    fwIt = callgraph_nodes.begin();
    while (fwIt != callgraph_nodes.end()) {
        FunctionWrapper* fw = *fwIt;
        set<CommonCall*>* commonCalls = fw->getCommonCallsForCG();
        set<CommonCall*>::iterator comIt = commonCalls->begin();
        while (comIt != commonCalls->end()) {
            CommonCall* cc = *comIt;
            Function * callee = (Function*) cc->calledValue;

            if (wrapped_functions_map.count(callee)) {
                if (WithEdgeLabels) {
                    Value * ci = cc->ret;
                    std::string s;
                    raw_string_ostream rso(s);
                    if (ci != NULL) {
                        rso << *(ci);
                    } else {
                        rso << "Hidden";
                    }
                    string& edgelabel = rso.str();
                    for (unsigned int i = 0; i < edgelabel.length(); i++) {
                        if (edgelabel[i] == '\"') {
                            edgelabel[i] = '`';
                        }

                        if (edgelabel[i] == '\n') {
                            edgelabel[i] = ' ';
                        }
                    }
                    fprintf(fout, "\tf%d->f%d[label=\"%s\"]\n", fw->getIndex(), wrapped_functions_map[callee]->getIndex(), edgelabel.data());
                } else {
                    fprintf(fout, "\tf%d->f%d\n", fw->getIndex(), wrapped_functions_map[callee]->getIndex());
                }
            } else {
                errs() << "ERROR in printCG when print common function calls.\n";
                exit(-1);
            }
            comIt++;
        }

        set<PointerCall*>* fpCallsMap = fw->getPointerCallsForCG();
        set<PointerCall*>::iterator fpIt = fpCallsMap->begin();
        while (fpIt != fpCallsMap->end()) {
            PointerCall* pcall = *fpIt;
            set<Function*>* mayCalled = &((*fpIt)->mayAliasedCallees);

            char * edgeLabelData = NULL;
            if (WithEdgeLabels) {
                std::string s;
                raw_string_ostream rso(s);
                if (pcall->ret != NULL) {
                    rso << *(pcall->ret);
                } else {
                    rso << "Hidden";
                }
                string& edgelabel = rso.str(); // edge label is the call inst
                for (unsigned int i = 0; i < edgelabel.length(); i++) {
                    if (edgelabel[i] == '\"') {
                        edgelabel[i] = '`';
                    }

                    if (edgelabel[i] == '\n') {
                        edgelabel[i] = ' ';
                    }
                }
                edgeLabelData = const_cast<char*> (edgelabel.data());
            }
            set<Function*>::iterator mcIt = mayCalled->begin();
            while (mcIt != mayCalled->end()) {
                Function * mcf = *mcIt;
                if (wrapped_functions_map.count(mcf)) {
                    if (WithEdgeLabels) {
                        fprintf(fout, "\tf%d->f%d[label=\"%s\"]\n", fw->getIndex(), wrapped_functions_map[mcf]->getIndex(), edgeLabelData);
                    } else {
                        fprintf(fout, "\tf%d->f%d\n", fw->getIndex(), wrapped_functions_map[mcf]->getIndex());
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

void CallGraph::printFunctionPointersInformation(const string& mIdentifier) {
    string dotfilename("");
    dotfilename.append(mIdentifier);
    dotfilename.append(".fp.txt");

    FILE * fout = fopen(dotfilename.data(), "w+");

    set<FunctionWrapper*>::iterator fwIt = this->begin();
    while (fwIt != this->end()) {
        FunctionWrapper* fw = *fwIt;

        set<PointerCall*>* fpCallsMap = fw->getPointerCallsForCG();
        set<PointerCall*>::iterator fpIt = fpCallsMap->begin();
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
            set<Function*>* mayCalled = &((*(fpIt))->mayAliasedCallees);
            fprintf(fout, "%zd\n", mayCalled->size()); //number of functions

            // what functions?
            set<Function*>::iterator mcIt = mayCalled->begin();
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
