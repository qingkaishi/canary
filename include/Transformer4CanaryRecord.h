/* 
 * File:   Transformer4CanaryRecord.h
 *
 * Created on March 25, 2014, 17:00 PM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef TRANSFORMER4CANARYRECORD_H
#define	TRANSFORMER4CANARYRECORD_H

#include "Transformer.h"

class Transformer4CanaryRecord : public Transformer {
private:
    map<Value *, int> sv_idx_map;
    map<Value *, int> lv_idx_map;

    Function *F_load, *F_prestore, *F_store;
    Function *F_lock;
    Function *F_prefork, *F_fork;
    Function *F_premutexinit, *F_mutexinit;
    Function *F_wait;
    Function *F_init, *F_exit, *F_address_init;
    Function *F_local, *F_preexternal, *F_external;
private:
    static int stmt_idx;

    set<Function*>* extern_lib_funcs;
    set<Instruction*>* unhandled_calls;
    set<Value*>* local_variables;

public:

    Transformer4CanaryRecord(Module * m,
            set<Value*> * svs,
            unsigned psize);
    Transformer4CanaryRecord(Module * m,
            set<Value*> * svs,
            set<Value*> * lvs,
            set<Instruction*> * unhandled_calls, // function pointer calls that does not find may-aliased functions
            set<Function*> * exfuncs,
            unsigned psize);

    void transform(AliasAnalysis& AA);

public:
    virtual void beforeTransform(AliasAnalysis& AA);
    virtual void afterTransform(AliasAnalysis& AA);
    virtual bool functionToTransform(Function * f);
    virtual bool blockToTransform(BasicBlock * bb);
    virtual bool instructionToTransform(Instruction * ins);
    virtual void transformLoadInst(LoadInst* ins, AliasAnalysis& AA);
    virtual void transformStoreInst(StoreInst* ins, AliasAnalysis& AA);
    virtual void transformAtomicCmpXchgInst(AtomicCmpXchgInst* inst, AliasAnalysis& AA);
    virtual void transformAtomicRMWInst(AtomicRMWInst* inst, AliasAnalysis& AA);
    virtual void transformPthreadCreate(CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadJoin(CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadMutexInit(CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadMutexLock(CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadCondWait(CallInst* ins, AliasAnalysis& AA);
    virtual void transformPthreadCondTimeWait(CallInst* ins, AliasAnalysis& AA);
    virtual void transformSystemExit(CallInst* ins, AliasAnalysis& AA);
    virtual void transformSpecialFunctionCall(CallInst* ins, AliasAnalysis& AA);
    virtual void transformMemCpyMov(CallInst* ins, AliasAnalysis& AA);
    virtual void transformMemSet(CallInst* ins, AliasAnalysis& AA);
    virtual bool isInstrumentationFunction(Function *f);

    virtual void transformAddressInit(CallInst* ins, AliasAnalysis& AA);
    virtual void transformAllocaInst(AllocaInst* alloca, Instruction* firstNotAlloca, AliasAnalysis& AA);
    virtual void transformVAArgInst(VAArgInst* inst, AliasAnalysis& AA);

private:

    int getSharedValueIndex(Value * v, AliasAnalysis& AA);
    int getLocalValueIndex(Value * v, AliasAnalysis& AA);

    void initializeFunctions(Module * m);

    /*bool isUnsafeExternalLibraryCall(CallInst * ci, AliasAnalysis& AA) {
        Value* cv = ci->getCalledValue();
        if (isa<Function>(cv)) {
            Function *f = (Function*) cv;
            if (Transformer4CanaryRecord::isUnsafeExternalLibraryFunction(f) && !this->isInstrumentationFunction(f)) {
                return true;
            }
        } else {
            return false;
        }

        for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
            Function* f = iterF;
            if (Transformer4CanaryRecord::isUnsafeExternalLibraryFunction(f) && !this->isInstrumentationFunction(f)) {
                if (AA.alias(f, ci->getCalledValue())) {
                    return true;
                }
            }
        }

        return false;
    }*/

public:

    static bool isUnsafeExternalLibraryFunction(Function * f) {
        if (Transformer4CanaryRecord::isExternalLibraryFunction(f)
                && !f->doesNotAccessMemory()
                && !f->onlyReadsMemory()
                && !f->hasFnAttribute(Attribute::ReadOnly)
                && !f->hasFnAttribute(Attribute::ReadNone)
                && !f->getArgumentList().empty()) {
            return true;
        }

        return false;
    }

    static bool isExternalLibraryFunction(Function * f) {
        if (f->isIntrinsic()) {
            switch (f->getIntrinsicID()) {
                case Intrinsic::memmove:
                case Intrinsic::memcpy:
                case Intrinsic::memset: 
                case Intrinsic::vacopy: return true;
            }
        }

        if (!f->empty() || f->isIntrinsic())
            return false;

        std::string name = f->getName().str();
        if (name.find("pthread") == 0 || name == "exit"
                || name == "calloc" || name == "malloc"
                || name == "realloc" || name == "_Znaj"
                || name == "_ZnajRKSt9nothrow_t" || name == "_Znam"
                || name == "_ZnamRKSt9nothrow_t" || name == "_Znwj"
                || name == "_ZnwjRKSt9nothrow_t" || name == "_Znwm"
                || name == "_ZnwmRKSt9nothrow_t" 
                || name == "printf" || name == "fprintf")
            return false;

        return true;
    }
};



#endif	/* TRANSFORMER4CANARYRECORD_H */

