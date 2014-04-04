#include "Transformer4CanaryRecord.h"

#define POINTER_BIT_SIZE ptrsize*8
#define INT_BIT_SIZE 32

#define FUNCTION_ARG_TYPE Type::getVoidTy(context),Type::getIntNTy(context,INT_BIT_SIZE),(Type*)0
#define FUNCTION_LDST_ARG_TYPE Type::getVoidTy(context),Type::getIntNTy(context,INT_BIT_SIZE),Type::getIntNTy(context,INT_BIT_SIZE),(Type*)0
#define FUNCTION_WAIT_ARG_TYPE Type::getVoidTy(context),Type::getIntNTy(context,INT_BIT_SIZE),Type::getIntNTy(context,POINTER_BIT_SIZE),Type::getIntNTy(context,POINTER_BIT_SIZE),(Type*)0
#define FUNCTION_FORK_ARG_TYPE Type::getVoidTy(context),Type::getIntNTy(context,POINTER_BIT_SIZE),(Type*)0

int Transformer4CanaryRecord::stmt_idx = 0;

Transformer4CanaryRecord::Transformer4CanaryRecord(Module* m, set<Value*>* svs, unsigned psize) : Transformer(m, svs, psize) {
    int idx = 0;
    set<Value*>::iterator it = sharedVariables->begin();
    while (it != sharedVariables->end()) {
        sv_idx_map.insert(pair<Value *, int>(*it, idx++));
        it++;
    }

    ///initialize functions
    // do not remove context, it is used in the macro FUNCTION_ARG_TYPE
    LLVMContext& context = m->getContext();

    F_init = cast<Function>(m->getOrInsertFunction("OnInit", FUNCTION_ARG_TYPE));
    F_exit = cast<Function>(m->getOrInsertFunction("OnExit", FUNCTION_ARG_TYPE));

    //F_thread_init = cast<Function>(m->getOrInsertFunction("OnThreadInit", FUNCTION_ARG_TYPE));
    //F_thread_exit = cast<Function>(m->getOrInsertFunction("OnThreadExit", FUNCTION_ARG_TYPE));

    F_preload = cast<Function>(m->getOrInsertFunction("OnPreLoad", FUNCTION_LDST_ARG_TYPE));
    F_load = cast<Function>(m->getOrInsertFunction("OnLoad", FUNCTION_LDST_ARG_TYPE));

    F_prestore = cast<Function>(m->getOrInsertFunction("OnPreStore", FUNCTION_LDST_ARG_TYPE));
    F_store = cast<Function>(m->getOrInsertFunction("OnStore", FUNCTION_LDST_ARG_TYPE));

    F_prelock = cast<Function>(m->getOrInsertFunction("OnPreLock", FUNCTION_ARG_TYPE));
    F_lock = cast<Function>(m->getOrInsertFunction("OnLock", FUNCTION_ARG_TYPE));

    F_preunlock = cast<Function>(m->getOrInsertFunction("OnPreUnlock", FUNCTION_ARG_TYPE));
    F_unlock = cast<Function>(m->getOrInsertFunction("OnUnlock", FUNCTION_ARG_TYPE));

    F_prefork = cast<Function>(m->getOrInsertFunction("OnPreFork", FUNCTION_ARG_TYPE));
    F_fork = cast<Function>(m->getOrInsertFunction("OnFork", FUNCTION_FORK_ARG_TYPE));

    F_prejoin = cast<Function>(m->getOrInsertFunction("OnPreJoin", FUNCTION_ARG_TYPE));
    F_join = cast<Function>(m->getOrInsertFunction("OnJoin", FUNCTION_ARG_TYPE));

    F_prenotify = cast<Function>(m->getOrInsertFunction("OnPreNotify", FUNCTION_ARG_TYPE));
    F_notify = cast<Function>(m->getOrInsertFunction("OnNotify", FUNCTION_ARG_TYPE));

    F_prewait = cast<Function>(m->getOrInsertFunction("OnPreWait", FUNCTION_ARG_TYPE));
    F_wait = cast<Function>(m->getOrInsertFunction("OnWait", FUNCTION_WAIT_ARG_TYPE));
}

