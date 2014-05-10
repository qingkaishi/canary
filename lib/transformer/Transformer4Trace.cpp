#include "Transformer4Trace.h"

#define POINTER_BIT_SIZE ptrsize*8

#define FUNCTION_VOID_ARG_TYPE Type::getVoidTy(context),(Type*)0
#define FUNCTION_MEM_LN_ARG_TYPE Type::getVoidTy(context),Type::getIntNPtrTy(context,POINTER_BIT_SIZE),Type::getIntNTy(context,POINTER_BIT_SIZE),Type::getInt8PtrTy(context,0),(Type*)0
#define FUNCTION_TID_LN_ARG_TYPE Type::getVoidTy(context),Type::getIntNTy(context,POINTER_BIT_SIZE),Type::getIntNTy(context,POINTER_BIT_SIZE),Type::getInt8PtrTy(context,0),(Type*)0
#define FUNCTION_2MEM_LN_ARG_TYPE Type::getVoidTy(context),Type::getIntNPtrTy(context,POINTER_BIT_SIZE),Type::getIntNPtrTy(context,POINTER_BIT_SIZE),Type::getIntNTy(context,POINTER_BIT_SIZE),Type::getInt8PtrTy(context,0),(Type*)0

Transformer4Trace::Transformer4Trace(Module* m, set<Value*>* svs, unsigned psize) : Transformer(m, svs, psize) {
    int idx = 0;
    set<Value*>::iterator it = sharedVariables->begin();
    while (it != sharedVariables->end()) {
        sv_idx_map.insert(pair<Value *, int>(*it, idx++));
        it++;
    }

    ///initialize functions
    // do not remove context, it is used in the macro FUNCTION_ARG_TYPE
    LLVMContext& context = m->getContext();

    F_init = cast<Function>(m->getOrInsertFunction("OnInit", FUNCTION_VOID_ARG_TYPE));
    F_exit = cast<Function>(m->getOrInsertFunction("OnExit", FUNCTION_VOID_ARG_TYPE));

    //F_thread_init = cast<Function>(m->getOrInsertFunction("OnThreadInit", FUNCTION_ARG_TYPE));
    //F_thread_exit = cast<Function>(m->getOrInsertFunction("OnThreadExit", FUNCTION_ARG_TYPE));

    F_preload = cast<Function>(m->getOrInsertFunction("OnPreLoad", FUNCTION_MEM_LN_ARG_TYPE));
    F_load = cast<Function>(m->getOrInsertFunction("OnLoad", FUNCTION_MEM_LN_ARG_TYPE));

    F_prestore = cast<Function>(m->getOrInsertFunction("OnPreStore", FUNCTION_MEM_LN_ARG_TYPE));
    F_store = cast<Function>(m->getOrInsertFunction("OnStore", FUNCTION_MEM_LN_ARG_TYPE));

    F_prelock = cast<Function>(m->getOrInsertFunction("OnPreLock", FUNCTION_MEM_LN_ARG_TYPE));
    F_lock = cast<Function>(m->getOrInsertFunction("OnLock", FUNCTION_MEM_LN_ARG_TYPE));

    F_preunlock = cast<Function>(m->getOrInsertFunction("OnPreUnlock", FUNCTION_MEM_LN_ARG_TYPE));
    F_unlock = cast<Function>(m->getOrInsertFunction("OnUnlock", FUNCTION_MEM_LN_ARG_TYPE));

    F_prefork = cast<Function>(m->getOrInsertFunction("OnPreFork", FUNCTION_MEM_LN_ARG_TYPE));
    F_fork = cast<Function>(m->getOrInsertFunction("OnFork", FUNCTION_MEM_LN_ARG_TYPE));

    F_prejoin = cast<Function>(m->getOrInsertFunction("OnPreJoin", FUNCTION_TID_LN_ARG_TYPE));
    F_join = cast<Function>(m->getOrInsertFunction("OnJoin", FUNCTION_TID_LN_ARG_TYPE));

    F_prenotify = cast<Function>(m->getOrInsertFunction("OnPreNotify", FUNCTION_2MEM_LN_ARG_TYPE));
    F_notify = cast<Function>(m->getOrInsertFunction("OnNotify", FUNCTION_2MEM_LN_ARG_TYPE));

    F_prewait = cast<Function>(m->getOrInsertFunction("OnPreWait", FUNCTION_2MEM_LN_ARG_TYPE));
    F_wait = cast<Function>(m->getOrInsertFunction("OnWait", FUNCTION_2MEM_LN_ARG_TYPE));
}

