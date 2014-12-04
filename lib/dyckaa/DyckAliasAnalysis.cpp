/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "DyckAliasAnalysis.h"
#include "DyckCallGraph.h"

#include <stdio.h>
#include <algorithm>
#include <stack>

// cananry options
//static cl::opt<bool>
//CanaryRecordTransformer("canary-record-transformer", cl::init(false), cl::Hidden,
//        cl::desc("Transform programs using canary record transformer."));
//
//static cl::opt<bool>
//CanaryReplayTransformer("canary-replay-transformer", cl::init(false), cl::Hidden,
//        cl::desc("Transform programs using canary replay transformer."));

static cl::opt<bool>
PrintAliasSetInformation("print-alias-set-info", cl::init(false), cl::Hidden,
        cl::desc("Output all alias sets, their relations and the evaluation results."));

static cl::opt<bool>
PreserveCallGraph("preserve-dyck-callgraph", cl::init(false), cl::Hidden,
        cl::desc("Preserve the call graph for usage in other passes."));

static cl::opt<bool>
DotCallGraph("dot-dyck-callgraph", cl::init(false), cl::Hidden,
        cl::desc("Calculate the program's call graph and output into a \"dot\" file."));

static cl::opt<bool>
CountFP("count-fp", cl::init(false), cl::Hidden,
        cl::desc("Calculate how many functions a function pointer may point to."));

static const Function *getParent(const Value *V) {
    if (const Instruction * inst = dyn_cast<Instruction>(V))
        return inst->getParent()->getParent();

    if (const Argument * arg = dyn_cast<Argument>(V))
        return arg->getParent();

    return NULL;
}

static bool notDifferentParent(const Value *O1, const Value *O2) {

    const Function *F1 = getParent(O1);
    const Function *F2 = getParent(O2);

    return !F1 || !F2 || F1 == F2;
}

DyckAliasAnalysis::DyckAliasAnalysis() : ModulePass(ID) {
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
}

void DyckAliasAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<AliasAnalysis>();
    AU.addRequired<TargetLibraryInfo>();
    AU.addRequired<DataLayoutPass>();
}

DyckAliasAnalysis::AliasResult DyckAliasAnalysis::alias(const Location &LocA,
        const Location &LocB) {
    AliasResult ret = MayAlias;
    if (notDifferentParent(LocA.Ptr, LocB.Ptr)) {
        ret = AliasAnalysis::alias(LocA, LocB);
        if (ret != MayAlias) {
            return ret;
        }
    }

    if ((isa<Argument>(LocA.Ptr) && ((Argument*) LocA.Ptr)->getParent()->empty())
            || (isa<Argument>(LocB.Ptr) && ((Argument*) LocB.Ptr)->getParent()->empty())) {
        outs() << "[WARNING] Arguments of empty functions are not supported, MAYALIAS is returned!\n";
        return MayAlias;
    }

    if (LocA.Ptr == LocB.Ptr) {
        return MustAlias;
    }

    pair < DyckVertex*, bool> retpair = dyck_graph->retrieveDyckVertex(const_cast<Value*> (LocA.Ptr));
    DyckVertex * VA = retpair.first->getRepresentative();

    retpair = dyck_graph->retrieveDyckVertex(const_cast<Value*> (LocB.Ptr));
    DyckVertex * VB = retpair.first->getRepresentative();

    if (VA == VB) {
        ret = MayAlias;
    } else if (isPartialAlias(VA, VB)) {
        ret = PartialAlias;
    } else {
        ret = NoAlias;
    }
    
    if (ret == PartialAlias && (isa<Function>(LocA.Ptr) || isa<Function>(LocB.Ptr))) {
        return NoAlias;
    }

    if (ret == MayAlias && (isa<Function>(LocA.Ptr) || isa<Function>(LocB.Ptr))) {
        Function* function = isa<Function>(LocA.Ptr) ? (Function*) LocA.Ptr : (Function*) LocB.Ptr;
        const Value* calledValue = function == LocA.Ptr ? LocB.Ptr : LocA.Ptr;

        Value * cvcopy = const_cast<Value*> (calledValue);
        Value * temp = cvcopy;
        do {
            temp = cvcopy;

            while (isa<ConstantExpr>(cvcopy) && ((ConstantExpr*) cvcopy)->isCast()) {
                cvcopy = ((ConstantExpr*) cvcopy)->getOperand(0)->stripPointerCastsNoFollowAliases();
            }

            while (isa<Instruction>(cvcopy) && ((Instruction*) cvcopy)->isCast()) {
                cvcopy = ((Instruction*) cvcopy)->getOperand(0)->stripPointerCastsNoFollowAliases();
            }

            while (isa<GlobalAlias>(cvcopy)) {
                cvcopy = ((GlobalAlias*) cvcopy)->getAliasee()->stripPointerCastsNoFollowAliases();
            }

        } while (cvcopy != temp);

        if (isa<Function>(cvcopy)) {
            if (cvcopy == function) {
                return MustAlias;
            } else {
                return NoAlias;
            }
        }
    }

    return ret;
}