bool Transformer4CanaryRecord::debug() {
    return false;
}

void Transformer4CanaryRecord::beforeTransform(AliasAnalysis& AA) {
}

void Transformer4CanaryRecord::afterTransform(AliasAnalysis& AA) {
    Function * mainFunction = module->getFunction("main");
    if (mainFunction != NULL) {
        ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), sharedVariables->size());
        this->insertCallInstAtHead(mainFunction, F_init, tmp, NULL);
        this->insertCallInstAtTail(mainFunction, F_exit, tmp, NULL);
    }
}

bool Transformer4CanaryRecord::functionToTransform(Function* f) {
    return !f->isIntrinsic() && !f->empty() && !this->isInstrumentationFunction(f);
}

bool Transformer4CanaryRecord::blockToTransform(BasicBlock* bb) {
    return true;
}

bool Transformer4CanaryRecord::instructionToTransform(Instruction* ins) {
    return true;
}

void Transformer4CanaryRecord::transformLoadInst(LoadInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);
    ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

    this->insertCallInstBefore(inst, F_preload, tmp, debug_idx, NULL);
    this->insertCallInstAfter(inst, F_load, tmp, debug_idx, NULL);
}

void Transformer4CanaryRecord::transformStoreInst(StoreInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);
    ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

    this->insertCallInstBefore(inst, F_prestore, tmp, debug_idx, NULL);
    this->insertCallInstAfter(inst, F_store, tmp, debug_idx, NULL);
}

void Transformer4CanaryRecord::transformPthreadCreate(CallInst* ins, AliasAnalysis& AA) {
    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), -1);
    this->insertCallInstBefore(ins, F_prefork, tmp, NULL);

    CastInst* c = CastInst::CreatePointerCast(ins->getArgOperand(0), Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE));
    c->insertAfter(ins);
    this->insertCallInstAfter(c, F_fork, c, NULL);
}

void Transformer4CanaryRecord::transformPthreadJoin(CallInst* ins, AliasAnalysis& AA) {
    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), -1);

    this->insertCallInstBefore(ins, F_prejoin, tmp, NULL);
    this->insertCallInstAfter(ins, F_join, tmp, NULL);
}

void Transformer4CanaryRecord::transformPthreadMutexLock(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);

    this->insertCallInstBefore(ins, F_prelock, tmp, NULL);
    this->insertCallInstAfter(ins, F_lock, tmp, NULL);
}

void Transformer4CanaryRecord::transformPthreadMutexUnlock(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);

    this->insertCallInstBefore(ins, F_preunlock, tmp, NULL);
    this->insertCallInstAfter(ins, F_unlock, tmp, NULL);
}

void Transformer4CanaryRecord::transformPthreadCondWait(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);
    this->insertCallInstBefore(ins, F_prewait, tmp, NULL);

    // F_wait need two more arguments
    CastInst* c1 = CastInst::CreatePointerCast(ins->getArgOperand(0), Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE));
    CastInst* c2 = CastInst::CreatePointerCast(ins->getArgOperand(1), Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE));
    c1->insertAfter(ins);
    c2->insertAfter(c1);

    this->insertCallInstAfter(c2, F_wait, tmp, c1, c2, NULL);
}

void Transformer4CanaryRecord::transformPthreadCondTimeWait(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);
    this->insertCallInstBefore(ins, F_prewait, tmp, NULL);

    // F_wait need two more arguments
    CastInst* c1 = CastInst::CreatePointerCast(ins->getArgOperand(0), Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE));
    CastInst* c2 = CastInst::CreatePointerCast(ins->getArgOperand(1), Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE));
    c1->insertAfter(ins);
    c2->insertAfter(c1);

    this->insertCallInstAfter(c2, F_wait, tmp, c1, c2, NULL);
}

