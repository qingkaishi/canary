/*
 * This class is used for inter-procedure analysis
 * 
 * Created on November 9, 2013, 12:35 PM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef DYCKCALLGRAPHNODE_H
#define	DYCKCALLGRAPHNODE_H
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
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

#include <set>
#include <map>
#include <vector>


using namespace llvm;
using namespace std;

class Call {
public:
    // if it has a return value, this is the return value; 
    // it may be null, because there exists some implicit calls, like those in pthread_create
    // it may be a callinst or invoke inst, currently only call inst because all invokes are lowered to call
    Value* instruction; 
    
    Value* calledValue;
    vector<Value*> args;
    
    Call(Value* inst, Value * calledValue, vector<Value*>* args);
};

class CommonCall : public Call{
public:
    CommonCall(Value* inst, Function * function, vector<Value*>* args);
};

class PointerCall : public Call{
public:
    set<Function*> mayAliasedCallees;
    bool mustAliasedPointerCall;

    PointerCall(Value* inst, Value* calledValue, vector<Value*>* args);
};

class DyckCallGraphNode {
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
    
    set<CallInst*> inlineAsms; // inline asm must be a call inst

private:
    static int global_idx;

public:

    DyckCallGraphNode(Function *f);

    ~DyckCallGraphNode();

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
    
    void addInlineAsm(CallInst* inlineAsm);
    
    set<CallInst*>& getInlineAsms();

    vector<Value*>& getArgs();

    vector<Value*>& getVAArgs();

    set<Value*>& getReturns();

    set<Value*>& getResumes();

    Value* getLandingPad(Value * invoke);

};


#endif	/* DYCKCALLGRAPHNODE_H */

