#include "Transformer4CanaryRecord.h"

#define POINTER_BIT_SIZE ptrsize*8

#define VOID_TY(m) Type::getVoidTy(m->getContext())
#define INT_N_TY(m,n) Type::getIntNTy(m->getContext(),n)

#define VOID_PTR_TY(m) Type::getInt8PtrTy(m->getContext())
#define THREAD_PTR_TY(m) Type::getIntNPtrTy(m->getContext(), POINTER_BIT_SIZE)
#define INT_TY(m) INT_N_TY(m,32)
#define LONG_TY(m) INT_N_TY(m, POINTER_BIT_SIZE)
#define BOOL_TY(m) INT_N_TY(m, 1)

#define MUTEX_TY(m) m->getTypeByName("union.pthread_mutex_t")
#define CONDN_TY(m) m->getTypeByName("union.pthread_cond_t")

#define MUTEX_PTR_TY(m) PointerType::get(MUTEX_TY(m), 0)
#define CONDN_PTR_TY(m) PointerType::get(CONDN_TY(m), 0)

#define ADD_STACK(m) ConstantInt::get(LONG_TY(m), 1000)
#define ADD_HEAP(m) ConstantInt::get(LONG_TY(m), 999)
#define ADD_GLOBAL(m) ConstantInt::get(LONG_TY(m), 998)

static int extern_lib_num = 0;

