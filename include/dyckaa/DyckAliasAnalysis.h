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

#include "dyckgraph/DyckGraph.h"
#include "dyckcg/DyckCallGraph.h"
#include "dyckaa/AAAnalyzer.h"

#include <set>

using namespace llvm;
using namespace std;

class DyckAliasAnalysis: public ModulePass, public AliasAnalysis {
public:
	static char ID; // Class identification, replacement for typeinfo

	DyckAliasAnalysis();

	virtual ~DyckAliasAnalysis();

	virtual bool runOnModule(Module &M);

	virtual void getAnalysisUsage(AnalysisUsage &AU) const;

	virtual AliasResult alias(const Location &LocA, const Location &LocB);

	AliasResult alias(const Value *V1, uint64_t V1Size, const Value *V2, uint64_t V2Size) {
		return alias(Location(V1, V1Size), Location(V2, V2Size));
	}

	AliasResult alias(const Value *V1, const Value *V2) {
		return alias(V1, UnknownSize, V2, UnknownSize);
	}

	/// Get the may/must alias set.
	virtual const set<Value*>* getAliasSet(Value * ptr) const;

	virtual ModRefResult getModRefInfo(ImmutableCallSite CS, const Location &Loc) {
		return AliasAnalysis::getModRefInfo(CS, Loc);
	}

	virtual ModRefResult getModRefInfo(ImmutableCallSite CS1, ImmutableCallSite CS2) {
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
			return (AliasAnalysis*) this;
		return this;
	}

private:
	DyckGraph* dyck_graph;
	DyckCallGraph * call_graph;

	std::set<Function*> mem_allocas;
	map<DyckVertex*, std::vector<Value*>*> vertexMemAllocaMap;

private:
	friend class AAAnalyzer;

	EdgeLabel * DEREF_LABEL;
	map<long, EdgeLabel*> OFFSET_LABEL_MAP;
	map<long, EdgeLabel*> INDEX_LABEL_MAP;

private:
	EdgeLabel* getOrInsertOffsetEdgeLabel(long offset) {
		if (OFFSET_LABEL_MAP.count(offset)) {
			return OFFSET_LABEL_MAP[offset];
		} else {
			EdgeLabel* ret = new PointerOffsetEdgeLabel(offset);
			OFFSET_LABEL_MAP.insert(pair<long, EdgeLabel*>(offset, ret));
			return ret;
		}
	}

	EdgeLabel* getOrInsertIndexEdgeLabel(long offset) {
		if (INDEX_LABEL_MAP.count(offset)) {
			return INDEX_LABEL_MAP[offset];
		} else {
			EdgeLabel* ret = new FieldIndexEdgeLabel(offset);
			INDEX_LABEL_MAP.insert(pair<long, EdgeLabel*>(offset, ret));
			return ret;
		}
	}

private:

	/// Determine whether the object that VB points to can be got by
	/// extractvalue instruction from the object VA points to.
	bool isPartialAlias(DyckVertex *VA, DyckVertex *VB);

	/// Three kinds of information will be printed.
	/// 1. Alias Sets will be printed to the console
	/// 2. The relation of Alias Sets will be output into "alias_rel.dot"
	/// 3. The evaluation results will be output into "distribution.log"
	///     The summary of the evaluation will be printed to the console
	void printAliasSetInformation(Module& M);

	void getEscapedPointersTo(set<DyckVertex*>* ret, Function * func); // escaped to 'func'
	void getEscapedPointersFrom(set<DyckVertex*>* ret, Value * from); // escaped from 'from'

public:
	/// Get the vector of the may/must alias set that escape to 'func'
	void getEscapedPointersTo(std::vector<const set<Value*>*>* ret, Function * func);

	/// Get the vector of the may/must alias set that escape from 'from'
	void getEscapedPointersFrom(std::vector<const set<Value*>*>* ret, Value * from);

	bool callGraphPreserved();
	DyckCallGraph* getCallGraph();

	DyckGraph* getDyckGraph() {
	    return dyck_graph;
	}

	/// Get the set of objects that a pointer may point to,
	/// e.g. for %a = load i32* %b, {%a} will be returned for the
	/// pointer %b
	void getPointstoObjects(std::set<Value*>& objects, Value* pointer);

    /// Given a pointer %p, suppose the memory location that %p points to
    /// is allocated by instruction "%p = malloc(...)", then the instruction
    /// will be returned.
    ///
    /// For a global variable @q, since its memory location is allocated
    /// statically, itself will be returned.
    ///
    /// Default mem alloca function includes
    /// {
    ///    "calloc", "malloc", "realloc",
    ///    "_Znaj", "_ZnajRKSt9nothrow_t",
    ///    "_Znam", "_ZnamRKSt9nothrow_t",
    ///    "_Znwj", "_ZnwjRKSt9nothrow_t",
    ///    "_Znwm", "_ZnwmRKSt9nothrow_t"
    /// }
    ///
    /// For a pointer that points to a struct or class field, this interface
    /// may return you nothing, because the field may be initialized using
    /// the same allocation instruction as the struct or class. For example
    /// %addr = alloca {int, int} not only initializes the memory %addr points to,
    /// but also initializes the memory gep %addr 0 and gep %addr 1 point to.
    std::vector<Value*>* getDefaultPointstoMemAlloca(Value* pointer);

    /// Default mem alloca function includes
    /// {
    ///    "calloc", "malloc", "realloc",
    ///    "_Znaj", "_ZnajRKSt9nothrow_t",
    ///    "_Znam", "_ZnamRKSt9nothrow_t",
    ///    "_Znwj", "_ZnwjRKSt9nothrow_t",
    ///    "_Znwm", "_ZnwmRKSt9nothrow_t"
    /// }
    bool isDefaultMemAllocaFunction(Value* calledValue);

};

llvm::ModulePass *createDyckAliasAnalysisPass();

namespace llvm {
void initializeDyckAliasAnalysisPass(PassRegistry &Registry);
}

#endif
