/* 
 * File:   Transformer.h
 *
 * Created on December 19, 2013, 2:14 PM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef TRANSFORMER_H
#define	TRANSFORMER_H

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
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DebugInfo.h"

#include <stdarg.h>

#include <vector>
#include <set>
#include <map>

using namespace std;
using namespace llvm;

class Transformer {
public:
    Module * module;
    set<Value*> * sharedVariables;
    unsigned ptrsize; // = sizeof(int*)

public:

    Transformer() {
    }
    Transformer(Module * m, set<Value*> * svs, unsigned psize);

    void transform(AliasAnalysis& AA);

    /// the var agrs are the arguments of the call inst.
    /// the last one must be NULL, otherwise the behavior is not defined.
    CallInst* insertCallInstBefore(Instruction* beforeInst, Function* tocall, ...);
    CallInst* insertCallInstAfter(Instruction* afterInst, Function* tocall, ...);
    void insertCallInstAtHead(Function* theFunc, Function * tocall, ...);
    void insertCallInstAtTail(Function* theFunc, Function * tocall, ...);

public:

    virtual ~Transformer() {
    }

    virtual void beforeTransform(AliasAnalysis& AA) {
    }

    virtual void afterTransform(AliasAnalysis& AA) {
    }

    virtual bool functionToTransform(Function * f) {
        return false;
    }

    virtual bool blockToTransform(BasicBlock * bb) {
        return false;
    }

    virtual bool instructionToTransform(Instruction * ins) {
        return false;
    }

    virtual void transformLoadInst(LoadInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformStoreInst(StoreInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadCreate(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadJoin(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadMutexInit(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadMutexLock(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadMutexUnlock(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadCondWait(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadCondSignal(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadCondTimeWait(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformSystemExit(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformMemCpyMov(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformMemSet(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformSpecialFunctionCall(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformSpecialFunctionInvoke(InvokeInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformAddressInit(CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformAllocaInst(AllocaInst* alloca, Instruction* firstNotAlloca, AliasAnalysis& AA) {
    }
    
    virtual void transformAtomicCmpXchgInst(AtomicCmpXchgInst* inst,  AliasAnalysis& AA) {
    }
    
    virtual void transformAtomicRMWInst(AtomicRMWInst* inst,  AliasAnalysis& AA) {
    }
    
    virtual void transformVAArgInst(VAArgInst* inst, AliasAnalysis& AA) {
    }

    virtual bool isInstrumentationFunction(Function *f) {
        return false;
    }

private:
    bool handleCalls(CallInst * call, Function* calledFunction, AliasAnalysis& AA);
    bool handleInvokes(InvokeInst * call, Function* calledFunction, AliasAnalysis& AA);
    set<Function * >* getAliasFunctions(Value * callInst, Value * ptr, AliasAnalysis& AA, set<Function *>* ret);
    bool matchFunctionAndCall(Function * callee, Value * callInst, AliasAnalysis &aa);
};



#endif	/* TRANSFORMER_H */

