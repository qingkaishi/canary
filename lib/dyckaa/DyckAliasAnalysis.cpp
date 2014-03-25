#include "DyckGraph.h"
#include "AAAnalyzer.h"
#include "Transformer.h"
#include "Transformer4Leap.h"
#include "Transformer4Trace.h"
#include "Transformer4CanaryRecord.h"
#include "Transformer4CanaryReplay.h"

#include "DyckAliasAnalysis.h"

#include <stdio.h>
#include <algorithm>
#include <stack>
using namespace llvm;

// cananry options
static cl::opt<bool>
CanaryRecordTransformer("canary-record-transformer", cl::init(false), cl::Hidden,
        cl::desc("Transform programs using canary record transformer."));

static cl::opt<bool>
CanaryReplayTransformer("canary-replay-transformer", cl::init(false), cl::Hidden,
        cl::desc("Transform programs using canary replay transformer."));

static cl::opt<bool>
LeapTransformer("leap-transformer", cl::init(false), cl::Hidden,
        cl::desc("Transform programs using Leap transformer."));

static cl::opt<bool>
TraceTransformer("trace-transformer", cl::init(false), cl::Hidden,
        cl::desc("Transform programs to record a trace."));

static cl::opt<bool>
DotCallGraph("dot-may-callgraph", cl::init(false), cl::Hidden,
        cl::desc("Calculate the program's call graph and output into a \"dot\" file."));

static cl::opt<bool>
InterAAEval("inter-aa-eval", cl::init(false), cl::Hidden,
        cl::desc("Inter-procedure alias analysis evaluator."));

static cl::opt<bool>
CountFP("count-fp", cl::init(false), cl::Hidden,
        cl::desc("Calculate how many function pointers point to."));

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

namespace {
    /// DyckAliasAnalysis - This is the primary alias analysis implementation.

    struct DyckAliasAnalysis : public ModulePass, public AliasAnalysis {
        static char ID; // Class identification, replacement for typeinfo

        DyckAliasAnalysis() : ModulePass(ID) {
            dyck_graph = new DyckGraph;

            if (LeapTransformer && TraceTransformer 
                    && CanaryRecordTransformer && CanaryReplayTransformer) {
                errs() << "Error: you cannot use different transformers together.\n";
                exit(1);
            }
        }

        virtual bool runOnModule(Module &M);

        virtual void getAnalysisUsage(AnalysisUsage &AU) const {
            //AliasAnalysis::getAnalysisUsage(AU);
            AU.addRequired<AliasAnalysis>();
            AU.addRequired<TargetLibraryInfo>();
            AU.addRequired<DataLayout>();
            //AU.setPreservesAll();
        }

        virtual AliasResult alias(const Location &LocA,
                const Location &LocB) {
            if (notDifferentParent(LocA.Ptr, LocB.Ptr)) {
                AliasResult ret = AliasAnalysis::alias(LocA, LocB);
                if (ret == NoAlias) {
                    return ret;
                }

                if (ret == MustAlias) {
                    return ret;
                }

                if (ret == PartialAlias) {
                    return ret;
                }
            }

            pair < DyckVertex*, bool> retpair = dyck_graph->retrieveDyckVertex(const_cast<Value*> (LocA.Ptr));
            DyckVertex * VA = retpair.first->getRepresentative();

            retpair = dyck_graph->retrieveDyckVertex(const_cast<Value*> (LocB.Ptr));
            DyckVertex * VB = retpair.first->getRepresentative();

            if (VA == VB) {
                return MayAlias;
            } else {
                if (isPartialAlias(VA, VB)) {
                    return PartialAlias;
                } else {
                    return NoAlias;
                }
            }
        }

        virtual ModRefResult getModRefInfo(ImmutableCallSite CS,
                const Location &Loc) {
            return AliasAnalysis::getModRefInfo(CS, Loc);
        }

        virtual ModRefResult getModRefInfo(ImmutableCallSite CS1,
                ImmutableCallSite CS2) {
            return AliasAnalysis::getModRefInfo(CS1, CS2);
        }

        /// pointsToConstantMemory - Chase pointers until we find a (constant
        /// global) or not.

        virtual bool pointsToConstantMemory(const Location &Loc, bool OrLocal) {
            return AliasAnalysis::pointsToConstantMemory(Loc, OrLocal);
        }

        /// getModRefBehavior - Return the behavior when calling the given
        /// call site.

        virtual ModRefBehavior getModRefBehavior(ImmutableCallSite CS) {
            return AliasAnalysis::getModRefBehavior(CS);
        }

        /// getModRefBehavior - Return the behavior when calling the given function.
        /// For use when the call site is not known.

