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
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DebugInfo.h"

#include "DyckAliasAnalysis.h"

#include <stdarg.h>

#include <vector>
#include <set>
#include <map>

using namespace std;
using namespace llvm;

class Transformer  {
public:
    void transform(Module* module, AliasAnalysis* AAptr);

    /// the var agrs are the arguments of the call inst.
    /// the last one must be NULL, otherwise the behavior is not defined.
    CallInst* insertCallInstBefore(Instruction* beforeInst, Function* tocall, ...);
    CallInst* insertCallInstAfter(Instruction* afterInst, Function* tocall, ...);
    CallInst* insertCallInstAtHead(Function* theFunc, Function * tocall, ...);
    CallInst* insertCallInstAtTail(Function* theFunc, Function * tocall, ...);

public:

    virtual ~Transformer() {
    }

    virtual void beforeTransform(Module* module, AliasAnalysis& AA) {
    }

    virtual void afterTransform(Module* module, AliasAnalysis& AA) {
    }

    virtual bool functionToTransform(Module* module, Function * f) {
        return false;
    }

    virtual bool blockToTransform(Module* module, BasicBlock * bb) {
        return false;
    }

    virtual bool instructionToTransform(Module* module, Instruction * ins) {
        return false;
    }

    virtual void transformLoadInst(Module* module, LoadInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformStoreInst(Module* module, StoreInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadCreate(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadJoin(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadMutexInit(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadMutexLock(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadMutexUnlock(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadCondWait(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadCondSignal(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformPthreadCondTimeWait(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformOtherPthreadFunctions(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformSystemExit(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformMemCpyMov(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformMemSet(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformOtherFunctionCalls(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformOtherIntrinsics(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformAddressInit(Module* module, CallInst* ins, AliasAnalysis& AA) {
    }

    virtual void transformAllocaInst(Module* module, AllocaInst* alloca, Instruction* firstNotAlloca, AliasAnalysis& AA) {
    }

    virtual void transformAtomicCmpXchgInst(Module* module, AtomicCmpXchgInst* inst, AliasAnalysis& AA) {
    }

    virtual void transformAtomicRMWInst(Module* module, AtomicRMWInst* inst, AliasAnalysis& AA) {
    }

    virtual void transformVAArgInst(Module* module, VAArgInst* inst, AliasAnalysis& AA) {
    }

    virtual void transformVACpy(Module* module, CallInst* inst, AliasAnalysis& AA) {
    }

    virtual bool isInstrumentationFunction(Module* module, Function *f) {
        return false;
    }

private:
    // all calls have been lowered to invokes
    bool handleCalls(Module* module, CallInst * call, Function* calledFunction, AliasAnalysis& AA);
};

#endif	/* TRANSFORMER_H */

