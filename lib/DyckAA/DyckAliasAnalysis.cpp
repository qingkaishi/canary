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

#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckCG/DyckCallGraph.h"

static cl::opt<bool> PrintAliasSetInformation("print-alias-set-info", cl::init(false), cl::Hidden,
                                              cl::desc(
                                                      "Output all alias sets, their relations and the evaluation results."));

static cl::opt<bool> PreserveCallGraph("preserve-dyck-callgraph", cl::init(false), cl::Hidden,
                                       cl::desc("Preserve the call graph for usage in other passes."));

static cl::opt<bool> DotCallGraph("dot-dyck-callgraph", cl::init(false), cl::Hidden,
                                  cl::desc("Calculate the program's call graph and output into a \"dot\" file."));

static cl::opt<bool> CountFP("count-fp", cl::init(false), cl::Hidden,
                             cl::desc("Calculate how many functions a function pointer may point to."));

char DyckAliasAnalysis::ID = 0;
static RegisterPass<DyckAliasAnalysis> X("dyckaa", "a unification based alias analysis");

static const Function *getParent(const Value *V) {
    if (const auto *inst = dyn_cast<Instruction>(V))
        return inst->getParent()->getParent();

    if (const auto *arg = dyn_cast<Argument>(V))
        return arg->getParent();

    return nullptr;
}

static bool notDifferentParent(const Value *O1, const Value *O2) {

    const Function *F1 = getParent(O1);
    const Function *F2 = getParent(O2);

    return !F1 || !F2 || F1 == F2;
}

DyckAliasAnalysis::DyckAliasAnalysis() :
        ModulePass(ID) {
    dyck_graph = new DyckGraph;
    call_graph = new DyckCallGraph;
    DEREF_LABEL = new DerefEdgeLabel;
}

DyckAliasAnalysis::~DyckAliasAnalysis() {
    delete call_graph;
    delete dyck_graph;

    // delete edge labels
    delete DEREF_LABEL;

    auto olIt = OFFSET_LABEL_MAP.begin();
    while (olIt != OFFSET_LABEL_MAP.end()) {
        delete olIt->second;
        olIt++;
    }

    auto ilIt = INDEX_LABEL_MAP.begin();
    while (ilIt != INDEX_LABEL_MAP.end()) {
        delete ilIt->second;
        ilIt++;
    }

    for (auto &it: vertexMemAllocaMap) {
        delete it.second;
    }
}

void DyckAliasAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
}

const std::set<Value *> *DyckAliasAnalysis::getAliasSet(Value *ptr) const {
    DyckVertex *v = dyck_graph->retrieveDyckVertex(ptr).first;
    return (const std::set<Value *> *) v->getEquivalentSet();
}

bool DyckAliasAnalysis::mayAlias(Value *V1, Value *V2) const {
    return getAliasSet(V1)->count(V2);
}

void DyckAliasAnalysis::getEscapedPointersFrom(std::vector<const std::set<Value *> *> *ret, Value *from) {
    assert(ret != nullptr);

    std::set<DyckVertex *> temp;
    getEscapedPointersFrom(&temp, from);

    auto tempIt = temp.begin();
    while (tempIt != temp.end()) {
        DyckVertex *t = *tempIt;
        ret->push_back((const std::set<Value *> *) t->getEquivalentSet());
        tempIt++;
    }
}

void DyckAliasAnalysis::getEscapedPointersFrom(std::set<DyckVertex *> *ret, Value *from) {
    assert(ret != nullptr);
    assert(from != nullptr);
    if (isa<Argument>(from)) {
        assert(!((Argument *) from)->getParent()->empty());
    }

    std::set<DyckVertex *> &visited = *ret;
    std::stack<DyckVertex *> workStack;

    workStack.push(dyck_graph->retrieveDyckVertex(from).first);

    while (!workStack.empty()) {
        DyckVertex *top = workStack.top();
        workStack.pop();

        // have visited
        if (visited.find(top) != visited.end()) {
            continue;
        }

        visited.insert(top);

        std::set<DyckVertex *> tars;
        top->getOutVertices(&tars);
        auto tit = tars.begin();
        while (tit != tars.end()) {
            // if it has not been visited
            if (visited.find(*tit) == visited.end()) {
                workStack.push(*tit);
            }
            tit++;
        }
    }
}

