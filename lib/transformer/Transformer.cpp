#include "Transformer.h"

Transformer::Transformer(Module* m, set<Value*>* svs, unsigned psize) {
    module = m;
    sharedVariables = svs;
    ptrsize = psize;
}

void Transformer::insertCallInstBefore(Instruction* beforeInst, Function* tocall, ...){
    std::vector<Value *> args;
    
    va_list ap; 
    va_start( ap, tocall );
    Value* arg = va_arg( ap, Value* );
    
    while(arg!=NULL){
        args.push_back(arg);
        arg = va_arg( ap, Value* );
    }
    
    va_end(ap);
    
    CallInst *InstCall = CallInst::Create(tocall, args);
    InstCall->insertBefore(beforeInst);
}

void Transformer::insertCallInstAfter(Instruction* afterInst, Function* tocall, ...){
    std::vector<Value *> args;
    
    va_list ap; 
    va_start( ap, tocall );
    Value* arg = va_arg( ap, Value* );
    
    while(arg!=NULL){
        args.push_back(arg);
        arg = va_arg( ap, Value* );
    }
    
    va_end(ap);
    
    CallInst *InstCall = CallInst::Create(tocall, args);
    InstCall->insertAfter(afterInst);
}

void Transformer::insertCallInstAtHead(Function* theFunc, Function* tocall, ...){
    std::vector<Value *> args;
    
    va_list ap; 
    va_start( ap, tocall );
    Value* arg = va_arg( ap, Value* );
    
    while(arg!=NULL){
        args.push_back(arg);
        arg = va_arg( ap, Value* );
    }
    
    va_end(ap);
    
    bool add = false;
    ilist_iterator<BasicBlock> iterB = theFunc->getBasicBlockList().begin();
    while (iterB != theFunc->getBasicBlockList().end()) {
        BasicBlock& BB = *(theFunc->getBasicBlockList().begin());
        ilist_iterator<Instruction> iterI;
        for (iterI = BB.getInstList().begin(); iterI != BB.getInstList().end();) {
            Instruction &inst = *iterI;
            if (inst.getOpcodeName()[0] == '<') break;
            if (!(isa<AllocaInst>(inst) || isa<PHINode>(inst))) {
                
                CallInst *InstCall = CallInst::Create(tocall, args);
                InstCall->insertBefore(&inst);
                
                add = true;
                break;
            }
            iterI++;
        }
        if (add) break;
        iterB++;
    }
}

void Transformer::insertCallInstAtTail(Function* theFunc, Function* tocall, ...){
    std::vector<Value *> args;
    
    va_list ap; 
    va_start( ap, tocall );
    Value* arg = va_arg( ap, Value* );
    
    while(arg!=NULL){
        args.push_back(arg);
        arg = va_arg( ap, Value* );
    }
    
    va_end(ap);
    
    ilist_iterator<BasicBlock> iterB = theFunc->getBasicBlockList().begin();
    for (; iterB != theFunc->getBasicBlockList().end(); iterB++) {
        BasicBlock &b = *iterB;
        ilist_iterator<Instruction> iterI;
        for (iterI = b.getInstList().begin(); iterI != b.getInstList().end();) {
            Instruction &inst = *iterI;
            if (isa<ReturnInst>(inst)) {
                CallInst *InstCall = CallInst::Create(tocall, args);
                InstCall->insertBefore(&inst);
            }
            iterI++;
        }
    }
}

void Transformer::transform(AliasAnalysis& AA) {
    this->beforeTransform(AA);

    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function& f = *iterF;
        if (!this->functionToTransform(&f)) {
            continue;
        }
        for (ilist_iterator<BasicBlock> iterB = f.getBasicBlockList().begin(); iterB != f.getBasicBlockList().end(); iterB++) {
            BasicBlock &b = *iterB;
            if (!this->blockToTransform(&b)) {
                continue;
            }
            for (ilist_iterator<Instruction> iterI = b.getInstList().begin(); iterI != b.getInstList().end(); iterI++) {
                Instruction &inst = *iterI;

                if (!this->instructionToTransform(&inst)) {
                    continue;
                }

                if (this->debug()) {
                    errs() << "Transforming: " << inst << "\n";
                }

                if (isa<LoadInst>(inst)) {
                    this->transformLoadInst((LoadInst*)&inst, AA);
                }

                if (isa<StoreInst>(inst)) {
                    this->transformStoreInst((StoreInst*)&inst, AA);
                }

                if (isa<CallInst>(inst)) {
                    CallInst &call = *(CallInst*) & inst;
                    Value* calledValue = call.getCalledValue();

                    if (calledValue == NULL) continue;

                    if (isa<Function>(calledValue)) {
                        handleCalls((CallInst*) & inst, (Function*) calledValue, AA);
                    } else if (calledValue->getType()->isPointerTy()) {
                        set<Function*> may;
                        this->getAliasFunctions(&inst, calledValue, AA, &may);

                        set<Function*>::iterator it = may.begin();
                        while (it != may.end()) {
                            Function* cf = *it;
                            handleCalls((CallInst*) & inst, cf, AA);
                            it++;
                        }
                    }
                }

                if (isa<InvokeInst>(inst)) {
                    InvokeInst &call = *(InvokeInst*) & inst;
                    Value* calledValue = call.getCalledValue();

                    if (calledValue == NULL) continue;

                    if (isa<Function>(calledValue)) {
                        handleInvokes((InvokeInst*) & inst, (Function*) calledValue, AA);
                    } else if (calledValue->getType()->isPointerTy()) {
                        set<Function*> may;
                        this->getAliasFunctions(&inst, calledValue, AA, &may);

                        set<Function*>::iterator it = may.begin();
                        while (it != may.end()) {
                            Function* cf = *it;
                            handleInvokes((InvokeInst*) & inst, cf, AA);
                            it++;
                        }
                    }
                }
            }
        }
    }

    this->afterTransform(AA);
}