RegisterPass<DyckAliasAnalysis> X("dyckaa", "Alias Analysis based on PLDI 2013 paper: Qirun Zhang, Michael R. Lyu, Hao Yuan, and Zhendong Su. Fast Algorithms for Dyck-CFL-Reachability with Applications to Alias Analysis.");
RegisterAnalysisGroup<AliasAnalysis> Y(X);

// Register this pass...
char DyckAliasAnalysis::ID = 0;

const set<Value*>* DyckAliasAnalysis::getAliasSet(Value * ptr) {
    DyckVertex* v = dyck_graph->retrieveDyckVertex(ptr).first;
    return (const set<Value*>*) v->getEquivalentSet();
}

bool DyckAliasAnalysis::isPartialAlias(DyckVertex *v1, DyckVertex * v2) {
    if (v1 == NULL || v2 == NULL) return false;

    v1 = v1->getRepresentative();
    v2 = v2->getRepresentative();

    if (v1 == v2) return false;

    set<DyckVertex*> visited;
    stack<DyckVertex*> workStack;
    workStack.push(v1);

    while (!workStack.empty()) {
        DyckVertex* top = workStack.top();
        workStack.pop();

        // have visited
        if (visited.find(top) != visited.end()) {
            continue;
        }

        if (top == v2) {
            return true;
        }

        visited.insert(top);

        { // push out tars
            set<void*>& outlabels = top->getOutLabels();
            set<void*>::iterator olIt = outlabels.begin();
            while (olIt != outlabels.end()) {
                EdgeLabel* labelValue = (EdgeLabel*) (*olIt);
                if (labelValue->isLabelTy(EdgeLabel::OFFSET_TYPE)) { /// address offset; @FIXME
                    set<DyckVertex*>* tars = top->getOutVertices(*olIt);

                    set<DyckVertex*>::iterator tit = tars->begin();
                    while (tit != tars->end()) {
                        // if it has not been visited
                        if (visited.find(*tit) == visited.end()) {
                            workStack.push(*tit);
                        }
                        tit++;
                    }

                }
                olIt++;
            }
        }


        { // push in srcs
            set<void*>& inlabels = top->getInLabels();
            set<void*>::iterator ilIt = inlabels.begin();
            while (ilIt != inlabels.end()) {
                EdgeLabel* labelValue = (EdgeLabel*) (*ilIt);
                if (labelValue->isLabelTy(EdgeLabel::OFFSET_TYPE)) { /// address offset; @FIXME
                    set<DyckVertex*>* srcs = top->getInVertices(*ilIt);

                    set<DyckVertex*>::iterator sit = srcs->begin();
                    while (sit != srcs->end()) {
                        // if it has not been visited
                        if (visited.find(*sit) == visited.end()) {
                            workStack.push(*sit);
                        }
                        sit++;
                    }

                }
                ilIt++;
            }
        }

    }

    return false;
}

void DyckAliasAnalysis::getEscapedPointersFrom(std::vector<const set<Value*>*>* ret, Value * from) {
    assert(ret != NULL && "Error in DyckAliasAnalysis::getEscapedPointersFrom: ret == NULL");

    set<DyckVertex*> temp;
    getEscapedPointersFrom(&temp, from);
    
    auto tempIt = temp.begin();
    while (tempIt != temp.end()) {
        DyckVertex* t = *tempIt;
        ret->push_back((const set<Value*>*)t->getEquivalentSet());
        tempIt++;
    }
}

