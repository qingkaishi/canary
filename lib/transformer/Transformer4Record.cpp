#include "Transformer4Record.h"

#define POINTER_BIT_SIZE ptrsize*8
#define INT_BIT_SIZE 32

#define FUNCTION_ARG_TYPE Type::getVoidTy(context),Type::getIntNTy(context,INT_BIT_SIZE),(Type*)0
#define FUNCTION_LDST_ARG_TYPE Type::getVoidTy(context),Type::getIntNTy(context,INT_BIT_SIZE),Type::getIntNTy(context,INT_BIT_SIZE),(Type*)0
#define FUNCTION_WAIT_ARG_TYPE Type::getVoidTy(context),Type::getIntNTy(context,INT_BIT_SIZE),Type::getIntNTy(context,POINTER_BIT_SIZE),Type::getIntNTy(context,POINTER_BIT_SIZE),(Type*)0
#define FUNCTION_FORK_ARG_TYPE Type::getVoidTy(context),Type::getIntNTy(context,POINTER_BIT_SIZE),(Type*)0

int Transformer4Record::stmt_idx = 0;

Transformer4Record::Transformer4Record(Module* m, set<Value*>* svs, unsigned psize) : Transformer(m, svs, psize) {
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

bool Transformer4Record::debug() {
    return false;
}

void Transformer4Record::beforeTransform(AliasAnalysis& AA) {
    outs() << "Distinguishing signal handler functions ...\n";
    
    Function * signalFunction = module->getFunction("signal");
    if(!isSignalFunctionStarType(signalFunction->getType())) {
        signalFunction = NULL;
    }
    
    Function * signalActionFunction = module->getFunction("sigaction");
    if(!isSigactFunctionStarType(signalActionFunction->getType())){
        signalActionFunction = NULL;
    }
    
    set<Value*> possibleSigHandler;

    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function& f = *iterF;
        if (!this->functionToTransform(&f)) {
            continue;
        }
        
        if(this->isSighandlerStarType((&f)->getType())){
            possibleSigHandlerFunctions.insert(&f);
        }
        
        for (ilist_iterator<BasicBlock> iterB = f.getBasicBlockList().begin(); iterB != f.getBasicBlockList().end(); iterB++) {
            BasicBlock &b = *iterB;
            if (!this->blockToTransform(&b)) {
                continue;
            }
            for (ilist_iterator<Instruction> iterI = b.getInstList().begin(); iterI != b.getInstList().end(); iterI++) {
                Instruction &inst = *iterI;

                Value * calledValue = NULL;
                vector<Value*> args;

                if (isa<CallInst>(inst)) {
                    calledValue = ((CallInst*) &inst)->getCalledValue();
                    for (unsigned i = 0; i < ((CallInst*) &inst)->getNumArgOperands(); i++) {
                        args.push_back(((CallInst*) &inst)->getArgOperand(i));
                    }
                }

                if (isa<InvokeInst>(inst)) {
                    calledValue = ((InvokeInst*) &inst)->getCalledValue();
                    for (unsigned i = 0; i < ((InvokeInst*) &inst)->getNumArgOperands(); i++) {
                        args.push_back(((CallInst*) &inst)->getArgOperand(i));
                    }
                }

                if (calledValue == NULL || !calledValue->getType()->isPointerTy()) continue;

                if (signalFunction != NULL && isSignalFunctionStarType(calledValue->getType())) {
                    if (AA.alias(calledValue, signalFunction)) {
                        if (args[1] != NULL) {
                            if (isa<Constant>(args[1])) {
                                if (!((Constant*) (args[1]))->isNullValue()) {
                                    possibleSigHandler.insert(args[1]);
                                }
                            } else {
                                possibleSigHandler.insert(args[1]);
                            }
                        }
                    }
                } else if (signalActionFunction != NULL && isSigactFunctionStarType(calledValue->getType())) {
                    if (AA.alias(calledValue, signalActionFunction)) {
                        if (args[1] != NULL && isSigactStructStarType(args[1]->getType())) {
                            // although args[1] is a pointer type that points to sigact struct
                            // it also can partially alias with the handler, the first element of
                            // sigact structure
                            possibleSigHandler.insert(args[1]);
                        }
                    }
                }
            }
        }
    }
    
    set<Function *>::iterator sit = possibleSigHandlerFunctions.begin();
    while(sit!=possibleSigHandlerFunctions.end()){
        Function * func = *sit;
        
        bool yes = false;
        set<Value*>::iterator vit = possibleSigHandler.begin();
        while(vit!=possibleSigHandler.end()){
            if(AA.alias(*vit, func)){
                yes= true;
                break;
            }
            vit++;
        }
        
        if(!yes){
            possibleSigHandlerFunctions.erase(sit++);
        }else{
            sit++;
        }
    }
    
    int idx = 0;
    sit = possibleSigHandlerFunctions.begin();
    while(sit!=possibleSigHandlerFunctions.end()){
        Function * func = *sit;
        outs() << "[" << idx << "] " << func->getName() << "\n";
    }
}

