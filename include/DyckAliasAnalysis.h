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
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/InlineAsm.h"

#include <set>

using namespace llvm;

class ExtraAliasAnalysisInterface {
public:
    virtual AliasAnalysis::AliasResult function_alias(const Function* function, CallInst* callInst) = 0;
    
    /// search aliased functions in uset if uset != NULL
    /// otherwise, we will search all functions in the module
    virtual std::set<Function*>* get_aliased_functions(std::set<Function*>* ret, std::set<Function*>* uset, CallInst* call) = 0;
};

llvm::ModulePass *createDyckAliasAnalysisPass();


#endif