static bool IS_MUTEX_INIT_TY(Type* t, Module* m) {
    // PTHREAD_MUTEX_INITIALIZER
    // { { i32, i32, i32, i32, i32, { %struct.anon } } }
    if (t->isStructTy()) {
        StructType* st = (StructType*) t;
        if (st->getNumElements() == 1) {
            Type * et = st->getElementType(0);
            if (et->isStructTy()) {
                StructType* est = (StructType*) et;
                if (est->getNumElements() == 6) {
                    for (unsigned i = 0; i < 5; i++) {
                        if (!est->getElementType(i)->isIntegerTy()) {
                            return false;
                        }
                    }

                    Type * sixest = est->getElementType(5);
                    if (sixest->isStructTy()) {
                        StructType* esixest = (StructType*) sixest;
                        if (esixest->getNumElements() == 1) {
                            StructType* anonty = cast<StructType>(esixest->getElementType(0));
                            if (anonty != NULL && anonty->getNumElements() == 2) {
                                if (anonty->getElementType(0)->isIntegerTy()
                                        && anonty->getElementType(1)->isIntegerTy()) {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

int Transformer4CanaryRecord::stmt_idx = 0;

Transformer4CanaryRecord::Transformer4CanaryRecord(Module* m, set<Value*>* svs, unsigned psize) : Transformer(m, svs, psize) {
    initializeFunctions(m);
}

Transformer4CanaryRecord::Transformer4CanaryRecord(Module * m, set<Value*> * svs, set<Value*> * lvs, unsigned psize) : Transformer(m, svs, psize) {
    //Transformer4CanaryRecord::Transformer4CanaryRecord(m, svs, psize);
    initializeFunctions(m);

    this->local_variables = lvs;
}

void Transformer4CanaryRecord::beforeTransform(AliasAnalysis& AA) {
}

void Transformer4CanaryRecord::afterTransform(AliasAnalysis& AA) {
    Function * mainFunction = module->getFunction("main");
    if (mainFunction != NULL) {
        ConstantInt* tmp = ConstantInt::get(INT_TY(module), sv_idx_map.size());
        ConstantInt* tmp2 = ConstantInt::get(INT_TY(module), lv_idx_map.size());

        this->insertCallInstAtHead(mainFunction, F_init, tmp, tmp2, NULL);
        this->insertCallInstAtTail(mainFunction, F_exit, NULL);

        outs() << "[INFO] Shared memory groups number: " << sv_idx_map.size() << "\n";
        outs() << "[INFO] Critical local memory groups number: " << lv_idx_map.size() - extern_lib_num << "\n";
        outs() << "[INFO] External function groups number: " << extern_lib_num << "\n";

        iplist<GlobalVariable>::iterator git = module->global_begin();
        while (git != module->global_end()) {
            GlobalVariable& gv = *git;
            if (!gv.isThreadLocal() && IS_MUTEX_INIT_TY(gv.getType()->getPointerElementType(), module)) {
                outs() << "[INFO] Find a global mutex..." << gv << "\n";
                Constant* mutexstar = ConstantExpr::getBitCast(&gv, MUTEX_PTR_TY(module));
                ConstantInt* tmp = ConstantInt::get(BOOL_TY(module), 0);
                this->insertCallInstAtHead(mainFunction, F_mutexinit, mutexstar, tmp, NULL);
            }

            if (!gv.hasPrivateLinkage() && !gv.getName().startswith("llvm.")) {
                size_t size = (size_t) AA.getTypeStoreSize(gv.getType()->getPointerElementType());
                ConstantInt* sizeValue = ConstantInt::get(LONG_TY(module), size);
                ConstantInt* nValue = ConstantInt::get(LONG_TY(module), 1);

                Constant* globalstar = ConstantExpr::getBitCast(&gv, VOID_PTR_TY(module));
                this->insertCallInstAtHead(mainFunction, F_address_init, globalstar, sizeValue, nValue, ADD_GLOBAL(module), NULL);
            }

            git++;
        }

    } else {
        errs() << "[ERROR] Cannot find main function...\n";
        exit(1);
    }
}

bool Transformer4CanaryRecord::functionToTransform(Function* f) {
    return !f->isIntrinsic() && !f->empty()
            && !this->isInstrumentationFunction(f);
}

bool Transformer4CanaryRecord::blockToTransform(BasicBlock* bb) {
    return true;
}

bool Transformer4CanaryRecord::instructionToTransform(Instruction* ins) {
    return true;
}

static set<Function*> foralloca;

void Transformer4CanaryRecord::transformAllocaInst(AllocaInst* alloca, Instruction* firstNotAlloca, AliasAnalysis& AA) {
    /*if (getSharedValueIndex(alloca, AA) == -1 && getLocalValueIndex(alloca, AA) == -1) {
        return;
    }*/

    /* We instrument the first alloca inst of every function, because we do not know which one will be executed first.
     * 
     * During record, we only record the first alloca of every thread.
     */

    Function * parent = alloca->getParent()->getParent();
    if (foralloca.count(parent)) {
        return;
    } else {
        foralloca.insert(parent);
    }

    Constant * nValue = (Constant*) alloca->getArraySize();
    if (!nValue->getType()->isIntegerTy(POINTER_BIT_SIZE)) {
        nValue = ConstantExpr::getIntegerCast(nValue, LONG_TY(module), false);
    }

    uint64_t size = AA.getTypeStoreSize(alloca->getAllocatedType());
    ConstantInt* sizeValue = ConstantInt::get(LONG_TY(module), size);

    if (alloca->getType()->getPointerElementType()->isIntegerTy(8)) {
        this->insertCallInstBefore(firstNotAlloca, F_address_init, alloca, sizeValue, nValue, ADD_STACK(module), NULL);
    } else {
        CastInst * ci = CastInst::CreatePointerCast(alloca, VOID_PTR_TY(module), "", firstNotAlloca);
        this->insertCallInstBefore(firstNotAlloca, F_address_init, ci, sizeValue, nValue, ADD_STACK(module), NULL);
    }


}

/// such instructions need not synchronize
/// in llvm, it firstly initialize a memory space
/// and then store the address to a variable
/// so, we need care about store inst

void Transformer4CanaryRecord::transformAddressInit(CallInst* inst, AliasAnalysis& AA) {
    /*if (getSharedValueIndex(inst, AA) == -1 && getLocalValueIndex(inst, AA) == -1) {
        return;
    }*/

    Instruction * tmp = inst;

    Value * sizeValue = inst->getArgOperand(0);
    Value * nValue = ConstantInt::get(LONG_TY(module), 1);
    if (inst->getCalledFunction()->getName().str() == "realloc") {
        sizeValue = inst->getArgOperand(1);
    } else if (inst->getCalledFunction()->getName().str() == "calloc") {
        nValue = inst->getArgOperand(0);
        sizeValue = inst->getArgOperand(1);
    }

    if (!sizeValue->getType()->isIntegerTy(POINTER_BIT_SIZE)) {
        CastInst* ci = CastInst::CreateIntegerCast(sizeValue, LONG_TY(module), false);
        ci->insertAfter(inst);
        sizeValue = ci;
        tmp = ci;
    }
    this->insertCallInstAfter(tmp, F_address_init, inst, sizeValue, nValue, ADD_HEAP(module), NULL);
}

void Transformer4CanaryRecord::transformVAArgInst(VAArgInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getPointerOperand();
    int svIdx = this->getSharedValueIndex(val, AA);
    if (svIdx != -1) {
        // shared memory
        Instruction* ci = NULL;
        if (inst->getType()->isPointerTy()) {
            ci = CastInst::CreatePointerCast(inst, LONG_TY(module));
        } else if (!inst->getType()->isIntegerTy()) {
            ci = CastInst::Create(Instruction::FPToUI, inst, LONG_TY(module));
        } else if (inst->getType()->getIntegerBitWidth() != POINTER_BIT_SIZE) {
            ci = CastInst::CreateIntegerCast(inst, LONG_TY(module), false);
        }
        if (ci != NULL) {
            ci->insertAfter(inst);
        } else {
            ci = inst;
        }

        CastInst* addci = CastInst::CreatePointerCast(val, LONG_TY(module));
        addci->insertAfter(ci);

        ConstantInt* tmp = ConstantInt::get(INT_TY(module), svIdx);
        ConstantInt* debug_idx = ConstantInt::get(INT_TY(module), stmt_idx++);

        this->insertCallInstAfter(addci, F_load, tmp, addci, ci, debug_idx, NULL);

        return;
    }
}

void Transformer4CanaryRecord::transformLoadInst(LoadInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getOperand(0);
    int svIdx = this->getSharedValueIndex(val, AA);
    if (svIdx != -1) {
        // shared memory
        Instruction* ci = NULL;
        if (inst->getType()->isPointerTy()) {
            ci = CastInst::CreatePointerCast(inst, LONG_TY(module));
        } else if (!inst->getType()->isIntegerTy()) {
            ci = CastInst::Create(Instruction::FPToUI, inst, LONG_TY(module));
        } else if (inst->getType()->getIntegerBitWidth() != POINTER_BIT_SIZE) {
            ci = CastInst::CreateIntegerCast(inst, LONG_TY(module), false);
        }
        if (ci != NULL) {
            ci->insertAfter(inst);
        } else {
            ci = inst;
        }

        CastInst* addci = CastInst::CreatePointerCast(val, LONG_TY(module));
        addci->insertAfter(ci);

        ConstantInt* tmp = ConstantInt::get(INT_TY(module), svIdx);
        ConstantInt* debug_idx = ConstantInt::get(INT_TY(module), stmt_idx++);

        this->insertCallInstAfter(addci, F_load, tmp, addci, ci, debug_idx, NULL);

        return;
    }

    int lvIdx = this->getLocalValueIndex(val, AA);
    if (lvIdx != -1) {
        // critical local memory
        Instruction* ci = NULL;
        if (inst->getType()->isPointerTy()) {
            ci = CastInst::CreatePointerCast(inst, LONG_TY(module));
        } else if (!inst->getType()->isIntegerTy()) {
            ci = CastInst::Create(Instruction::FPToUI, inst, LONG_TY(module));
        } else if (inst->getType()->getIntegerBitWidth() != POINTER_BIT_SIZE) {
            ci = CastInst::CreateIntegerCast(inst, LONG_TY(module), false);
        }
        if (ci != NULL) {
            ci->insertAfter(inst);
        } else {
            ci = inst;
        }

        this->insertCallInstAfter(ci, F_local, ci, ConstantInt::get(INT_TY(module), lvIdx), NULL);

        return;
    }
}

void Transformer4CanaryRecord::transformStoreInst(StoreInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getOperand(1);
    int svIdx = this->getSharedValueIndex(val, AA);
    if (svIdx == -1) return;

    Value* ci = NULL;
    if (inst->getOperand(0)->getType()->isPointerTy()) {
        ci = CastInst::CreatePointerCast(inst->getOperand(0), LONG_TY(module));
    } else if (!inst->getOperand(0)->getType()->isIntegerTy()) {
        ci = CastInst::Create(Instruction::FPToUI, inst->getOperand(0), LONG_TY(module));
    } else if (inst->getOperand(0)->getType()->getIntegerBitWidth() != POINTER_BIT_SIZE) {
        ci = CastInst::CreateIntegerCast(inst->getOperand(0), LONG_TY(module), false);
    }

    CastInst* addci = CastInst::CreatePointerCast(val, LONG_TY(module));
    if (ci != NULL) {
        ((CastInst*) ci)->insertAfter(inst);
        addci->insertAfter((CastInst*) ci);
    } else {
        ci = inst->getOperand(0);
        addci->insertAfter(inst);
    }

    ConstantInt* tmp = ConstantInt::get(INT_TY(module), svIdx);
    ConstantInt* debug_idx = ConstantInt::get(INT_TY(module), stmt_idx++);

    CallInst * call = this->insertCallInstBefore(inst, F_prestore, tmp, debug_idx, NULL);
    this->insertCallInstAfter(addci, F_store, tmp, call, addci, ci, debug_idx, NULL);
}

void Transformer4CanaryRecord::transformAtomicCmpXchgInst(AtomicCmpXchgInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getPointerOperand();
    int svIdx = this->getSharedValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(INT_TY(module), svIdx);
    ConstantInt* debug_idx = ConstantInt::get(INT_TY(module), stmt_idx++);

    CallInst * call = this->insertCallInstBefore(inst, F_prestore, tmp, debug_idx, NULL);
    this->insertCallInstAfter(inst, F_store, tmp, call, ConstantInt::get(LONG_TY(module), -1), ConstantInt::get(LONG_TY(module), -1), debug_idx, NULL);
}

void Transformer4CanaryRecord::transformAtomicRMWInst(AtomicRMWInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getPointerOperand();
    int svIdx = this->getSharedValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(INT_TY(module), svIdx);
    ConstantInt* debug_idx = ConstantInt::get(INT_TY(module), stmt_idx++);

    CallInst * call = this->insertCallInstBefore(inst, F_prestore, tmp, debug_idx, NULL);
    this->insertCallInstAfter(inst, F_store, tmp, call, ConstantInt::get(LONG_TY(module), -1), ConstantInt::get(LONG_TY(module), -1), debug_idx, NULL);
}

void Transformer4CanaryRecord::transformPthreadCreate(CallInst* ins, AliasAnalysis& AA) {
    ConstantInt* tmp = ConstantInt::get(BOOL_TY(module), 0);
    this->insertCallInstBefore(ins, F_prefork, tmp, NULL);
    this->insertCallInstAfter(ins, F_fork, ins->getArgOperand(0), tmp, NULL);
}

void Transformer4CanaryRecord::transformPthreadJoin(CallInst* ins, AliasAnalysis& AA) {
    ConstantInt* tmp = ConstantInt::get(BOOL_TY(module), 0);
    this->insertCallInstAfter(ins, F_join, ins->getArgOperand(0), tmp, NULL);
}

void Transformer4CanaryRecord::transformPthreadMutexInit(CallInst* ins, AliasAnalysis& AA) {
    ConstantInt* tmp = ConstantInt::get(BOOL_TY(module), 0);
    this->insertCallInstBefore(ins, F_premutexinit, tmp, NULL);
    this->insertCallInstAfter(ins, F_mutexinit, ins->getArgOperand(0), tmp, NULL);
}

void Transformer4CanaryRecord::transformPthreadMutexLock(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getSharedValueIndex(val, AA);
    if (svIdx == -1) return;

    this->insertCallInstAfter(ins, F_lock, ins->getArgOperand(0), NULL);
}

void Transformer4CanaryRecord::transformPthreadCondWait(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getSharedValueIndex(val, AA);
    if (svIdx == -1) return;

    // F_wait need two more arguments, cond*, mutex*
    this->insertCallInstAfter(ins, F_wait, ins->getArgOperand(0), ins->getArgOperand(1), NULL);
}

void Transformer4CanaryRecord::transformPthreadCondTimeWait(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getSharedValueIndex(val, AA);
    if (svIdx == -1) return;

    // F_wait need two more arguments, cond*, mutex*
    this->insertCallInstAfter(ins, F_wait, ins->getArgOperand(0), ins->getArgOperand(1), NULL);
}

void Transformer4CanaryRecord::transformSystemExit(CallInst* ins, AliasAnalysis& AA) {
    this->insertCallInstBefore(ins, F_exit, NULL);
}

void Transformer4CanaryRecord::transformMemCpyMov(CallInst* inst, AliasAnalysis& AA) {
    //this->insertCallInstAfter(inst, F_invalidate_cache, NULL);
}

void Transformer4CanaryRecord::transformMemSet(CallInst* inst, AliasAnalysis& AA) {
    //this->insertCallInstAfter(inst, F_invalidate_cache, NULL);
}

void Transformer4CanaryRecord::transformSpecialFunctionCall(CallInst* inst, AliasAnalysis& AA) {
    // if it may change mem, invalidate cache
    //if (this->isUnsafeExternalLibraryCall(inst, AA))
    //this->insertCallInstAfter(inst, F_invalidate_cache, NULL);

    // extern call, record the returned value
    if (inst->doesNotReturn() || inst->getNumUses() == 0) return;

    // if it is a extern lib call, it has return value, and the return value's use number > 0 
    // record it
    bool record = false;
    Value* cv = inst->getCalledValue();
    if (isa<Function>(cv) && Transformer4CanaryRecord::isExternalLibraryFunction((Function*) cv)) {
        record = true;
    } else {
        set<Function*>::iterator fit = extern_lib_funcs.begin();
        while (fit != extern_lib_funcs.end()) {
            Function* f = *fit;
            if (AA.alias(f, cv) && !f->getReturnType()->isVoidTy()) {
                cv = f;
                record = true;
                break;
            }
            fit++;
        }
    }

    if (record) {
        // get index
        int index = -1;
        if (lv_idx_map.count(cv)) {
            index = lv_idx_map[cv];
        } else {
            extern_lib_num++;
            index = lv_idx_map.size();
            lv_idx_map.insert(pair<Value*, int>(cv, index));
        }

        // instrument it
        // cast return value to i32/64, and record
        CastInst* ci = NULL;
        if (inst->getType()->isPointerTy()) {
            ci = CastInst::CreatePointerCast(inst, LONG_TY(module));
        } else if (!inst->getType()->isIntegerTy()) {
            ci = CastInst::Create(Instruction::FPToUI, inst, LONG_TY(module));
        } else if (!inst->getType()->isIntegerTy(POINTER_BIT_SIZE)) {
            ci = CastInst::CreateIntegerCast(inst, LONG_TY(module), false);
        }
        if (ci != NULL) {
            ci->insertAfter(inst);
            this->insertCallInstAfter(ci, F_local, ci, ConstantInt::get(LONG_TY(module), index), NULL);
        } else {
            this->insertCallInstAfter(inst, F_local, inst, ConstantInt::get(LONG_TY(module), index), NULL);
        }
    }
}

bool Transformer4CanaryRecord::isInstrumentationFunction(Function * called) {
    return called == F_init || called == F_exit
            || called == F_load
            || called == F_prestore || called == F_store /*|| called == F_store_without_cache*/
            || called == F_lock
            || called == F_prefork || called == F_fork || called == F_join
            || called == F_wait
            || called == F_premutexinit || called == F_mutexinit || called == F_local /*|| called == F_invalidate_cache*/;
}

// private functions

int Transformer4CanaryRecord::getSharedValueIndex(Value* v, AliasAnalysis & AA) {
    v = v->stripPointerCastsNoFollowAliases();
    while (isa<GlobalAlias>(v)) {
        // aliasee can be either global or bitcast of global
        v = ((GlobalAlias*) v)->getAliasee()->stripPointerCastsNoFollowAliases();
    }

    if (isa<GlobalVariable>(v) && ((GlobalVariable*) v)->isConstant()) {
        return -1;
    }

    set<Value*>::iterator it = sharedVariables->begin();
    while (it != sharedVariables->end()) {
        Value * rep = *it;
        if (AA.alias(v, rep) != AliasAnalysis::NoAlias) {
            if (sv_idx_map.count(rep)) {
                return sv_idx_map[rep];
            } else {
                int idx = sv_idx_map.size();
                sv_idx_map.insert(pair<Value*, int>(rep, idx));
                return idx;
            }
        }
        it++;
    }

    return -1;
}

int Transformer4CanaryRecord::getLocalValueIndex(Value* v, AliasAnalysis& AA) {
    if (v->getType()->isPointerTy()) {
        v = v->stripPointerCastsNoFollowAliases();
    }

    while (isa<GlobalAlias>(v)) {
        // aliasee can be either global or bitcast of global
        v = ((GlobalAlias*) v)->getAliasee()->stripPointerCastsNoFollowAliases();
    }

    if (isa<GlobalVariable>(v) && ((GlobalVariable*) v)->isConstant()) {
        return -1;
    }

    if (getSharedValueIndex(v, AA) != -1) {
        return -1;
    }

    set<Value*>::iterator it = local_variables->begin();
    while (it != local_variables->end()) {
        Value * rep = *it;
        if (AA.alias(v, rep) != AliasAnalysis::NoAlias) {
            if (lv_idx_map.count(rep)) {
                return lv_idx_map[rep];
            } else {
                int idx = lv_idx_map.size();
                lv_idx_map.insert(pair<Value*, int>(rep, idx));
                return idx;
            }
        }
        it++;
    }
    return -1;
}

void Transformer4CanaryRecord::initializeFunctions(Module * m) {
    if (m->getFunction("posix_memalign") != NULL) {
        errs() << "[ERROR] please handle posix_memalign\n";
        exit(1);
    }

    if (m->getFunction("aligned_alloc") != NULL) {
        errs() << "[ERROR] please handle aligned_alloc\n";
        exit(1);
    }

    if (m->getFunction("valloc") != NULL) {
        errs() << "[ERROR] please handle valloc\n";
        exit(1);
    }

    if (m->getFunction("memalign") != NULL) {
        errs() << "[ERROR] please handle memalign\n";
        exit(1);
    }

    if (m->getFunction("pvalloc") != NULL) {
        errs() << "[ERROR] please handle pvalloc\n";
        exit(1);
    }

    if (m->getFunction("brk") != NULL || m->getFunction("sbrk") != NULL) {
        errs() << "[ERROR] please handle brk and sbrk\n";
        exit(1);
    }

    /// extern lib functions
    for (ilist_iterator<Function> iterF = m->getFunctionList().begin(); iterF != m->getFunctionList().end(); iterF++) {
        Function* f = iterF;
        if (Transformer4CanaryRecord::isExternalLibraryFunction(f) /*&& !this->isInstrumentationFunction(f)*/) {
            extern_lib_funcs.insert(f);
        }
    }

    ///initialize functions
    F_init = cast<Function>(m->getOrInsertFunction("OnInit",
            VOID_TY(m),
            INT_TY(m), INT_TY(m),
            NULL));

    F_exit = cast<Function>(m->getOrInsertFunction("OnExit",
            VOID_TY(m),
            NULL));

    F_load = cast<Function>(m->getOrInsertFunction("OnLoad",
            VOID_TY(m),
            INT_TY(m), LONG_TY(m), LONG_TY(m), INT_TY(m),
            NULL));

    F_prestore = cast<Function>(m->getOrInsertFunction("OnPreStore",
            INT_TY(m),
            INT_TY(m), INT_TY(m),
            NULL));

    F_store = cast<Function>(m->getOrInsertFunction("OnStore",
            VOID_TY(m),
            INT_TY(m), INT_TY(m), LONG_TY(m), LONG_TY(m), INT_TY(m),
            NULL));

    /*F_store_without_cache = cast<Function>(m->getOrInsertFunction("OnStoreNoCache",
            VOID_TY(m),
            INT_TY(m), INT_TY(m), INT_TY(m),
            NULL));*/

    F_prefork = cast<Function>(m->getOrInsertFunction("OnPreFork",
            VOID_TY(m),
            BOOL_TY(m),
            NULL));

    F_fork = cast<Function>(m->getOrInsertFunction("OnFork",
            VOID_TY(m),
            THREAD_PTR_TY(m), BOOL_TY(m),
            NULL));

    F_join = cast<Function>(m->getOrInsertFunction("OnJoin",
            VOID_TY(m),
            LONG_TY(m), BOOL_TY(m),
            NULL));

    F_address_init = cast<Function>(m->getOrInsertFunction("OnAddressInit",
            VOID_TY(m),
            VOID_PTR_TY(m), LONG_TY(m), LONG_TY(m), INT_TY(m), // ptr, size, n, type
            NULL));

    F_local = cast<Function>(m->getOrInsertFunction("OnLocal",
            VOID_TY(m),
            LONG_TY(m), INT_TY(m), // value, index
            NULL));

    /*F_invalidate_cache = cast<Function>(m->getOrInsertFunction("InvalidateCache",
            VOID_TY(m), // value, index
            NULL));*/

    if (MUTEX_TY(m) != NULL) {
        F_lock = cast<Function>(m->getOrInsertFunction("OnLock",
                VOID_TY(m),
                MUTEX_PTR_TY(m),
                NULL));

        F_premutexinit = cast<Function>(m->getOrInsertFunction("OnPreMutexInit",
                VOID_TY(m),
                BOOL_TY(m),
                NULL));

        F_mutexinit = cast<Function>(m->getOrInsertFunction("OnMutexInit",
                VOID_TY(m),
                MUTEX_PTR_TY(m), BOOL_TY(m),
                NULL));

        if (CONDN_TY(m) != NULL) {
            F_wait = cast<Function>(m->getOrInsertFunction("OnWait",
                    VOID_TY(m),
                    CONDN_PTR_TY(m), MUTEX_PTR_TY(m),
                    NULL));
        } else {
            outs() << "[INFO] No pthread_cond_t type is detected, which means no wait/signal.\n";
        }
    } else {
        outs() << "[INFO] No pthread_mutex_t type is detected, which means no synchronization.\n";
    }
}
