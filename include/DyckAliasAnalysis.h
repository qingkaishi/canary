/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef DYCKALIASANALYSIS_H
#define DYCKALIASANALYSIS_H

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/InlineAsm.h"

#include "DyckGraph.h"
#include "DyckCallGraph.h"
#include "AAAnalyzer.h"

#include <set>

using namespace llvm;
using namespace std;

class DyckAliasAnalysis : public ModulePass, public AliasAnalysis {
public:
    static char ID; // Class identification, replacement for typeinfo

    DyckAliasAnalysis();
    
    virtual ~DyckAliasAnalysis();

    virtual bool runOnModule(Module &M);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual AliasResult alias(const Location &LocA,
            const Location &LocB);

    AliasResult alias(const Value *V1, uint64_t V1Size,
            const Value *V2, uint64_t V2Size) {
        return alias(Location(V1, V1Size), Location(V2, V2Size));
    }

    AliasResult alias(const Value *V1, const Value *V2) {
        return alias(V1, UnknownSize, V2, UnknownSize);
    }

    virtual const set<Value*>* getAliasSet(Value * ptr);

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
    DyckCallGraph * call_graph;
    
private:
    friend class AAAnalyzer;
    
    EdgeLabel * DEREF_LABEL;
    map<long, EdgeLabel*> OFFSET_LABEL_MAP;
    map<long, EdgeLabel*> INDEX_LABEL_MAP;
    
private:
    EdgeLabel* getOrInsertOffsetEdgeLabel(long offset){
        if(OFFSET_LABEL_MAP.count(offset)) {
            return OFFSET_LABEL_MAP[offset];
        } else {
            EdgeLabel* ret = new PointerOffsetEdgeLabel(offset);
            OFFSET_LABEL_MAP.insert(pair<long, EdgeLabel*>(offset, ret));
            return ret;
        }
    }
    
    EdgeLabel* getOrInsertIndexEdgeLabel(long offset){
        if(INDEX_LABEL_MAP.count(offset)) {
            return INDEX_LABEL_MAP[offset];
        } else {
            EdgeLabel* ret = new FieldIndexEdgeLabel(offset);
            INDEX_LABEL_MAP.insert(pair<long, EdgeLabel*>(offset, ret));
            return ret;
        }
    }

private:
    bool isPartialAlias(DyckVertex *v1, DyckVertex *v2);
    void fromDyckVertexToValue(set<DyckVertex*>& from, set<Value*>& to);
    
    /// Three kinds of information will be printed.
    /// 1. Alias Sets will be printed to the console
    /// 2. The relation of Alias Sets will be output into "alias_rel.dot"
    /// 3. The evaluation results will be output into "distribution.log"
    ///     The summary of the evaluation will be printed to the console
    void printAliasSetInformation(Module& M);

    void getEscapedPointersTo(set<DyckVertex*>* ret, Function * func); // escaped to 'func'
    void getEscapedPointersFrom(set<DyckVertex*>* ret, Value * from); // escaped from 'from'
    
 public:  
    void getEscapedPointersTo(set<Value*>* ret, Function * func); // escaped to 'func'
    void getEscapedPointersFrom(set<Value*>* ret, Value * from); // escaped from 'from'
    
    bool callGraphPreserved();
    DyckCallGraph* getCallGraph();

};

llvm::ModulePass *createDyckAliasAnalysisPass();


#endif
