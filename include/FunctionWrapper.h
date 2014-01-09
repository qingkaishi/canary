/* 
 * File:   FunctionWrapper.h
 * Author: jack
 * 
 * This class is used for inter-procedure analysis
 *
 * Created on November 9, 2013, 12:35 PM
 */

#ifndef FUNCTIONWRAPPER_H
#define	FUNCTIONWRAPPER_H
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

#include <set>
#include <map>
#include <vector>


using namespace llvm;
using namespace std;

class CommonCall{
public:
    Value * ret;
    vector<Value *> args;
    Function * callee;
    
    CommonCall(Function * f, Value* ci);
};

class FunctionWrapper {
private:
    Function * llvm_function;
    set<Value*> rets;

    vector<Value*> args;
    vector<Value*> va_args;
    
    set<Value*> resumes;
    map<Value*, Value*> lpads; // invoke <-> lpad

    // call instructions in the function
    set<CommonCall *> callInsts; //Common CallInst/InvokeInst
    map<Value *, set<Function*>*> ci_cand_map; // call insts <-> candidate for fps
    
public:

    FunctionWrapper(Function *f);
    
    ~FunctionWrapper();

    void setCandidateFunctions(Value * ci, set<Function*>& fs);
    
    map<Value *, set<Function*>*>& getFPCallInsts();
    
    Function* getLLVMFunction();

    set<CommonCall *>& getCommonCallInsts();

    void addCommonCallInst(CommonCall * call);

    void addResume(Value * res);

    void addLandingPad(Value * invoke, Value * lpad);

    void addRet(Value * ret);

    void addArg(Value * arg);

    void addVAArg(Value* vaarg);

    vector<Value*>& getArgs();

    vector<Value*>& getVAArgs();

    set<Value*>& getReturns();

    set<Value*>& getResumes();

    Value* getLandingPad(Value * invoke);
};


#endif	/* FUNCTIONWRAPPER_H */