void DyckAliasAnalysis::getEscapedPointersFrom(set<DyckVertex*>* ret, Value * from) {
    assert(ret != NULL && "Error in DyckAliasAnalysis::getEscapedPointersFrom: ret == NULL");
    assert(from != NULL && "Error in DyckAliasAnalysis::getEscapedPointersFrom: from == NULL");
    if (isa<Argument>(from)) {
        assert(!((Argument*) from)->getParent()->empty() && "Error in DyckAliasAnalysis::getEscapedPointersFrom: from is an empty function's argument");
    }

    set<DyckVertex*>& visited = *ret;
    stack<DyckVertex*> workStack;

    workStack.push(dyck_graph->retrieveDyckVertex(from).first->getRepresentative());

    while (!workStack.empty()) {
        DyckVertex* top = workStack.top();
        workStack.pop();

        // have visited
        if (visited.find(top) != visited.end()) {
            continue;
        }

        visited.insert(top);

        set<DyckVertex*> tars;
        top->getOutVertices(&tars);
        set<DyckVertex*>::iterator tit = tars.begin();
        while (tit != tars.end()) {
            // if it has not been visited
            if (visited.find(*tit) == visited.end()) {
                workStack.push(*tit);
            }
            tit++;
        }
    }
}

void DyckAliasAnalysis::getEscapedPointersTo(std::vector<const set<Value*>*>* ret, Function * func) {
    assert(ret != NULL && "Error in DyckAliasAnalysis::getEscapedPointersTo: ret == NULL");

    set<DyckVertex*> temp;
    getEscapedPointersTo(&temp, func);

    auto tempIt = temp.begin();
    while (tempIt != temp.end()) {
        DyckVertex* t = *tempIt;
        ret->push_back((const set<Value*>*)t->getEquivalentSet());
        tempIt++;
    }
}

void DyckAliasAnalysis::getEscapedPointersTo(set<DyckVertex*>* ret, Function * func) {
    assert(ret != NULL && "Error in DyckAliasAnalysis::getEscapedPointersTo: ret == NULL");
    assert(func != NULL && "Error in DyckAliasAnalysis::getEscapedPointersTo: func == NULL");

    Module* module = func->getParent();

    set<DyckVertex*>& visited = *ret;
    stack<DyckVertex*> workStack;

    iplist<GlobalVariable>::iterator git = module->global_begin();
    while (git != module->global_end()) {
        if (!git->hasPrivateLinkage() && !git->getName().startswith("llvm.") && git->getName().str() != "stderr" && git->getName().str() != "stdout") { // in fact, no such symbols in src codes.
            DyckVertex * rt = dyck_graph->retrieveDyckVertex(git).first->getRepresentative();
            workStack.push(rt);
        }
        git++;
    }

    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function* f = iterF;
        for (ilist_iterator<BasicBlock> iterB = f->getBasicBlockList().begin(); iterB != f->getBasicBlockList().end(); iterB++) {
            for (ilist_iterator<Instruction> iterI = iterB->getInstList().begin(); iterI != iterB->getInstList().end(); iterI++) {
                Instruction *rawInst = iterI;
                if (isa<CallInst> (rawInst)) {
                    CallInst *inst = (CallInst*) rawInst; // all invokes are lowered to call
                    AliasResult ar = this->alias(func, inst->getCalledValue());
                    if (ar == MayAlias || ar == MustAlias) {
                        if (func->hasName() && func->getName() == "pthread_create") {
                            DyckVertex * rt = dyck_graph->retrieveDyckVertex(inst->getArgOperand(3)).first->getRepresentative();
                            workStack.push(rt);
                        } else {
                            unsigned num = inst->getNumArgOperands();
                            for (unsigned i = 0; i < num; i++) {
                                DyckVertex * rt = dyck_graph->retrieveDyckVertex(inst->getArgOperand(i)).first->getRepresentative();
                                workStack.push(rt);
                            }
                        }
                    }
                }
            }
        }
    }

    while (!workStack.empty()) {
        DyckVertex* top = workStack.top();
        workStack.pop();

        // have visited
        if (visited.find(top) != visited.end()) {
            continue;
        }

        visited.insert(top);

        set<DyckVertex*> tars;
        top->getOutVertices(&tars);
        set<DyckVertex*>::iterator tit = tars.begin();
        while (tit != tars.end()) {
            // if it has not been visited
            DyckVertex* dv = (*tit)->getRepresentative();
            if (visited.find(dv) == visited.end()) {
                workStack.push(dv);
            }
            tit++;
        }
    }
}

bool DyckAliasAnalysis::callGraphPreserved() {
    return PreserveCallGraph;
}