void Transformer4Record::afterTransform(AliasAnalysis& AA) {
    Function * mainFunction = module->getFunction("main");
    if (mainFunction != NULL) {
        ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), sharedVariables->size());
        this->insertCallInstAtHead(mainFunction, F_init, tmp, NULL);
        this->insertCallInstAtTail(mainFunction, F_exit, tmp, NULL);
    }
}

bool Transformer4Record::functionToTransform(Function* f) {
    return !f->isIntrinsic() && !f->empty() && !this->isInstrumentationFunction(f);
}

bool Transformer4Record::blockToTransform(BasicBlock* bb) {
    return true;
}

bool Transformer4Record::instructionToTransform(Instruction* ins) {
    return true;
}

void Transformer4Record::transformLoadInst(LoadInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);
    ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

    this->insertCallInstBefore(inst, F_preload, tmp, debug_idx, NULL);
    this->insertCallInstAfter(inst, F_load, tmp, debug_idx, NULL);
}

void Transformer4Record::transformStoreInst(StoreInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);
    ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

    this->insertCallInstBefore(inst, F_prestore, tmp, debug_idx, NULL);
    this->insertCallInstAfter(inst, F_store, tmp, debug_idx, NULL);
}

void Transformer4Record::transformPthreadCreate(CallInst* ins, AliasAnalysis& AA) {
    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), -1);
    this->insertCallInstBefore(ins, F_prefork, tmp, NULL);

    CastInst* c = CastInst::CreatePointerCast(ins->getArgOperand(0), Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE));
    c->insertAfter(ins);
    this->insertCallInstAfter(c, F_fork, c, NULL);
}

void Transformer4Record::transformPthreadJoin(CallInst* ins, AliasAnalysis& AA) {
    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), -1);

    this->insertCallInstBefore(ins, F_prejoin, tmp, NULL);
    this->insertCallInstAfter(ins, F_join, tmp, NULL);
}

void Transformer4Record::transformPthreadMutexLock(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);

    this->insertCallInstBefore(ins, F_prelock, tmp, NULL);
    this->insertCallInstAfter(ins, F_lock, tmp, NULL);
}

void Transformer4Record::transformPthreadMutexUnlock(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);

    this->insertCallInstBefore(ins, F_preunlock, tmp, NULL);
    this->insertCallInstAfter(ins, F_unlock, tmp, NULL);
}

void Transformer4Record::transformPthreadCondWait(CallInst* ins, AliasAnalysis& AA) {
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

void Transformer4Record::transformPthreadCondTimeWait(CallInst* ins, AliasAnalysis& AA) {
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

void Transformer4Record::transformPthreadCondSignal(CallInst* ins, AliasAnalysis& AA) {
    Value * val = ins->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);

    this->insertCallInstBefore(ins, F_prenotify, tmp, NULL);
    this->insertCallInstAfter(ins, F_notify, tmp, NULL);
}

void Transformer4Record::transformSystemExit(CallInst* ins, AliasAnalysis& AA) {
    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), sharedVariables->size());
    this->insertCallInstBefore(ins, F_exit, tmp, NULL);
}

void Transformer4Record::transformMemCpyMov(CallInst* call, AliasAnalysis& AA) {
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

void Transformer4Record::transformMemSet(CallInst* call, AliasAnalysis& AA) {
    Value * val = call->getArgOperand(0);
    int svIdx = this->getValueIndex(val, AA);
    if (svIdx == -1) return;

    ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIdx);
    ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

    this->insertCallInstBefore(call, F_prestore, tmp, debug_idx, NULL);
    this->insertCallInstAfter(call, F_store, tmp, debug_idx, NULL);
}

