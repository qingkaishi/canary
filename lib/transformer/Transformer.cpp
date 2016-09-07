/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 * 
 * @TODO
 * 1. matchFunctionAndCall; e.g. if a function is considered as pthread_create, we may lock it...
 */

#include "transformer/Transformer.h"
#include <llvm/Support/Debug.h>
#include <list>

//Transformer::Transformer(Module* m, set<Value*>* svs, unsigned psize) {
//    module = m;
//    sharedVariables = svs;
//    ptrsize = psize;
//}

CallInst* Transformer::insertCallInstBefore(Instruction* beforeInst, Function* tocall, ...) {
    std::vector<Value *> args;

    va_list ap;
    va_start(ap, tocall);
    Value* arg = va_arg(ap, Value*);

    while (arg != NULL) {
        args.push_back(arg);
        arg = va_arg(ap, Value*);
    }

    va_end(ap);

    CallInst *InstCall = CallInst::Create(tocall, args);
    InstCall->insertBefore(beforeInst);
    return InstCall;
}

CallInst* Transformer::insertCallInstAfter(Instruction* afterInst, Function* tocall, ...) {
    std::vector<Value *> args;

    va_list ap;
    va_start(ap, tocall);
    Value* arg = va_arg(ap, Value*);

    while (arg != NULL) {
        args.push_back(arg);
        arg = va_arg(ap, Value*);
    }

    va_end(ap);

    CallInst *InstCall = CallInst::Create(tocall, args);
    InstCall->insertAfter(afterInst);
    return InstCall;
}

CallInst* Transformer::insertCallInstAtHead(Function* theFunc, Function* tocall, ...) {
    std::vector<Value *> args;

    va_list ap;
    va_start(ap, tocall);
    Value* arg = va_arg(ap, Value*);

    while (arg != NULL) {
        args.push_back(arg);
        arg = va_arg(ap, Value*);
    }

    va_end(ap);

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

                return InstCall;
            }
            iterI++;
        }
        iterB++;
    }
    errs() << "Instruction insert failed in insertCallInstAtHead\n";
    errs() << theFunc->getName() << "\n";
    return NULL;
}

CallInst* Transformer::insertCallInstAtTail(Function* theFunc, Function* tocall, ...) {
    std::vector<Value *> args;

    va_list ap;
    va_start(ap, tocall);
    Value* arg = va_arg(ap, Value*);

    while (arg != NULL) {
        args.push_back(arg);
        arg = va_arg(ap, Value*);
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

                return InstCall;
            }
            iterI++;
        }
    }

    return NULL;
}

void Transformer::transform(Module* module, AliasAnalysis* AAptr) {
    AliasAnalysis& AA = *AAptr;
    this->beforeTransform(module, AA);

    unsigned functionsNum = module->getFunctionList().size();
    unsigned handledNum = 0;

    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function& f = *iterF;
        if (!this->functionToTransform(module, &f)) {
            outs() << "Remaining... " << (functionsNum - handledNum++) << " functions             \r";
            continue;
        }

        outs() << "Remaining... " << (functionsNum - handledNum++) << " functions             \r";

        bool allocHasHandled = false;
        vector<AllocaInst*> allocas;
        for (ilist_iterator<BasicBlock> iterB = f.getBasicBlockList().begin(); iterB != f.getBasicBlockList().end(); iterB++) {
            BasicBlock &b = *iterB;
            if (!this->blockToTransform(module, &b)) {
                continue;
            }
            for (ilist_iterator<Instruction> iterI = b.getInstList().begin(); iterI != b.getInstList().end(); iterI++) {
                Instruction &inst = *iterI;

                if (!this->instructionToTransform(module, &inst)) {
                    continue;
                }

                DEBUG_WITH_TYPE("transform", errs() << "[Transforming] " << inst << "\n");

                if (isa<AllocaInst>(inst)) {
                    // record a vector
                    allocas.push_back((AllocaInst*) & inst);
                }

                if (!isa<AllocaInst>(inst)
                        && !(isa<CallInst>(inst) && ((CallInst*) & inst)->getCalledFunction() != NULL && ((CallInst*) & inst)->getCalledFunction()->isIntrinsic())
                        && !allocHasHandled) {
                    allocHasHandled = true;

                    for (unsigned i = 0; i < allocas.size(); i++) {
                        AllocaInst * a = allocas[i];
                        this->transformAllocaInst(module, a, & inst, AA);
                    }
                }

                if (isa<LoadInst>(inst)) {
                    this->transformLoadInst(module, (LoadInst*) & inst, AA);
                }

                if (isa<StoreInst>(inst)) {
                    this->transformStoreInst(module, (StoreInst*) & inst, AA);
                }

                if (isa<AtomicRMWInst>(inst)) {
                    this->transformAtomicRMWInst(module, (AtomicRMWInst*) & inst, AA);
                }

                if (isa<AtomicCmpXchgInst>(inst)) {
                    this->transformAtomicCmpXchgInst(module, (AtomicCmpXchgInst*) & inst, AA);
                }

                if (isa<VAArgInst>(inst)) {
                    this->transformVAArgInst(module, (VAArgInst*) & inst, AA);
                }

                // all invokes have been lowered to calls
                if (isa<CallInst>(inst)) {
                    CallInst &call = *(CallInst*) & inst;
                    Value* calledValue = call.getCalledValue();

                    if (calledValue == NULL) continue;

                    if (isa<Function>(calledValue)) {
                        handleCalls(module, (CallInst*) & inst, (Function*) calledValue, AA);
                    } else if (calledValue->getType()->isPointerTy()) {
                        Call * c = ((DyckAliasAnalysis*) & AA)->getCallGraph()->getOrInsertFunction(&f)->getCall(&call);
                        if (c != NULL) {
                            if (isa<Function>(c->calledValue)) {
                                handleCalls(module, (CallInst*) & inst, (Function*) (c->calledValue), AA);
                            } else {
                                set<Function*>& may = ((PointerCall*) c)->mayAliasedCallees;
                                set<Function*>::iterator it = may.begin();
                                while (it != may.end()) {
                                    Function* cf = *it;
                                    handleCalls(module, (CallInst*) & inst, cf, AA);
                                    it++;
                                }
                            }
                        } else {
                            assert(call.isInlineAsm() && "Error in Transformer: transform: a non-inlineasm instruction cannot get Call!");
                        }
                    }
                }
            }
        }
    }

    outs() << "                                                            \r";

    this->afterTransform(module, AA);
}