DyckCallGraph* DyckAliasAnalysis::getCallGraph() {
    assert(this->callGraphPreserved() && "Please add -preserve-dyck-callgraph option when using opt or canary.\n");
    return call_graph;
}

bool DyckAliasAnalysis::runOnModule(Module & M) {
    InitializeAliasAnalysis(this);

    AAAnalyzer* aaa = new AAAnalyzer(&M, this, dyck_graph, call_graph);

    /// step 1: intra-procedure analysis
    aaa->start_intra_procedure_analysis();
    outs() << "Start intra-procedure analysis...\n";
    aaa->intra_procedure_analysis();
    outs() << "Done!\n\n";
    aaa->end_intra_procedure_analysis();

    /// step 2: inter-procedure analysis 
    aaa->start_inter_procedure_analysis();
    outs() << "Start inter-procedure analysis...";
    aaa->inter_procedure_analysis();
    outs() << "\nDone!\n\n";
    aaa->end_inter_procedure_analysis();

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
    aaa = NULL;

    if (!this->callGraphPreserved()) {
        delete this->call_graph;
        this->call_graph = NULL;
    }

    if (PrintAliasSetInformation) {
        outs() << "Printing alias set information...\n";
        this->printAliasSetInformation(M);
        outs() << "Done!\n\n";
    }

    DEBUG_WITH_TYPE("validate-dyckgraph", dyck_graph->validation(__FILE__, __LINE__));

    /* instrumentation */
    //    if (TraceTransformer || LeapTransformer
    //            || CanaryRecordTransformer || CanaryReplayTransformer) {
    //    set<Instruction*> unhandled_calls; // Currently, it is used only for canary-record-transformer
    //    aaa->getUnhandledCallInstructions(&unhandled_calls);
    //        outs() << ("Thread escape analysis ...\n");
    //        set<DyckVertex*> svs;
    //        set<Value*> llvm_svs;
    //        this->getEscapedPointersTo(&svs, M.getFunction("pthread_create"));
    //        fromDyckVertexToValue(svs, llvm_svs);
    //        outs() << "Done!\n\n";
    //
    //        set<DyckVertex*> lvs;
    //        set<Value *> llvm_lvs;
    //
    //        set<Function*> unsafe_ex_functions;
    //
    //        Transformer * robot = NULL;
    //        if (LeapTransformer) {
    //            robot = new Transformer4Leap(&M, &llvm_svs, this->getDataLayout()->getPointerSize());
    //            outs() << ("Start transforming using leap-transformer ...\n");
    //        } else if (TraceTransformer) {
    //            robot = new Transformer4Trace(&M, &llvm_svs, this->getDataLayout()->getPointerSize());
    //            outs() << ("Start transforming using trace-transformer ...\n");
    //        } else if (CanaryRecordTransformer || CanaryReplayTransformer) {
    //            outs() << "External call escape analysis ...\n";
    //            // get unsafe_ex_functions
    //            for (ilist_iterator<Function> iterF = M.getFunctionList().begin(); iterF != M.getFunctionList().end(); iterF++) {
    //                Function* f = iterF;
    //                if (Transformer4CanaryRecord::isUnsafeExternalLibraryFunction(f)) {
    //                    unsafe_ex_functions.insert(f);
    //                }
    //            }
    //
    //            for (ilist_iterator<Function> iterF = M.getFunctionList().begin(); iterF != M.getFunctionList().end(); iterF++) {
    //                Function* f = iterF;
    //                for (ilist_iterator<BasicBlock> iterB = f->getBasicBlockList().begin(); iterB != f->getBasicBlockList().end(); iterB++) {
    //                    for (ilist_iterator<Instruction> iterI = iterB->getInstList().begin(); iterI != iterB->getInstList().end(); iterI++) {
    //                        Instruction *rawInst = iterI;
    //                        if (isa<CallInst> (rawInst)) {
    //                            CallInst *inst = (CallInst*) rawInst; // all invokes are lowered to call
    //
    //                            if (inst->isInlineAsm()) {
    //                                DEBUG_WITH_TYPE("asm", errs() << "[Inline Asm] " << *inst << "\n");
    //                            }
    //
    //                            set<Function*> mayAliasedFunctions;
    //                            this->get_aliased_functions(&mayAliasedFunctions, &unsafe_ex_functions, inst->getCalledValue(), &M);
    //                            if (mayAliasedFunctions.empty() && !unhandled_calls.count(inst)) {
    //                                continue; // an internal call
    //                            }
    //
    //                            //args escape analysis
    //                            for (unsigned x = 0; x < inst->getNumArgOperands(); x++) {
    //                                bool constant = true && !unhandled_calls.count(inst);
    //                                set<Function*>::iterator mafIt = mayAliasedFunctions.begin();
    //                                while (mafIt != mayAliasedFunctions.end()) {
    //                                    Function* maf = *mafIt;
    //
    //                                    iplist<Argument>& alt = maf->getArgumentList();
    //                                    iplist<Argument>::iterator altIt = alt.begin();
    //                                    unsigned y = 0;
    //                                    while (y < x) {
    //                                        altIt++;
    //                                        if (altIt == alt.end()) {
    //                                            break;
    //                                        }
    //                                        y++;
    //                                    }
    //
    //                                    if (!maf->onlyReadsMemory() && ((altIt != alt.end() &&!altIt->onlyReadsMemory()) || altIt == alt.end())) {
    //                                        constant = false;
    //                                        break;
    //                                    }
    //
    //                                    mafIt++;
    //                                }
    //
    //                                Value *argx = inst->getArgOperand(x);
    //
    //                                if (constant || isa<ConstantInt>(argx)) {
    //                                    continue;
    //                                }
    //
    //                                pair < DyckVertex*, bool> retpair = dyck_graph->retrieveDyckVertex(const_cast<Value*> (argx));
    //                                DyckVertex * argxV = retpair.first->getRepresentative();
    //                                if (!lvs.count(argxV)) {
    //                                    // do escape analysis
    //                                    set<DyckVertex*> tmp_lvs;
    //                                    this->getEscapedPointersFrom(&tmp_lvs, argx);
    //                                    lvs.insert(tmp_lvs.begin(), tmp_lvs.end());
    //                                }
    //                            }
    //
    //                            // return value escape analysis
    //                            if (!inst->doesNotReturn()) {
    //                                set<DyckVertex*> tmp_lvs;
    //                                this->getEscapedPointersFrom(&tmp_lvs, inst);
    //                                lvs.insert(tmp_lvs.begin(), tmp_lvs.end());
    //                            }
    //                        }
    //                    }
    //                }
    //            }
    //            DEBUG_WITH_TYPE("asm", errs() << "\n");
    //
    //            this->fromDyckVertexToValue(lvs, llvm_lvs);
    //
    //            outs() << "Done!\n\n";
    //
    //            // outs() << llvm_lvs.size() << "\n";
    //            if (CanaryRecordTransformer) {
    //                outs() << ("Start transforming using canary-record-transformer ...\n");
    //                robot = new Transformer4CanaryRecord(&M, &llvm_svs, &llvm_lvs, &unhandled_calls, &unsafe_ex_functions, this->getDataLayout()->getPointerSize());
    //            } else {
    //                outs() << "[WARINING] The transformer is in progress\n";
    //                robot = new Transformer4CanaryReplay(&M, &llvm_svs, this->getDataLayout()->getPointerSize());
    //                outs() << ("Start transforming using canary-replay-transformer ...\n");
    //            }
    //        } else {
    //            errs() << "[ERROR] unknown transformer\n";
    //            exit(1);
    //        }
    //
    //        robot->transform(*this);
    //        outs() << "Done!\n\n";
    //
    //        if (LeapTransformer) {
    //            outs() << "\nPleaase add -ltsxleaprecord or -lleaprecord / -lleapreplay for record / replay when you compile the transformed bitcode file to an executable file.\n";
    //        } else if (TraceTransformer) {
    //            
    //        } else if (CanaryRecordTransformer) {
    //            outs() << "Maker sure your bitcode files are compiled using \"-c -emit-llvm -O2 -g -fno-vectorize -fno-slp-vectorize\" options\n";
    //            outs() << "Please add -lcanaryrecord for record at link time\n";
    //        }
    //
    //        delete robot;
    //
    //        return true;
    //    }
    return false;
}

