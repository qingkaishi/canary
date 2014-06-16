/* 
 * File:   Transformer4Replay.h
 *
 * Created on December 3, 2013, 10:30 AM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef TRANSFORMER4LEAP_H
#define	TRANSFORMER4LEAP_H

#include "Transformer.h"

class Transformer4Leap : public Transformer{
private:
    map<Value *, int> sv_idx_map;

    Function *F_preload, *F_load, *F_prestore, *F_store;
    Function *F_prelock, *F_lock, *F_preunlock, *F_unlock;
    Function *F_prefork, *F_fork, *F_prejoin, *F_join;
    Function *F_prewait, *F_wait, *F_prenotify, *F_notify;
    Function *F_init, *F_exit/*, *F_thread_init, *F_thread_exit*/;
private:
    static int stmt_idx;

public:

    Transformer4Leap(Module * m, set<Value*> * svs, unsigned psize);

    void transform(AliasAnalysis& AA);
    
public:
    public:
    virtual void beforeTransform(AliasAnalysis& AA) ;
    virtual void afterTransform(AliasAnalysis& AA) ;
    virtual bool functionToTransform(Function * f) ;
    virtual bool blockToTransform(BasicBlock * bb) ;
    virtual bool instructionToTransform(Instruction * ins) ;
    virtual void transformLoadInst(LoadInst* ins, AliasAnalysis& AA) ;
    virtual void transformStoreInst(StoreInst* ins, AliasAnalysis& AA) ;
    virtual void transformPthreadCreate(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformPthreadJoin(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformPthreadMutexLock(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformPthreadMutexUnlock(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformPthreadCondWait(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformPthreadCondTimeWait(CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadCondSignal(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformSystemExit(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformMemCpyMov(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformMemSet(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformSpecialFunctionCall(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformSpecialFunctionInvoke(InvokeInst* ins, AliasAnalysis& AA);
    virtual bool isInstrumentationFunction(Function *f);
    
    virtual bool debug();

private:

    int getValueIndex(Value * v, AliasAnalysis& AA);
    
};



#endif	/* TRANSFORMER4LEAP_H */