void DyckAliasAnalysis::getEscapedPointersTo(std::vector<const std::set<Value *> *> *ret, Function *func) {
    assert(ret != nullptr);

    std::set<DyckVertex *> temp;
    getEscapedPointersTo(&temp, func);

    auto tempIt = temp.begin();
    while (tempIt != temp.end()) {
        DyckVertex *t = *tempIt;
        ret->push_back((const std::set<Value *> *) t->getEquivalentSet());
        tempIt++;
    }
}

void DyckAliasAnalysis::getEscapedPointersTo(std::set<DyckVertex *> *ret, Function *func) {
    assert(ret != nullptr);
    assert(func != nullptr);

    Module *module = func->getParent();

    std::set<DyckVertex *> &visited = *ret;
    std::stack<DyckVertex *> workStack;

    for (auto &GV: module->getGlobalList()) {
        if (!GV.hasPrivateLinkage() && !GV.getName().startswith("llvm.") && GV.getName().str() != "stderr"
            && GV.getName().str() != "stdout") { // in fact, no such symbols in src codes.
            DyckVertex *rt = dyck_graph->retrieveDyckVertex(&GV).first;
            workStack.push(rt);
        }
    }

    for (auto &f: *module) {
        for (auto &b: f) {
            for (auto &Inst: b) {
                Instruction *rawInst = &Inst;
                if (isa<CallInst>(rawInst)) {
                    auto *inst = (CallInst *) rawInst; // all invokes are lowered to call
                    bool ar = mayAlias(func, inst->getCalledOperand());
                    if (ar) {
                        if (func->hasName() && func->getName() == "pthread_create") {
                            DyckVertex *rt = dyck_graph->retrieveDyckVertex(inst->getArgOperand(3)).first;
                            workStack.push(rt);
                        } else {
                            unsigned num = inst->getNumArgOperands();
                            for (unsigned i = 0; i < num; i++) {
                                DyckVertex *rt = dyck_graph->retrieveDyckVertex(inst->getArgOperand(i)).first;
                                workStack.push(rt);
                            }
                        }
                    }
                }
            }
        }
    }

    while (!workStack.empty()) {
        DyckVertex *top = workStack.top();
        workStack.pop();

        // have visited
        if (visited.find(top) != visited.end()) {
            continue;
        }

        visited.insert(top);

        std::set<DyckVertex *> tars;
        top->getOutVertices(&tars);
        auto tit = tars.begin();
        while (tit != tars.end()) {
            // if it has not been visited
            DyckVertex *dv = (*tit);
            if (visited.find(dv) == visited.end()) {
                workStack.push(dv);
            }
            tit++;
        }
    }
}

void DyckAliasAnalysis::getPointstoObjects(std::set<Value *> &objects, Value *pointer) {
    assert(pointer != nullptr);

    DyckVertex *rt = dyck_graph->retrieveDyckVertex(pointer).first;
    auto tars = rt->getOutVertices(DEREF_LABEL);
    if (tars != nullptr && !tars->empty()) {
        assert(tars->size() == 1);
        auto tit = tars->begin();
        DyckVertex *tar = (*tit);
        auto vals = tar->getEquivalentSet();
        for (auto &val: *vals) {
            objects.insert((Value *) val);
        }
    }
}

bool DyckAliasAnalysis::isDefaultMemAllocaFunction(Value *calledValue) {
    if (isa<Function>(calledValue)) {
        if (mem_allocas.count((Function *) calledValue)) {
            return true;
        }
    } else {
        for (auto &func: mem_allocas) {
            if (mayAlias(calledValue, func)) {
                return true;
            }
        }
    }

    return false;
}

std::vector<Value *> *DyckAliasAnalysis::getDefaultPointstoMemAlloca(Value *ptr) {
    assert(ptr->getType()->isPointerTy());

    DyckVertex *v = dyck_graph->retrieveDyckVertex(ptr).first;
    if (vertexMemAllocaMap.count(v)) {
        return vertexMemAllocaMap[v];
    }

    auto *objects = new std::vector<Value *>;
    vertexMemAllocaMap[v] = objects;

    // find allocas in v self
    auto aliases = (const std::set<Value *> *) v->getEquivalentSet();

    for (auto &al: *aliases) {
        if (isa<GlobalVariable>(al) || isa<Function>(al)) {
            objects->push_back(al);
        } else if (isa<AllocaInst>(al)) {
            objects->push_back(al);
        } else if (auto *cs = dyn_cast<CallInst>(al)) {
            Value *calledValue = cs->getCalledOperand();
            if (isDefaultMemAllocaFunction(calledValue)) {
                objects->push_back(al);
            }
        } else if (isa<InvokeInst>(al)) {
            llvm_unreachable("find invoke inst, please use -lowerinvoke");
        }
    }

    return objects;
}