void DyckAliasAnalysis::printAliasSetInformation(Module& M) {
    /*if (InterAAEval)*/
    {
        set<DyckVertex*>& allreps = dyck_graph->getRepresentatives();

        outs() << "Printing distribution.log... ";
        outs().flush();
        FILE * log = fopen("distribution.log", "w+");

        vector<unsigned long> aliasSetSizes;
        double totalSize = 0;
        set<DyckVertex*>::iterator it = allreps.begin();
        while (it != allreps.end()) {
            set<void*>* aliasset = (*it)->getEquivalentSet();

            unsigned long size = 0;

            set<void*>::iterator asIt = aliasset->begin();
            while (asIt != aliasset->end()) {
                Value * val = ((Value*) (*asIt));
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
        //errs() << totalSize << "\n";
        double pairNum = (((totalSize - 1) / 2) * totalSize);

        double noAliasNum = 0;
        for (unsigned i = 0; i < aliasSetSizes.size(); i++) {
            double isize = aliasSetSizes[i];
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

        FILE * aliasRel = fopen("alias_rel.dot", "w");
        fprintf(aliasRel, "digraph rel{\n");

        set<DyckVertex*> svs;
        this->getEscapedPointersTo(&svs, M.getFunction("pthread_create"));

        map<DyckVertex*, int> theMap;
        int idx = 0;
        set<DyckVertex*>& reps = dyck_graph->getRepresentatives();
        auto repIt = reps.begin();
        while (repIt != reps.end()) {
            idx++;
            if (svs.count(*repIt)) {
                fprintf(aliasRel, "a%d[label=%d color=red];\n", idx, idx);
            } else {
                fprintf(aliasRel, "a%d[label=%d];\n", idx, idx);
            }
            theMap.insert(pair<DyckVertex*, int>(*repIt, idx));
            repIt++;
        }

        repIt = reps.begin();
        while (repIt != reps.end()) {
            DyckVertex* dv = *repIt;
            map<void*, set<DyckVertex*>*>& outVs = dv->getOutVertices();

            auto ovIt = outVs.begin();
            while (ovIt != outVs.end()) {
                EdgeLabel* label = (EdgeLabel*) ovIt->first;
                set<DyckVertex*>* oVs = ovIt->second;

                set<DyckVertex*>::iterator olIt = oVs->begin();
                while (olIt != oVs->end()) {
                    DyckVertex * rep1 = dv->getRepresentative();
                    DyckVertex * rep2 = (*olIt)->getRepresentative();

                    assert(theMap.count(rep1) && "ERROR in DotAliasSet (1)\n");
                    assert(theMap.count(rep2) && "ERROR in DotAliasSet (2)\n");

                    int idx1 = theMap[rep1];
                    int idx2 = theMap[rep2];

                    if (svs.count(rep1) && svs.count(rep2)) {
                        fprintf(aliasRel, "a%d->a%d[label=\"%s\" color=red];\n", idx1, idx2, label->getEdgeLabelDescription().data());
                    } else {
                        fprintf(aliasRel, "a%d->a%d[label=\"%s\"];\n", idx1, idx2, label->getEdgeLabelDescription().data());
                    }

                    olIt++;
                }

                ovIt++;
            }

            theMap.insert(pair<DyckVertex*, int>(*repIt, idx));
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

        FILE * log = fopen("alias_sets.log", "w+");

        set<DyckVertex*> svs;
        this->getEscapedPointersTo(&svs, M.getFunction("pthread_create"));

        fprintf(log, "================= Alias Sets ==================\n");
        fprintf(log, "===== {.} means pthread escaped alias set =====\n");
        int idx = 0;
        set<DyckVertex*>& reps = dyck_graph->getRepresentatives();
        set<DyckVertex*>::iterator repsIt = reps.begin();
        while (repsIt != reps.end()) {
            idx++;
            DyckVertex* rep = *repsIt;

            bool pthread_escaped = false;
            if (svs.count(rep)) {
                pthread_escaped = true;
            }

            Value * val = (Value*) (rep->getValue());
            set<void*>* eset = rep->getEquivalentSet();
            set<void*>::iterator eit = eset->begin();
            while (eit != eset->end()) {
                val = (Value*) ((*eit));
                assert(val != NULL && "Error: val is null in an equiv set!");
                if (pthread_escaped) {
                    fprintf(log, "{%d} %s", idx, val->getName().data());
                } else {
                    fprintf(log, "[%d] %s", idx, val->getName().data());
                }
                eit++;
            }
            repsIt++;
            fprintf(log, "------------------------------\n");
        }

        fclose(log);
        outs() << "Done! \n";
    }
}

ModulePass *createDyckAliasAnalysisPass() {
    return new DyckAliasAnalysis();
}

