/* 
 * File:   Transformer4Dummy.h
 * Author: jack
 *
 * Created on March 25, 2014, 17:00 PM
 */

#ifndef TRANSFORMER4DUMMY_H
#define	TRANSFORMER4DUMMY_H

#include "Transformer.h"

class Transformer4Dummy : public Transformer{
private:
    map<Value *, int> sv_idx_map;

    Function *F_load, *F_prestore, *F_store;
    Function *F_lock;
    Function *F_prefork, *F_fork;
    Function *F_premutexinit, *F_mutexinit;
    Function *F_wait;
    Function *F_init, *F_exit, *F_address_init;
private:
    static int stmt_idx;
    set<Function*> ignored_funcs;

public:

    Transformer4Dummy(Module * m, set<Value*> * svs, unsigned psize);

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
    virtual void transformPthreadMutexInit(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformPthreadMutexLock(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformPthreadCondWait(CallInst* ins, AliasAnalysis& AA) ;
    virtual void transformPthreadCondTimeWait(CallInst* ins, AliasAnalysis& AA);
    virtual void transformSystemExit(CallInst* ins, AliasAnalysis& AA) ;
    virtual bool isInstrumentationFunction(Function *f);
    
    virtual void transformAddressInit(CallInst* ins, AliasAnalysis& AA);
    
private:

    int getValueIndex(Value * v, AliasAnalysis& AA);
    
};



#endif	/* TRANSFORMER4CANARYRECORD_H */