bool Transformer4Trace::debug() {
    return true;
}

void Transformer4Trace::beforeTransform(AliasAnalysis& AA) {
    if (debug()) {
        errs() << "Start Preprocessing...\n";
        errs().flush();
    }
    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function& f = *iterF;
        bool ignored = true;
        for (ilist_iterator<BasicBlock> iterB = f.getBasicBlockList().begin(); iterB != f.getBasicBlockList().end(); iterB++) {
            BasicBlock &b = *iterB;
            for (ilist_iterator<Instruction> iterI = b.getInstList().begin(); iterI != b.getInstList().end(); iterI++) {
                Instruction &inst = *iterI;
                MDNode* mdn = inst.getMetadata("dbg");
                DILocation LOC(mdn);
                std::string filename = LOC.getFilename().str();
//                unsigned ln = LOC.getLineNumber();
//                errs() << inst << "\n";
//                errs() << f.getName() << "\n";
//                errs() << filename; 
//                errs() << " : " << ln << "\n";
//                errs() <<"************************"<< "\n";
//                errs().flush();
                //if (filename == "pbzip2.cpp") {
                    ignored = false;
                    break;
                //}
            }
            if (!ignored) {
                break;
            }
        }

        if (ignored) {
            ignored_funcs.insert(&f);
        }
    }

    if (debug()) {
        errs() << "End Preprocessing...\n";
        errs().flush();
    }
}

void Transformer4Trace::afterTransform(AliasAnalysis& AA) {
    Function * mainFunction = module->getFunction("main");
    if (mainFunction != NULL) {
        this->insertCallInstAtHead(mainFunction, F_init, NULL);
        this->insertCallInstAtTail(mainFunction, F_exit, NULL);
    }
}

void Transformer4Trace::transformLoadInst(LoadInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    MDNode* mdn = inst->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();

    CastInst* c = CastInst::CreatePointerCast(val, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    c->insertBefore(inst);
    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(inst, F_preload, c, lnval, getSrcFileNameArg(filename), NULL);
    this->insertCallInstAfter(inst, F_load, c, lnval, getSrcFileNameArg(filename), NULL);
}

void Transformer4Trace::transformStoreInst(StoreInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getOperand(1);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    MDNode* mdn = inst->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();

    CastInst* c = CastInst::CreatePointerCast(val, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    c->insertBefore(inst);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(inst, F_prestore, c, lnval, getSrcFileNameArg(filename), NULL);
    this->insertCallInstAfter(inst, F_store, c, lnval, getSrcFileNameArg(filename), NULL);
}

void Transformer4Trace::transformPthreadCreate(CallInst* call, AliasAnalysis& AA) {
    MDNode* mdn = call->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();

    ConstantInt * lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_prefork, call->getArgOperand(0), lnval, getSrcFileNameArg(filename), NULL);
    this->insertCallInstAfter(call, F_fork, call->getArgOperand(0), lnval, getSrcFileNameArg(filename), NULL);
}

void Transformer4Trace::transformPthreadJoin(CallInst* call, AliasAnalysis& AA) {
    MDNode* mdn = call->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();

    ConstantInt * lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_prejoin, call->getArgOperand(0), lnval, getSrcFileNameArg(filename), NULL);
    this->insertCallInstAfter(call, F_join, call->getArgOperand(0), lnval, getSrcFileNameArg(filename), NULL);

}

void Transformer4Trace::transformPthreadMutexLock(CallInst* call, AliasAnalysis& AA) {
    Value * val = call->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    MDNode* mdn = call->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();

    CastInst* c = CastInst::CreatePointerCast(val, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    c->insertBefore(call);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_prelock, c, lnval, getSrcFileNameArg(filename), NULL);
    this->insertCallInstAfter(call, F_lock, c, lnval, getSrcFileNameArg(filename), NULL);
}