bool Transformer::handleCalls(Module* module, CallInst* call, Function* calledFunction, AliasAnalysis & AA) {
    Function &cf = *calledFunction;
    // fork & join
    if (cf.getName().str() == "pthread_create" && cf.getArgumentList().size() == 4) {
        transformPthreadCreate(module, call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_join") {
        transformPthreadJoin(module, call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_mutex_lock") {
        // lock & unlock
        transformPthreadMutexLock(module, call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_mutex_unlock") {
        transformPthreadMutexUnlock(module, call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_cond_wait") {
        // wait & notify
        transformPthreadCondWait(module, call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_cond_timedwait") {
        // wait & notify
        transformPthreadCondTimeWait(module, call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_cond_signal") {
        transformPthreadCondSignal(module, call, AA);
        return true;
    } else if (cf.getName().str() == "pthread_mutex_init") {
        transformPthreadMutexInit(module, call, AA);
        return true;
    } else if (cf.getName().str().find("pthread") == 0) {
        transformOtherPthreadFunctions(module, call, AA);
        return true;
    } else if (cf.getName().str() == "exit") {
        // system exit
        transformSystemExit(module, call, AA);
        return true;
    } else if (cf.getName().str() == "malloc" || cf.getName().str() == "calloc"
            || cf.getName().str() == "realloc"
            || cf.getName().str() == "_Znaj"
            || cf.getName().str() == "_ZnajRKSt9nothrow_t"
            || cf.getName().str() == "_Znam"
            || cf.getName().str() == "_ZnamRKSt9nothrow_t"
            || cf.getName().str() == "_Znwj"
            || cf.getName().str() == "_ZnwjRKSt9nothrow_t"
            || cf.getName().str() == "_Znwm"
            || cf.getName().str() == "_ZnwmRKSt9nothrow_t") {
        // new / malloc
        transformAddressInit(module, call, AA);
        return true;
    } else if (cf.isIntrinsic()) {
        switch (cf.getIntrinsicID()) {
            case Intrinsic::vacopy:
            {
                transformVACpy(module, call, AA);
            }
                break;
            case Intrinsic::memmove:
            case Intrinsic::memcpy:
            {
                transformMemCpyMov(module, call, AA);
            }
                break;
            case Intrinsic::memset:
            {
                transformMemSet(module, call, AA);
            }
                break;
            default:
                transformOtherIntrinsics(module, call, AA);
                break;
        }
        return true;
    } else if (!this->functionToTransform(module, &cf) && !this->isInstrumentationFunction(module, &cf)) {
        // other empty functions
        transformOtherFunctionCalls(module, call, AA);
        return true;
    }
    //*/
    return false;
}