void Transformer4CanaryRecord::transformPthreadCondSignal(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);

    this->insertCallInstBefore(ins, F_prenotify, tmp, NULL);
    this->insertCallInstAfter(ins, F_notify, tmp, NULL);
}

void Transformer4CanaryRecord::transformSystemExit(CallInst* ins, AliasAnalysis& AA) {
    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), sharedVariables->size());
    this->insertCallInstBefore(ins, F_exit, tmp, NULL);
}

void Transformer4CanaryRecord::transformMemCpyMov(CallInst* call, AliasAnalysis& AA) {
    Value * dst = call->getArgOperand(0);
    Value * src = call->getArgOperand(1);
    int svIdx_dst = this->getValueIndex(dst, AA);
    int svIdx_src = this->getValueIndex(src, AA);
    if (svIdx_dst == -1 && svIdx_src == -1) {
        return;
    } else if (svIdx_dst != -1 && svIdx_src != -1) {
        if (svIdx_dst > svIdx_src) {
            ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx_dst);
            ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

            this->insertCallInstBefore(call, F_prestore, tmp, debug_idx, NULL);
            this->insertCallInstAfter(call, F_store, tmp, debug_idx, NULL);

            tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx_src);
            debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

            this->insertCallInstBefore(call, F_preload, tmp, debug_idx, NULL);
            this->insertCallInstAfter(call, F_load, tmp, debug_idx, NULL);

        } else if (svIdx_dst < svIdx_src) {
            ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx_src);
            ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);


            tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx_dst);
            debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);
            this->insertCallInstBefore(call, F_prestore, tmp, debug_idx, NULL);
            this->insertCallInstAfter(call, F_store, tmp, debug_idx, NULL);
        } else {
            ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx_src);
            ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

            this->insertCallInstBefore(call, F_preload, tmp, debug_idx, NULL);
            this->insertCallInstAfter(call, F_load, tmp, debug_idx, NULL);
        }

        return;
    } else if (svIdx_dst != -1) {
        int svIdx = svIdx_dst;
        ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);
        ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

        this->insertCallInstBefore(call, F_prestore, tmp, debug_idx, NULL);
        this->insertCallInstAfter(call, F_store, tmp, debug_idx, NULL);
        return;
    } else {
        ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx_src);
        ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

        this->insertCallInstBefore(call, F_preload, tmp, debug_idx, NULL);
        this->insertCallInstAfter(call, F_load, tmp, debug_idx, NULL);
        return;
    }
}

void Transformer4CanaryRecord::transformMemSet(CallInst* call, AliasAnalysis& AA) {
    Value * val = call->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);
    ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

    this->insertCallInstBefore(call, F_prestore, tmp, debug_idx, NULL);
    this->insertCallInstAfter(call, F_store, tmp, debug_idx, NULL);
}

void Transformer4CanaryRecord::transformSpecialFunctionCall(CallInst* call, AliasAnalysis& AA) {
    // do not instrument lib calls
}

void Transformer4CanaryRecord::transformSpecialFunctionInvoke(InvokeInst* call, AliasAnalysis& AA) {
    // do not instrument lib invokes
}

bool Transformer4CanaryRecord::isInstrumentationFunction(Function * called){
    return called == F_init || called == F_exit
            || called == F_preload || called == F_load
            || called == F_prestore || called == F_store
            || called == F_preunlock || called == F_unlock
            || called == F_prelock || called == F_lock
            || called == F_prefork || called == F_fork
            || called == F_prejoin || called == F_join
            || called == F_prenotify || called == F_notify
            || called == F_prewait || called == F_wait;
}

// private functions

int Transformer4CanaryRecord::getValueIndex(Value* v, AliasAnalysis & AA) {
    set<Value*>::iterator it = sharedVariables->begin();
    while (it != sharedVariables->end()) {
        Value * rep = *it;
        if (AA.alias(v, rep) != AliasAnalysis::NoAlias) {
            return sv_idx_map[rep];
        }
        it++;
    }

    return -1;
}