void Transformer4Trace::transformPthreadMutexUnlock(CallInst* call, AliasAnalysis& AA) {
    Value * val = call->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    MDNode* mdn = call->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();

    CastInst* c = CastInst::CreatePointerCast(val, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    c->insertBefore(call);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_preunlock, c, lnval, getSrcFileNameArg(filename), NULL);
    this->insertCallInstAfter(call, F_unlock, c, lnval, getSrcFileNameArg(filename), NULL);

}

void Transformer4Trace::transformPthreadCondWait(CallInst* call, AliasAnalysis& AA) {
    Value * val0 = call->getArgOperand(0);
    Value * val1 = call->getArgOperand(1);
    int svIdx0 = this->getValueIndex(val0, AA);
    int svIdx1 = this->getValueIndex(val1, AA);
    if (svIdx0 == -1 && svIdx1 == -1) return;

    MDNode* mdn = call->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();

    CastInst* cond = CastInst::CreatePointerCast(val0, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    cond->insertBefore(call);

    CastInst* mut = CastInst::CreatePointerCast(val1, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    mut->insertBefore(call);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_prewait, cond, mut, lnval, getSrcFileNameArg(filename), NULL);
    this->insertCallInstAfter(call, F_wait, cond, mut, lnval, getSrcFileNameArg(filename), NULL);

}

void Transformer4Trace::transformPthreadCondTimeWait(CallInst* ins, AliasAnalysis& AA) {
    // do nothing in this version
}

void Transformer4Trace::transformPthreadCondSignal(CallInst* call, AliasAnalysis& AA) {
    Value * val0 = call->getArgOperand(0);
    Value * val1 = call->getArgOperand(1);
    int svIdx0 = this->getValueIndex(val0, AA);
    int svIdx1 = this->getValueIndex(val1, AA);
    if (svIdx0 == -1 && svIdx1 == -1) return;

    MDNode* mdn = call->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();

    CastInst* cond = CastInst::CreatePointerCast(val0, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    cond->insertBefore(call);

    CastInst* mut = CastInst::CreatePointerCast(val1, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    mut->insertBefore(call);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_prenotify, cond, mut, lnval, getSrcFileNameArg(filename), NULL);
    this->insertCallInstAfter(call, F_notify, cond, mut, lnval, getSrcFileNameArg(filename), NULL);
}

void Transformer4Trace::transformSystemExit(CallInst* ins, AliasAnalysis& AA) {
    this->insertCallInstBefore(ins, F_exit, NULL);
}

void Transformer4Trace::transformMemCpyMov(CallInst* call, AliasAnalysis& AA) {
    MDNode* mdn = call->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    Value * dst = call->getArgOperand(0);
    Value * src = call->getArgOperand(1);
    int svIdx_dst = this->getValueIndex(dst, AA);
    int svIdx_src = this->getValueIndex(src, AA);
    if (svIdx_dst == -1 && svIdx_src == -1) {
        return;
    } else if (svIdx_dst != -1 && svIdx_src != -1) {
        CastInst* d = CastInst::CreatePointerCast(dst, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
        d->insertBefore(call);

        CastInst* s = CastInst::CreatePointerCast(src, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
        s->insertBefore(call);

        if (svIdx_dst != svIdx_src) {


            insertCallInstBefore(call, F_prestore, d, lnval, getSrcFileNameArg(filename), NULL);
            insertCallInstAfter(call, F_store, d, lnval, getSrcFileNameArg(filename), NULL);

            insertCallInstBefore(call, F_preload, s, lnval, getSrcFileNameArg(filename), NULL);
            insertCallInstAfter(call, F_load, s, lnval, getSrcFileNameArg(filename), NULL);

        } else {
            insertCallInstBefore(call, F_prestore, d, lnval, getSrcFileNameArg(filename), NULL);
            insertCallInstAfter(call, F_store, d, lnval, getSrcFileNameArg(filename), NULL);
        }

    } else if (svIdx_dst != -1) {
        CastInst* d = CastInst::CreatePointerCast(dst, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
        d->insertBefore(call);

        insertCallInstBefore(call, F_prestore, d, lnval, getSrcFileNameArg(filename), NULL);
        insertCallInstAfter(call, F_store, d, lnval, getSrcFileNameArg(filename), NULL);
    } else {
        CastInst* s = CastInst::CreatePointerCast(src, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
        s->insertBefore(call);

        insertCallInstBefore(call, F_preload, s, lnval, getSrcFileNameArg(filename), NULL);
        insertCallInstAfter(call, F_load, s, lnval, getSrcFileNameArg(filename), NULL);
    }
}

void Transformer4Trace::transformMemSet(CallInst* call, AliasAnalysis& AA) {
    MDNode* mdn = call->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    Value * val = call->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    CastInst* c = CastInst::CreatePointerCast(val, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    c->insertBefore(call);

    insertCallInstBefore(call, F_prestore, c, lnval, getSrcFileNameArg(filename), NULL);
    insertCallInstAfter(call, F_store, c, lnval, getSrcFileNameArg(filename), NULL);
}

void Transformer4Trace::transformSpecialFunctionCall(CallInst* call, AliasAnalysis& AA) {
    MDNode* mdn = call->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();
    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    for (unsigned i = 0; i < call->getNumArgOperands(); i++) {
        Value * arg = call->getArgOperand(i);

        CastInst* c = NULL;
        if (arg->getType()->isPointerTy()) {
            c = CastInst::CreatePointerCast(arg, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
            c->insertBefore(call);
        } else {
            continue;
        }

        int svIdx = this->getValueIndex(arg, AA);
        if (svIdx != -1) {
            insertCallInstBefore(call, F_prestore, c, lnval, getSrcFileNameArg(filename), NULL);
            insertCallInstBefore(call, F_store, c, lnval, getSrcFileNameArg(filename), NULL);
        }
    }
}

void Transformer4Trace::transformSpecialFunctionInvoke(InvokeInst* call, AliasAnalysis& AA) {
    MDNode* mdn = call->getMetadata("dbg");
    DILocation LOC(mdn);
    int ln = LOC.getLineNumber();
    std::string filename = LOC.getFilename().str();
    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    for (unsigned i = 0; i < call->getNumArgOperands(); i++) {
        Value * arg = call->getArgOperand(i);

        CastInst* c = NULL;
        if (arg->getType()->isPointerTy()) {
            c = CastInst::CreatePointerCast(arg, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
            c->insertBefore(call);
        } else {
            continue;
        }

        int svIdx = this->getValueIndex(arg, AA);
        if (svIdx != -1) {
            insertCallInstBefore(call, F_prestore, c, lnval, getSrcFileNameArg(filename), NULL);
            insertCallInstBefore(call, F_store, c, lnval, getSrcFileNameArg(filename), NULL);
        }
    }
}

bool Transformer4Trace::functionToTransform(Function* f) {
    return !f->empty() && ignored_funcs.find(f) == ignored_funcs.end() && !this->isInstrumentationFunction(f) && !f->isIntrinsic();
}

bool Transformer4Trace::blockToTransform(BasicBlock* bb) {
    return true;
}

bool Transformer4Trace::instructionToTransform(Instruction* inst) {
    MDNode* mdn = inst->getMetadata("dbg");
    DILocation LOC(mdn);
    std::string filename = LOC.getFilename().str();
    if (filename == ""
            || filename.find(".h") != filename.npos
            || filename.find(".tcc") != filename.npos) {
        return false;
    } else {
        //errs() << (filename) << "\n";
        //errs().flush();
        return true;
    }
}

bool Transformer4Trace::isInstrumentationFunction(Function* called) {
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

int Transformer4Trace::getValueIndex(Value* v, AliasAnalysis & AA) {
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

Value* Transformer4Trace::getSrcFileNameArg(std::string& filename) {
    std::string prefix("__trace_");
    prefix.append(filename);

    Constant* srcfile = ConstantDataArray::getString(module->getContext(), filename);
    Constant* global_srcfile = module->getOrInsertGlobal(prefix, srcfile->getType());
    ((GlobalVariable*) global_srcfile)->setUnnamedAddr(true);
    ((GlobalVariable*) global_srcfile)->setAlignment(1);
    ((GlobalVariable*) global_srcfile)->setLinkage(GlobalVariable::PrivateLinkage);
    ((GlobalVariable*) global_srcfile)->setConstant(true);
    ((GlobalVariable*) global_srcfile)->setInitializer(srcfile);

    vector<Value *> indices;
    indices.push_back(ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), 0));
    indices.push_back(ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), 0));
    return ConstantExpr::getGetElementPtr(global_srcfile, indices, true);
}