bool DyckAliasAnalysis::callGraphPreserved() const {
    return PreserveCallGraph;
}

DyckCallGraph *DyckAliasAnalysis::getCallGraph() const {
    assert(this->callGraphPreserved() && "Please add -preserve-dyck-callgraph option when using opt or canary.\n");
    return call_graph;
}

DyckGraph *DyckAliasAnalysis::getDyckGraph() const {
    return dyck_graph;
}

bool DyckAliasAnalysis::runOnModule(Module &M) {
    {
        auto addAllocLikeFunc = [this, &M](const char *name) {
            if (Function *F = M.getFunction(name)) {
                this->mem_allocas.insert(F);
            }
        };
        addAllocLikeFunc("malloc");
        addAllocLikeFunc("calloc");
        addAllocLikeFunc("realloc");
        addAllocLikeFunc("valloc");
        addAllocLikeFunc("reallocf");
        addAllocLikeFunc("strdup");
        addAllocLikeFunc("strndup");
        addAllocLikeFunc("_Znaj");
        addAllocLikeFunc("_ZnajRKSt9nothrow_t");
        addAllocLikeFunc("_Znam");
        addAllocLikeFunc("_ZnamRKSt9nothrow_t");
        addAllocLikeFunc("_Znwj");
        addAllocLikeFunc("_ZnwjRKSt9nothrow_t");
        addAllocLikeFunc("_Znwm");
        addAllocLikeFunc("_ZnwmRKSt9nothrow_t");
    }

    auto *aaa = new AAAnalyzer(&M, this, dyck_graph, call_graph);

    /// step 1: intra-procedure analysis
    aaa->intra_procedure_analysis();

    /// step 2: inter-procedure analysis
    aaa->inter_procedure_analysis();

    /* call graph */
    if (DotCallGraph) {
        outs() << "Printing call graph...\n";
        call_graph->dotCallGraph(M.getModuleIdentifier());
        outs() << "Done!\n\n";
    }

    if (CountFP) {
        outs() << "Printing function pointer information...\n";
        call_graph->printFunctionPointersInformation(M.getModuleIdentifier());
        outs() << "Done!\n\n";
    }

    delete aaa;
    aaa = nullptr;

    if (!this->callGraphPreserved()) {
        delete this->call_graph;
        this->call_graph = nullptr;
    }

    if (PrintAliasSetInformation) {
        outs() << "Printing alias set information...\n";
        this->printAliasSetInformation(M);
        outs() << "Done!\n\n";
    }

    DEBUG_WITH_TYPE("validate-dyckgraph", dyck_graph->validation(__FILE__, __LINE__));

    return false;
}

