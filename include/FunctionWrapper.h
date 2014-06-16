/*
 * This class is used for inter-procedure analysis
 * 
 * Created on November 9, 2013, 12:35 PM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef FUNCTIONWRAPPER_H
#define	FUNCTIONWRAPPER_H
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
//#include "llvm/Constants.h"
//#include "llvm/DerivedTypes.h"
//#include "llvm/Function.h"
//#include "llvm/GlobalAlias.h"
//#include "llvm/GlobalVariable.h"
//#include "llvm/Instructions.h"
//#include "llvm/IntrinsicInst.h"
//#include "llvm/LLVMContext.h"
//#include "llvm/Operator.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/ValueTracking.h"
//#include "llvm/DataLayout.h"
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

class Call {
public:
    Value* ret;
    Value* calledValue;
    vector<Value*> args;
    
    Call(Value* ret, Value * cv, vector<Value*>* args);
};

class CommonCall : public Call{
public:
    CommonCall(Value* ret, Function * f, vector<Value*>* args);
};

class PointerCall : public Call{
public:
    set<Function*> calleeCands;
    set<Function*> handledCallees;
    
    bool handled;

    PointerCall(Value* ret, Value* cv, set<Function *>* fs, vector<Value*>* args);
};

class FunctionWrapper {
private:
    int idx;

    Function * llvm_function;
    set<Value*> rets;

    vector<Value*> args;
    vector<Value*> va_args;

    set<Value*> resumes;
    map<Value*, Value*> lpads; // invoke <-> lpad

    // call instructions in the function
    set<CommonCall *> commonCalls; // common calls
    set<PointerCall*> pointerCalls; // pointer calls

    set<CommonCall *> commonCallsForCG;
    set<PointerCall *> pointerCallsForCG;

private:
    static int global_idx;

public:

    FunctionWrapper(Function *f);

    ~FunctionWrapper();

    int getIndex();

    Function* getLLVMFunction();

    set<CommonCall *>& getCommonCalls();

    void addCommonCall(CommonCall * call);

    set<PointerCall *>& getPointerCalls();

    void addPointerCall(PointerCall * call);

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

    set<PointerCall *>* getPointerCallsForCG();

    set<CommonCall *>* getCommonCallsForCG();
};


#endif	/* FUNCTIONWRAPPER_H */

