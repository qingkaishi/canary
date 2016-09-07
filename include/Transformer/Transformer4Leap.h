/* 
 * File:   Transformer4Leap.h
 *
 * Created on December 3, 2013, 10:30 AM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef TRANSFORMER4LEAP_H
#define	TRANSFORMER4LEAP_H

#include "Transformer.h"

class Transformer4Leap : public Transformer, public ModulePass {
private:
    Function *F_preload, *F_load, *F_prestore, *F_store;
    Function *F_prelock, *F_lock, *F_preunlock, *F_unlock;
    Function *F_prefork, *F_fork, *F_prejoin, *F_join;
    Function *F_prewait, *F_wait, *F_prenotify, *F_notify;
    Function *F_init, *F_exit/*, *F_thread_init, *F_thread_exit*/;
private:
    static int stmt_idx;

private:
    size_t ptrsize; // = sizeof(int*)
    std::vector<const set<Value*>*> sharedVariables;

public:
    static char ID;

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequiredTransitive<DyckAliasAnalysis>(); // required alias analysis transitively 
        AU.addRequired<TargetLibraryInfo>();
        AU.addRequired<DataLayoutPass>();
    }

public:
    Transformer4Leap() ;
    virtual bool runOnModule(Module &M);
    virtual void beforeTransform(Module* module, AliasAnalysis& AA);
    virtual void afterTransform(Module* module, AliasAnalysis& AA);
    virtual bool functionToTransform(Module* module, Function * f);
    virtual bool blockToTransform(Module* module, BasicBlock * bb);
    virtual bool instructionToTransform(Module* module, Instruction * ins);
    virtual void transformLoadInst(Module* module, LoadInst* ins, AliasAnalysis& AA);
    virtual void transformStoreInst(Module* module, StoreInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadCreate(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadJoin(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadMutexLock(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadMutexUnlock(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadCondWait(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadCondTimeWait(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadCondSignal(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual void transformSystemExit(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual void transformMemCpyMov(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual void transformMemSet(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual void transformOtherFunctionCalls(Module* module, CallInst* ins, AliasAnalysis& AA);
    virtual bool isInstrumentationFunction(Module* module, Function *f);

    virtual bool debug();

private:

    int getValueIndex(Module* module, Value * v, AliasAnalysis& AA);

};



#endif	/* TRANSFORMER4LEAP_H */