void DyckAliasAnalysis::printAliasSetInformation(Module &M) {
    /*if (InterAAEval)*/
    {
        std::set<DyckVertex *> &allreps = dyck_graph->getVertices();

        outs() << "Printing distribution.log... ";
        outs().flush();
        FILE *log = fopen("distribution.log", "w+");

        std::vector<unsigned long> aliasSetSizes;
        unsigned totalSize = 0;
        auto it = allreps.begin();
        while (it != allreps.end()) {
            std::set<void *> *aliasset = (*it)->getEquivalentSet();

            unsigned long size = 0;

            auto asIt = aliasset->begin();
            while (asIt != aliasset->end()) {
                Value *val = ((Value *) (*asIt));
                if (val->getType()->isPointerTy()) {
                    size++;
                }

                asIt++;
            }

            if (size != 0) {
                totalSize = totalSize + size;
                aliasSetSizes.push_back(size);
                fprintf(log, "%lu\n", size);
            }

            it++;
        }
        errs() << totalSize << "\n";
        double pairNum = (((double) totalSize - 1) / 2) * totalSize;

        unsigned noAliasNum = 0;
        for (unsigned i = 0; i < aliasSetSizes.size(); i++) {
            unsigned isize = aliasSetSizes[i];
            for (unsigned j = i + 1; j < aliasSetSizes.size(); j++) {
                noAliasNum = noAliasNum + isize * aliasSetSizes[j];
            }
        }
        //errs() << noAliasNum << "\n";

        double percentOfNoAlias = noAliasNum / (double) pairNum * 100;

        fclose(log);
        outs() << "Done!\n";

        outs() << "===== Alias Analysis Evaluator Report =====\n";
        outs() << "   " << pairNum << " Total Alias Queries Performed\n";
        outs() << "   " << noAliasNum << " no alias responses (" << (unsigned long) percentOfNoAlias << "%)\n\n";
    }

    /*if (DotAliasSet) */
    {
        outs() << "Printing alias_rel.dot... ";
        outs().flush();

        FILE *aliasRel = fopen("alias_rel.dot", "w");
        fprintf(aliasRel, "digraph rel{\n");

        std::set<DyckVertex *> svs;
        Function *PThreadCreate = M.getFunction("pthread_create");
        if (PThreadCreate != nullptr) {
            this->getEscapedPointersTo(&svs, PThreadCreate);
        }

        std::map<DyckVertex *, int> theMap;
        int idx = 0;
        std::set<DyckVertex *> &reps = dyck_graph->getVertices();
        auto repIt = reps.begin();
        while (repIt != reps.end()) {
            idx++;
            if (svs.count(*repIt)) {
                fprintf(aliasRel, "a%d[label=%d color=red];\n", idx, idx);
            } else {
                fprintf(aliasRel, "a%d[label=%d];\n", idx, idx);
            }
            theMap.insert(std::pair<DyckVertex *, int>(*repIt, idx));
            repIt++;
        }

        repIt = reps.begin();
        while (repIt != reps.end()) {
            DyckVertex *dv = *repIt;
            std::map<void *, std::set<DyckVertex *>> &outVs = dv->getOutVertices();

            auto ovIt = outVs.begin();
            while (ovIt != outVs.end()) {
                auto *label = (DyckEdgeLabel *) ovIt->first;
                std::set<DyckVertex *> *oVs = &ovIt->second;

                auto olIt = oVs->begin();
                while (olIt != oVs->end()) {
                    DyckVertex *rep1 = dv;
                    DyckVertex *rep2 = (*olIt);

                    assert(theMap.count(rep1) && "ERROR in DotAliasSet (1)\n");
                    assert(theMap.count(rep2) && "ERROR in DotAliasSet (2)\n");

                    int idx1 = theMap[rep1];
                    int idx2 = theMap[rep2];

                    if (svs.count(rep1) && svs.count(rep2)) {
                        fprintf(aliasRel, "a%d->a%d[label=\"%s\" color=red];\n", idx1, idx2,
                                label->getEdgeLabelDescription().data());
                    } else {
                        fprintf(aliasRel, "a%d->a%d[label=\"%s\"];\n", idx1, idx2,
                                label->getEdgeLabelDescription().data());
                    }

                    olIt++;
                }

                ovIt++;
            }

            theMap.insert(std::pair<DyckVertex *, int>(*repIt, idx));
            repIt++;
        }

        fprintf(aliasRel, "}\n");
        fclose(aliasRel);
        outs() << "Done!\n";
    }

    /*if (OutputAliasSet)*/
    {
        outs() << "Printing alias_sets.log... ";
        outs().flush();

        std::error_code EC;
        raw_fd_ostream log("alias_sets.log", EC);

        std::set<DyckVertex *> svs;
        Function *PThreadCreate = M.getFunction("pthread_create");
        if (PThreadCreate != nullptr) {
            this->getEscapedPointersTo(&svs, PThreadCreate);
        }

        log << "================= Alias Sets ==================\n";
        log << "===== {.} means pthread escaped alias set =====\n";

        int idx = 0;
        std::set<DyckVertex *> &reps = dyck_graph->getVertices();
        auto repsIt = reps.begin();
        while (repsIt != reps.end()) {
            idx++;
            DyckVertex *rep = *repsIt;

            bool pthread_escaped = false;
            if (svs.count(rep)) {
                pthread_escaped = true;
            }

            std::set<void *> *eset = rep->getEquivalentSet();
            auto eit = eset->begin();
            while (eit != eset->end()) {
                auto *val = (Value *) ((*eit));
                assert(val != nullptr && "Error: val is null in an equiv set!");
                if (pthread_escaped) {
                    log << "{" << idx << "}";
                } else {
                    log << "[" << idx << "]";
                }

                if (isa<Function>(val)) {
                    log << ((Function *) val)->getName() << "\n";
                } else {
                    log << *val << "\n";
                }
                eit++;
            }
            repsIt++;
            log << "\n------------------------------\n";
        }

        log.flush();
        log.close();
        outs() << "Done! \n";
    }
}

ModulePass *createDyckAliasAnalysisPass() {
    return new DyckAliasAnalysis();
}

