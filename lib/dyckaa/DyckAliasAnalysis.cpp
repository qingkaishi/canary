#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/GlobalAlias.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/LLVMContext.h"
#include "llvm/Operator.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/DataLayout.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include "DyckGraph.h"
#include "AAAnalyzer.h"

#include "Transformer.h"
#include "Transformer4Replay.h"
#include "Transformer4Trace.h"

#include <stdio.h>
#include <algorithm>
#include <stack>
using namespace llvm;

static cl::opt<bool>
LeapTransformer("leap-transformer", cl::init(false), cl::Hidden,
        cl::desc("Transform programs using Leap transformer."));

static cl::opt<bool>
PecanTransformer("pecan-transformer", cl::init(false), cl::Hidden,
        cl::desc("Transform programs using pecan transformer."));

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

            if (LeapTransformer && PecanTransformer) {
                errs() << "Error: you cannot use leap-transformer and pecan-transformer together.\n";
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
            if (!retpair.second) {
                return NoAlias;
            }
            DyckVertex * VA = retpair.first->getRepresentative();

            retpair = dyck_graph->retrieveDyckVertex(const_cast<Value*> (LocB.Ptr));
            if (!retpair.second) {
                return NoAlias;
            }
            DyckVertex * VB = retpair.first->getRepresentative();

            if (VA == VB) {
                return MayAlias;
            } else {
                set<DyckVertex*>* VAderefset = VA->getOutVertices((void*) DEREF_LABEL);
                if(VAderefset == NULL || VAderefset->empty()){
                    return NoAlias;
                }
                
                set<DyckVertex*>* VBderefset = VB->getOutVertices((void*) DEREF_LABEL);
                if(VBderefset == NULL || VBderefset->empty()){
                    return NoAlias;
                }
                
                DyckVertex* VAderef = (*(VAderefset->begin()))->getRepresentative();
                DyckVertex* VBderef = (*(VBderefset->begin()))->getRepresentative();
                
                if (dyck_graph->havePathsWithoutLabel(VAderef, VBderef, (void*) DEREF_LABEL)) {
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
        set<Value *> thread_escapes_pts;

    private:
        void getThreadEscapingPointers(set<DyckVertex*>* ret);
    };

    RegisterPass<DyckAliasAnalysis> X("dyckaa", "Alias Analysis based on Qirun's PLDI 2013 paper");
    RegisterAnalysisGroup<AliasAnalysis> Y(X);

    // Register this pass...
    char DyckAliasAnalysis::ID;

    void DyckAliasAnalysis::getThreadEscapingPointers(set<DyckVertex*>* ret) {
        if (ret == NULL)
            return;

        set<DyckVertex*> visited;
        stack<DyckVertex*> workStack;

        set<Value *>::iterator teptsIt = thread_escapes_pts.begin();
        while (teptsIt != thread_escapes_pts.end()) {
            DyckVertex * fromVertex = dyck_graph->retrieveDyckVertex(*teptsIt).first->getRepresentative();
            workStack.push(fromVertex);
            teptsIt++;
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

        set<DyckVertex*>::iterator vit = visited.begin();
        while (vit != visited.end()) {
            DyckVertex* dv = *vit;

            bool needToAdded = true;
            set<DyckVertex*>::iterator retIt = ret->begin();
            while (retIt != ret->end()) {
                if (dyck_graph->havePathsWithoutLabel(dv, *retIt, (void*) DEREF_LABEL)) {
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

        AAAnalyzer* aaa = new AAAnalyzer(&M, this, dyck_graph, DotCallGraph||CountFP);

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
            
            if(finished){
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


        /* do something others if needed */
        if (PecanTransformer || LeapTransformer) {
            aaa->getValuesEscapedFromThreadCreate(&thread_escapes_pts);
        }
        
        /* call graph */
        if(DotCallGraph) {
            outs() << "Printing call graph...\n";
            aaa->printCallGraph(M.getModuleIdentifier());
            outs() << "Done!\n\n";
        }
        
        if(CountFP){
            outs() << "Printing function pointer information...\n";
            aaa->printFunctionPointersInformation(M.getModuleIdentifier());
            outs() << "Done!\n\n";
        }

        delete aaa;

        /* instrumentation */
        if (PecanTransformer || LeapTransformer) {
            // do something others
            iplist<GlobalVariable>::iterator git = M.global_begin();
            while (git != M.global_end()) {
                if (!git->hasUnnamedAddr() && !git->isConstant() && !git->isThreadLocal()) {
                    thread_escapes_pts.insert(git);
                }
                git++;
            }
            iplist<GlobalAlias>::iterator ait = M.alias_begin();
            while (ait != M.alias_end()) {
                if (!ait->hasUnnamedAddr()) {
                    thread_escapes_pts.insert(git);
                }
                ait++;
            }


            set<Value*> llvm_svs;
            set<DyckVertex*> svs;
            this->getThreadEscapingPointers(&svs);
            //int idx = 0;
            set<DyckVertex*>::iterator svsIt = svs.begin();
            while (svsIt != svs.end()) {
                Value * val = (Value*) ((*svsIt)->getValue());
                if (val != NULL) {
                    llvm_svs.insert(val);
                    //outs() << "[" << idx++ << "] " << *val << "\n";
                } else {
                    set<DyckVertex*>* eset = (*svsIt)->getEquivalentSet();
                    set<DyckVertex*>::iterator eit = eset->begin();
                    while (eit != eset->end()) {
                        val = (Value*) ((*eit)->getValue());
                        if (val != NULL) {
                            //outs() << "[" << idx++ << "] " << *val << "\n";
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
                robot = new Transformer4Replay(&M, &llvm_svs, ptrsize);
                outs() << ("Start transforming using leap-transformer ...\n");
            } else if (PecanTransformer) {
                robot = new Transformer4Trace(&M, &llvm_svs, ptrsize);
                outs() << ("Start transforming using pecan-transformer ...\n");
            } else {
                errs() << "Error: unknown transformer\n";
                exit(1);
            }

            robot->transform(*this);
            outs() << "Done!\n";
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
                    Value * val = ((Value*)(*asIt)->getValue());
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

ModulePass *llvm::createDyckAliasAnalysisPass() {
    return new DyckAliasAnalysis();
}