        virtual ModRefBehavior getModRefBehavior(const Function *F) {
            return AliasAnalysis::getModRefBehavior(F);
        }

        /// getAdjustedAnalysisPointer - This method is used when a pass implements
        /// an analysis interface through multiple inheritance.  If needed, it
        /// should override this to adjust the this pointer as needed for the
        /// specified pass info.

        virtual void *getAdjustedAnalysisPointer(const void *ID) {
            if (ID == &AliasAnalysis::ID)
                return (AliasAnalysis*)this;
            return this;
        }

    private:
        DyckGraph* dyck_graph;

    private:
        bool isPartialAlias(DyckVertex *v1, DyckVertex *v2);
        void getEscapingPointers(set<DyckVertex*>* ret, Function * func);
    };

    RegisterPass<DyckAliasAnalysis> X("dyckaa", "Alias Analysis based on Qirun's PLDI 2013 paper");
    RegisterAnalysisGroup<AliasAnalysis> Y(X);

    // Register this pass...
    char DyckAliasAnalysis::ID = 0;

    bool DyckAliasAnalysis::isPartialAlias(DyckVertex *v1, DyckVertex *v2) {
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
                    long labelValue = (long) (*olIt);
                    if (labelValue > 0) { /// address offset; @FIXME
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
                    long labelValue = (long) (*ilIt);
                    if (labelValue > 0) { /// address offset; @FIXME
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

    void DyckAliasAnalysis::getEscapingPointers(set<DyckVertex*>* ret, Function *func) {
        if (ret == NULL || func == NULL) {
            errs() << "Error in getEscapingPointers: ret or func are null!\n";
            exit(-1);
        }

        Module* module = func->getParent();

        set<DyckVertex*> visited;
        stack<DyckVertex*> workStack;

        iplist<GlobalVariable>::iterator git = module->global_begin();
        while (git != module->global_end()) {
            if (!git->hasUnnamedAddr() && !git->isConstant()) {
                DyckVertex * rt = dyck_graph->retrieveDyckVertex(git).first->getRepresentative();
                workStack.push(rt);
            }
            git++;
        }
        iplist<GlobalAlias>::iterator ait = module->alias_begin();
        while (ait != module->alias_end()) {
            if (!ait->hasUnnamedAddr()) {
                DyckVertex * rt = dyck_graph->retrieveDyckVertex(ait).first->getRepresentative();
                workStack.push(rt);
            }
            ait++;
        }

        iplist<Argument>& alt = func->getArgumentList();
        iplist<Argument>::iterator it = alt.begin();
        if (func->hasName() && func->getName() == "pthread_create") {
            it++;
            it++;
            it++;
            DyckVertex * rt = dyck_graph->retrieveDyckVertex(it).first->getRepresentative();
            workStack.push(rt);
        } else {
            while (it != alt.end()) {
                DyckVertex * rt = dyck_graph->retrieveDyckVertex(it).first->getRepresentative();
                workStack.push(rt);
                it++;
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
            top->getOutVerticesWithout(NULL, &tars);
            set<DyckVertex*>::iterator tit = tars.begin();
            while (tit != tars.end()) {
                // if it has not been visited
                if (visited.find(*tit) == visited.end()) {
                    workStack.push(*tit);
                }
                tit++;
            }
        }

        // If A is a partial alias of B, A will not be put in ret, because
        // we will consider them as a pair of may alias during instrumentation.
        set<DyckVertex*>::iterator vit = visited.begin();
        while (vit != visited.end()) {
            DyckVertex* dv = *vit;

            bool needToAdded = true;
            set<DyckVertex*>::iterator retIt = ret->begin();
            while (retIt != ret->end()) {
                if (this->isPartialAlias(dv, *retIt)) {
                    needToAdded = false;
                    break;
                }
                retIt++;
            }
            if (needToAdded)
                ret->insert(dv);
            vit++;
        }
    }

    bool DyckAliasAnalysis::runOnModule(Module &M) {
        InitializeAliasAnalysis(this);

        AAAnalyzer* aaa = new AAAnalyzer(&M, this, dyck_graph, DotCallGraph || CountFP);

        /// step 1: intra-procedure analysis
        aaa->start_intra_procedure_analysis();
        outs() << "Start intra-procedure analysis...\n";
        aaa->intra_procedure_analysis();
        outs() << "Done!\n\n";
        aaa->end_intra_procedure_analysis();

        /// step 2: inter-procedure analysis 
        aaa->start_inter_procedure_analysis();
        outs() << "Start inter-procedure analysis...\n";
        int itTimes = 0;
        while (1) {
            bool finished = dyck_graph->qirunAlgorithm();

            if (itTimes != 0 && finished) {
                break;
            }

            outs() << "Iterating... " << ++itTimes << "             \r";

            finished = aaa->inter_procedure_analysis();

            if (finished) {
                break;
            }
        }
        outs() << "\nDone!\n\n";
        aaa->end_inter_procedure_analysis();

        /* call graph */
        if (DotCallGraph) {
            outs() << "Printing call graph...\n";
            aaa->printCallGraph(M.getModuleIdentifier());
            outs() << "Done!\n\n";
        }

        if (CountFP) {
            outs() << "Printing function pointer information...\n";
            aaa->printFunctionPointersInformation(M.getModuleIdentifier());
            outs() << "Done!\n\n";
        }

        delete aaa;

        /* instrumentation */
        if (TraceTransformer || LeapTransformer 
                || CanaryRecordTransformer || CanaryReplayTransformer) {
            set<Value*> llvm_svs;
            set<DyckVertex*> svs;
            this->getEscapingPointers(&svs, M.getFunction("pthread_create"));
            //int idx = 0;
            set<DyckVertex*>::iterator svsIt = svs.begin();
            while (svsIt != svs.end()) {
                Value * val = (Value*) ((*svsIt)->getValue());
                if (val != NULL) {
                    llvm_svs.insert(val);
                    //outs() << "[" << idx++ << "] " << *val << "\n";
                    //outs() << (*svsIt)->getEquivalentSet()->size()<<"\n";
                } else {
                    set<DyckVertex*>* eset = (*svsIt)->getEquivalentSet();
                    set<DyckVertex*>::iterator eit = eset->begin();
                    while (eit != eset->end()) {
                        val = (Value*) ((*eit)->getValue());
                        if (val != NULL) {
                            //outs() << "[" << idx++ << "] " << *val << "\n";
                            //outs() << (*svsIt)->getEquivalentSet()->size()<<"\n"; 
                            llvm_svs.insert(val);
                            break;
                        }
                        eit++;
                    }

                }
                svsIt++;
            }
            outs() << "Shared variable groups number: " << llvm_svs.size() << "\n";

            unsigned ptrsize = this->getDataLayout()->getPointerSize();

            Transformer * robot = NULL;
            if (LeapTransformer) {
                robot = new Transformer4Leap(&M, &llvm_svs, ptrsize);
                outs() << ("Start transforming using leap-transformer ...\n");
            } else if (TraceTransformer) {
                robot = new Transformer4Trace(&M, &llvm_svs, ptrsize);
                outs() << ("Start transforming using trace-transformer ...\n");
            } if(CanaryRecordTransformer) {
                robot = new Transformer4CanaryRecord(&M, &llvm_svs, ptrsize);
                outs() << ("Start transforming using canary-record-transformer ...\n");
            } if(CanaryReplayTransformer) {
                robot = new Transformer4CanaryReplay(&M, &llvm_svs, ptrsize);
                outs() << ("Start transforming using canary-replay-transformer ...\n");
            }else {
                errs() << "Error: unknown transformer\n";
                exit(1);
            }

            robot->transform(*this);
            outs() << "Done!\n";


            if (LeapTransformer)
                outs() << "\nPleaase add -ltsxleaprecord or -lleaprecord / -lleapreplay for record / replay when you compile the transformed bitcode file to an executable file.\n";

            if (TraceTransformer)
                outs() << "Pleaase add -ltrace for trace analysis when you compile the transformed bitcode file to an executable file. Please use pecan_log_analyzer to predict crugs.\n";

            delete robot;

            return true;
        }

        if (InterAAEval) {
            set<DyckVertex*>& allreps = dyck_graph->getRepresentatives();

            FILE * log = fopen("distribution.log", "w+");

            vector<unsigned long> aliasSetSizes;
            double totalSize = 0;
            set<DyckVertex*>::iterator it = allreps.begin();
            while (it != allreps.end()) {
                set<DyckVertex*>* aliasset = (*it)->getEquivalentSet();

                unsigned long size = 0;

                set<DyckVertex*>::iterator asIt = aliasset->begin();
                while (asIt != aliasset->end()) {
                    Value * val = ((Value*) (*asIt)->getValue());
                    if (val != NULL && val->getType()->isPointerTy()) {
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
            errs() << "===== Alias Analysis Evaluator Report =====\n";
            errs() << "   " << pairNum << " Total Alias Queries Performed\n";
            errs() << "   " << noAliasNum << " no alias responses (" << (unsigned long) percentOfNoAlias << "%)\n\n";
        }

        return false;
    }
}

ModulePass *createDyckAliasAnalysisPass() {
    return new DyckAliasAnalysis();
}