void Transformer4Record::transformSpecialFunctionCall(CallInst* call, AliasAnalysis& AA) {
    vector<int> svIndices;
    for (unsigned i = 0; i < call->getNumArgOperands(); i++) {
        Value * arg = call->getArgOperand(i);

        int svIdx = this->getValueIndex(arg, AA);
        if (svIdx != -1) {
            bool contained = false;
            vector<int>::iterator it = svIndices.begin();
            while (it != svIndices.end()) {
                if (*it == svIdx) {
                    contained = true;
                    break;
                }
                if (*it < svIdx) break;
                it++;
            }
            if (!contained)
                svIndices.insert(it, svIdx);
        }
    }

    if (svIndices.empty()) {
        return;
    } else {
        for (unsigned i = 0; i < svIndices.size(); i++) {
            ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIndices[i]);
            ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

            ///@FIXME
            this->insertCallInstBefore(call, F_prestore, tmp, debug_idx, NULL);
            this->insertCallInstBefore(call, F_store, tmp, debug_idx, NULL);
        }
    }
}

void Transformer4Record::transformSpecialFunctionInvoke(InvokeInst* call, AliasAnalysis& AA) {
    vector<int> svIndices;
    for (unsigned i = 0; i < call->getNumArgOperands(); i++) {
        Value * arg = call->getArgOperand(i);

        int svIdx = this->getValueIndex(arg, AA);
        if (svIdx != -1) {
            bool contained = false;
            vector<int>::iterator it = svIndices.begin();
            while (it != svIndices.end()) {
                if (*it == svIdx) {
                    contained = true;
                    break;
                }
                if (*it < svIdx) break;
                it++;
            }
            if (!contained)
                svIndices.insert(it, svIdx);
        }
    }

    if (!svIndices.empty()) {
        //BasicBlock * normal = call->getNormalDest();
        //BasicBlock * unwind = call->getUnwindDest();

        for (unsigned i = 0; i < svIndices.size(); i++) {
            ConstantInt* tmp = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), svIndices[i]);
            ConstantInt* debug_idx = ConstantInt::get(Type::getIntNTy(module->getContext(), INT_BIT_SIZE), stmt_idx++);

            ///@FIXME
            this->insertCallInstBefore(call, F_prestore, tmp, debug_idx, NULL);
            this->insertCallInstBefore(call, F_store, tmp, debug_idx, NULL);
        }
    }
}

bool Transformer4Record::isInstrumentationFunction(Function * called) {
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

int Transformer4Record::getValueIndex(Value* v, AliasAnalysis & AA) {
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


bool Transformer4Record::isSigactFunctionType(Type* v) {
    if (v != NULL && v->isFunctionTy()) {

        FunctionType * sigactfnty = (FunctionType*)v;
        if (sigactfnty->getNumParams() == 3
                && sigactfnty->getReturnType()->isIntegerTy()
                && sigactfnty->getParamType(0)->isIntegerTy()
                && isSigactStructStarType(sigactfnty->getParamType(1))
		&& isSigactStructStarType(sigactfnty->getParamType(2))) {
            return true;
        }
    }
    
    return false;
}

bool Transformer4Record::isSignalFunctionType(Type* vtype) {
    if(vtype == NULL) return false;
    
    if(vtype->isFunctionTy()) {
        FunctionType * signalfnty = (FunctionType*)vtype;
        if (signalfnty->getNumParams() == 2
                && isSighandlerStarType(signalfnty->getReturnType())
                && signalfnty->getParamType(0)->isIntegerTy()
                && isSighandlerStarType(signalfnty->getParamType(1))) {
            return true;
        }
        
        return false;
    } else {
        return false;
    }    
}

bool Transformer4Record::isSigactStructType(Type* v) {
    StructType* sigactType = module->getTypeByName("struct.sigaction");
    return v!=NULL && sigactType == v;
}

bool Transformer4Record::isSighandlerType(Type* v) {
    if(v!=NULL && v->isFunctionTy()){

            FunctionType* fnty = (FunctionType*)v;
            
            if(fnty->getNumParams() == 1 
                    && fnty->getReturnType()->isVoidTy() 
                    && fnty->getParamType(0)->isIntegerTy()){
                return true;
            }

    }
    return false;
}


bool Transformer4Record::isSigactFunctionStarType(Type* v) {
    return v != NULL && v->isPointerTy() && isSigactFunctionType(v->getPointerElementType());
}

bool Transformer4Record::isSignalFunctionStarType(Type* v) {
    return v != NULL && v->isPointerTy() && isSignalFunctionType(v->getPointerElementType());   
}

bool Transformer4Record::isSigactStructStarType(Type* v) {    
    return v != NULL && v->isPointerTy() && isSigactStructType(v->getPointerElementType());
}

bool Transformer4Record::isSighandlerStarType(Type* v) {
    return v != NULL && v->isPointerTy() && isSighandlerType(v->getPointerElementType());
}