set<Function * >* Transformer::getAliasFunctions(Value* callInst, Value* ptr, AliasAnalysis& AA, set<Function*>* ret) {
    if (ret == NULL)
        return NULL;

    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        if (AA.alias(ptr, iterF) != AliasAnalysis::NoAlias) {
            if (matchFunctionAndCall(iterF, callInst, AA)) {
                if (iterF->hasName()) {
                    const char* name = iterF->getName().str().c_str();
                    char namecp[1000];
                    strcpy(namecp, name);
                    namecp[7] = '\0';

                    ///@FIXME we assume a function only has one posix thread alias
                    if (strcmp(namecp, "pthread") == 0) {
                        ret->clear();
                        ret->insert(iterF);
                        return ret;
                    }
                }
                ret->insert(iterF);
            }
        }
    }
    return ret;
}

bool Transformer::handleCalls(CallInst* call, Function* calledFunction, AliasAnalysis & AA) {
    Function &cf = *calledFunction;
    // fork & join
    if (cf.getName().str() == "pthread_create" && cf.getArgumentList().size() == 4) {
        transformPthreadCreate(call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_join") {
        transformPthreadJoin(call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_mutex_lock") {
        // lock & unlock
        transformPthreadMutexLock(call, AA);

        return true;
    } else if (cf.getName().str() == "pthread_mutex_unlock") {
        transformPthreadMutexUnlock(call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_cond_wait") {
        // wait & notify
        transformPthreadCondWait(call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_cond_signal") {
        transformPthreadCondSignal(call, AA);
        return true;
    } else if (cf.getName().str() == "exit") {
        // system exit
        transformSystemExit(call, AA);
        return true;
    } else if (cf.isIntrinsic()) {
        switch (cf.getIntrinsicID()) {
            case Intrinsic::memmove:
            case Intrinsic::memcpy:
            {
                transformMemCpyMov(call, AA);
            }
                break;
            case Intrinsic::memset:
            {
                transformMemSet(call, AA);
            }
                break;
            default:
                break;
        }
        return true;
    } else if (!this->functionToTransform(&cf) && !this->isInstrumentationFunction(&cf)) {
        // other empty functions
        transformSpecialFunctionCall(call, AA);
        return true;
    }
    //*/
    return false;
}

bool Transformer::handleInvokes(InvokeInst* call, Function* calledFunction, AliasAnalysis & AA) {
    Function &cf = *calledFunction;
    if (!this->functionToTransform(&cf)) {
        transformSpecialFunctionInvoke(call, AA);
        return true;
    }
    return false;
}

bool Transformer::matchFunctionAndCall(Function * callee, Value * callInst, AliasAnalysis & aa) {
    iplist<Argument>& parameters = callee->getArgumentList();

    unsigned int numOfArgOp = 0;
    bool isCallInst = false;
    if (isa<CallInst>(*callInst)) {
        isCallInst = true;
        numOfArgOp = ((CallInst*) callInst)->getNumArgOperands();
    } else {
        numOfArgOp = ((InvokeInst*) callInst)->getNumArgOperands();
    }

    if (numOfArgOp < parameters.size()) {
        return false;
    } else if (numOfArgOp > parameters.size() && !callee->isVarArg()) {
        return false;
    } else {
        int i = 0;
        ilist_iterator<Argument> it = parameters.begin();
        while (it != parameters.end()) {
            Value *arg = NULL;
            if (isCallInst) {
                arg = ((CallInst*) callInst)->getArgOperand(i);
            } else {
                arg = ((InvokeInst*) callInst)->getArgOperand(i);
            }

            Value * par = (Value*) it;
            if (aa.getTypeStoreSize(arg->getType()) != aa.getTypeStoreSize(par->getType())) {
                return false;
            }
            it++;
            i++;
        }
    }
    return true;
}
