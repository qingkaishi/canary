/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "Transformer4Trace.h"

#define POINTER_BIT_SIZE ptrsize*8

#define FUNCTION_VOID_ARG_TYPE Type::getVoidTy(context),(Type*)0
#define FUNCTION_MEM_LN_ARG_TYPE Type::getVoidTy(context),Type::getIntNPtrTy(context,POINTER_BIT_SIZE),Type::getIntNTy(context,POINTER_BIT_SIZE),Type::getInt8PtrTy(context,0),(Type*)0
#define FUNCTION_TID_LN_ARG_TYPE Type::getVoidTy(context),Type::getIntNTy(context,POINTER_BIT_SIZE),Type::getIntNTy(context,POINTER_BIT_SIZE),Type::getInt8PtrTy(context,0),(Type*)0
#define FUNCTION_2MEM_LN_ARG_TYPE Type::getVoidTy(context),Type::getIntNPtrTy(context,POINTER_BIT_SIZE),Type::getIntNPtrTy(context,POINTER_BIT_SIZE),Type::getIntNTy(context,POINTER_BIT_SIZE),Type::getInt8PtrTy(context,0),(Type*)0

static int getInstructionLineNumber(Instruction * inst){
    MDNode* md = inst->getMDNode("dbg");
    DILocation DI(md);
    return DI.getLineNumber();
}

char Transformer4Trace::ID = 0;

Transformer4Trace::Transformer4Trace() : ModulePass(ID)  { 
}

bool Transformer4Trace::debug() {
    return true;
}

void Transformer4Trace::beforeTransform(Module* module, AliasAnalysis& AA) {
    // do not remove it, it is used in the macro
     ptrsize = AA.getDataLayout()->getPointerSize();
    
    ///initialize functions
    // do not remove context, it is used in the macro FUNCTION_ARG_TYPE
    Module * m = module;
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
 //               std::string &filename=getInstructionFileName(&inst) ;
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

void Transformer4Trace::afterTransform(Module* module, AliasAnalysis& AA) {
    Function * mainFunction = module->getFunction("main");
    if (mainFunction != NULL) {
        this->insertCallInstAtHead(mainFunction, F_init, NULL);
        this->insertCallInstAtTail(mainFunction, F_exit, NULL);
    }
}

void Transformer4Trace::transformLoadInst(Module* module, LoadInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getOperand(0);
    int svIdx = this->getValueIndex(module, val, AA);
    if (svIdx == -1) return;

    int ln = getInstructionLineNumber(inst);

    CastInst* c = CastInst::CreatePointerCast(val, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    c->insertBefore(inst);
    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(inst, F_preload, c, lnval, getSrcFileNameArg(module, inst), NULL);
    this->insertCallInstAfter(inst, F_load, c, lnval, getSrcFileNameArg(module, inst), NULL);
}

void Transformer4Trace::transformStoreInst(Module* module, StoreInst* inst, AliasAnalysis& AA) {
    Value * val = inst->getOperand(1);
    int svIdx = this->getValueIndex(module, val, AA);
    if (svIdx == -1) return;

    int ln = getInstructionLineNumber(inst);

    CastInst* c = CastInst::CreatePointerCast(val, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    c->insertBefore(inst);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(inst, F_prestore, c, lnval, getSrcFileNameArg(module, inst), NULL);
    this->insertCallInstAfter(inst, F_store, c, lnval, getSrcFileNameArg(module, inst), NULL);
}

void Transformer4Trace::transformPthreadCreate(Module* module, CallInst* call, AliasAnalysis& AA) {
    int ln = getInstructionLineNumber(call);

    ConstantInt * lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_prefork, call->getArgOperand(0), lnval, getSrcFileNameArg(module, call), NULL);
    this->insertCallInstAfter(call, F_fork, call->getArgOperand(0), lnval, getSrcFileNameArg(module, call), NULL);
}

void Transformer4Trace::transformPthreadJoin(Module* module, CallInst* call, AliasAnalysis& AA) {
    int ln = getInstructionLineNumber(call);
    ConstantInt * lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_prejoin, call->getArgOperand(0), lnval, getSrcFileNameArg(module, call), NULL);
    this->insertCallInstAfter(call, F_join, call->getArgOperand(0), lnval, getSrcFileNameArg(module, call), NULL);

}

void Transformer4Trace::transformPthreadMutexLock(Module* module, CallInst* call, AliasAnalysis& AA) {
    Value * val = call->getArgOperand(0);
    int svIdx = this->getValueIndex(module, val, AA);
    if (svIdx == -1) return;

    int ln = getInstructionLineNumber(call);

    CastInst* c = CastInst::CreatePointerCast(val, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    c->insertBefore(call);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_prelock, c, lnval, getSrcFileNameArg(module, call), NULL);
    this->insertCallInstAfter(call, F_lock, c, lnval, getSrcFileNameArg(module, call), NULL);
}

void Transformer4Trace::transformPthreadMutexUnlock(Module* module, CallInst* call, AliasAnalysis& AA) {
    Value * val = call->getArgOperand(0);
    int svIdx = this->getValueIndex(module, val, AA);
    if (svIdx == -1) return;

    int ln = getInstructionLineNumber(call);

    CastInst* c = CastInst::CreatePointerCast(val, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    c->insertBefore(call);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_preunlock, c, lnval, getSrcFileNameArg(module, call), NULL);
    this->insertCallInstAfter(call, F_unlock, c, lnval, getSrcFileNameArg(module, call), NULL);

}

void Transformer4Trace::transformPthreadCondWait(Module* module, CallInst* call, AliasAnalysis& AA) {
    Value * val0 = call->getArgOperand(0);
    Value * val1 = call->getArgOperand(1);
    int svIdx0 = this->getValueIndex(module, val0, AA);
    int svIdx1 = this->getValueIndex(module, val1, AA);
    if (svIdx0 == -1 && svIdx1 == -1) return;

    int ln = getInstructionLineNumber(call);

    CastInst* cond = CastInst::CreatePointerCast(val0, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    cond->insertBefore(call);

    CastInst* mut = CastInst::CreatePointerCast(val1, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    mut->insertBefore(call);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_prewait, cond, mut, lnval, getSrcFileNameArg(module, call), NULL);
    this->insertCallInstAfter(call, F_wait, cond, mut, lnval, getSrcFileNameArg(module, call), NULL);

}

void Transformer4Trace::transformPthreadCondTimeWait(Module* module, CallInst* ins, AliasAnalysis& AA) {
    // do nothing in this version
}

void Transformer4Trace::transformPthreadCondSignal(Module* module, CallInst* call, AliasAnalysis& AA) {
    Value * val0 = call->getArgOperand(0);
    Value * val1 = call->getArgOperand(1);
    int svIdx0 = this->getValueIndex(module, val0, AA);
    int svIdx1 = this->getValueIndex(module, val1, AA);
    if (svIdx0 == -1 && svIdx1 == -1) return;

    int ln = getInstructionLineNumber(call);

    CastInst* cond = CastInst::CreatePointerCast(val0, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    cond->insertBefore(call);

    CastInst* mut = CastInst::CreatePointerCast(val1, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    mut->insertBefore(call);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    this->insertCallInstBefore(call, F_prenotify, cond, mut, lnval, getSrcFileNameArg(module, call), NULL);
    this->insertCallInstAfter(call, F_notify, cond, mut, lnval, getSrcFileNameArg(module, call), NULL);
}

void Transformer4Trace::transformSystemExit(Module* module, CallInst* ins, AliasAnalysis& AA) {
    this->insertCallInstBefore(ins, F_exit, NULL);
}

void Transformer4Trace::transformMemCpyMov(Module* module, CallInst* call, AliasAnalysis& AA) {
    int ln = getInstructionLineNumber(call);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    Value * dst = call->getArgOperand(0);
    Value * src = call->getArgOperand(1);
    int svIdx_dst = this->getValueIndex(module, dst, AA);
    int svIdx_src = this->getValueIndex(module, src, AA);
    if (svIdx_dst == -1 && svIdx_src == -1) {
        return;
    } else if (svIdx_dst != -1 && svIdx_src != -1) {
        CastInst* d = CastInst::CreatePointerCast(dst, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
        d->insertBefore(call);

        CastInst* s = CastInst::CreatePointerCast(src, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
        s->insertBefore(call);

        if (svIdx_dst != svIdx_src) {


            insertCallInstBefore(call, F_prestore, d, lnval, getSrcFileNameArg(module, call), NULL);
            insertCallInstAfter(call, F_store, d, lnval, getSrcFileNameArg(module, call), NULL);

            insertCallInstBefore(call, F_preload, s, lnval, getSrcFileNameArg(module, call), NULL);
            insertCallInstAfter(call, F_load, s, lnval, getSrcFileNameArg(module, call), NULL);

        } else {
            insertCallInstBefore(call, F_prestore, d, lnval, getSrcFileNameArg(module, call), NULL);
            insertCallInstAfter(call, F_store, d, lnval, getSrcFileNameArg(module, call), NULL);
        }

    } else if (svIdx_dst != -1) {
        CastInst* d = CastInst::CreatePointerCast(dst, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
        d->insertBefore(call);

        insertCallInstBefore(call, F_prestore, d, lnval, getSrcFileNameArg(module, call), NULL);
        insertCallInstAfter(call, F_store, d, lnval, getSrcFileNameArg(module, call), NULL);
    } else {
        CastInst* s = CastInst::CreatePointerCast(src, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
        s->insertBefore(call);

        insertCallInstBefore(call, F_preload, s, lnval, getSrcFileNameArg(module, call), NULL);
        insertCallInstAfter(call, F_load, s, lnval, getSrcFileNameArg(module, call), NULL);
    }
}

void Transformer4Trace::transformMemSet(Module* module, CallInst* call, AliasAnalysis& AA) {
    int ln = getInstructionLineNumber(call);

    ConstantInt* lnval = ConstantInt::get(Type::getIntNTy(module->getContext(), POINTER_BIT_SIZE), ln);

    Value * val = call->getArgOperand(0);
    int svIdx = this->getValueIndex(module, val, AA);
    if (svIdx == -1) return;

    CastInst* c = CastInst::CreatePointerCast(val, Type::getIntNPtrTy(module->getContext(),POINTER_BIT_SIZE));
    c->insertBefore(call);

    insertCallInstBefore(call, F_prestore, c, lnval, getSrcFileNameArg(module, call), NULL);
    insertCallInstAfter(call, F_store, c, lnval, getSrcFileNameArg(module, call), NULL);
}

void Transformer4Trace::transformOtherFunctionCalls(Module* module, CallInst* call, AliasAnalysis& AA) {
    int ln = getInstructionLineNumber(call);
   
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

        int svIdx = this->getValueIndex(module, arg, AA);
        if (svIdx != -1) {
            insertCallInstBefore(call, F_prestore, c, lnval, getSrcFileNameArg(module, call), NULL);
            insertCallInstBefore(call, F_store, c, lnval, getSrcFileNameArg(module, call), NULL);
        }
    }
}

bool Transformer4Trace::functionToTransform(Module* module, Function* f) {
    return !f->empty() && ignored_funcs.find(f) == ignored_funcs.end() && !this->isInstrumentationFunction(module, f) && !f->isIntrinsic();
}

bool Transformer4Trace::blockToTransform(Module* module, BasicBlock* bb) {
    return true;
}

bool Transformer4Trace::instructionToTransform(Module* module, Instruction* inst) {
    return true;
}

bool Transformer4Trace::isInstrumentationFunction(Module* module, Function* called) {
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

int Transformer4Trace::getValueIndex(Module* module, Value* v, AliasAnalysis & AA) {
    v = v->stripPointerCastsNoFollowAliases();
    while(isa<GlobalAlias>(v)){
        // aliase can be either global or bitcast of global
        v = ((GlobalAlias*)v)->getAliasee()->stripPointerCastsNoFollowAliases();
    }
    
    if(isa<GlobalVariable>(v) && ((GlobalVariable*)v)->isConstant()){
        return -1;
    }
    
    set<Value*>::iterator it = sharedVariables.begin();
    while (it != sharedVariables.end()) {
        Value * rep = *it;
        if (AA.alias(v, rep) != AliasAnalysis::NoAlias) {
            if(sv_idx_map.count(rep)){
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

Value* Transformer4Trace::getSrcFileNameArg(Module* module, Instruction* inst) {
    MDNode* md = inst->getMDNode("dbg");
    DILocation DI(md);
    const std::string& filename = DI.getFilename();
    
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

bool Transformer4Trace::runOnModule(Module& M){
    DyckAliasAnalysis & AA = this->getAnalysis<DyckAliasAnalysis>();
    
    AA.getEscapedPointersTo(&sharedVariables, M.getFunction("pthread_create"));
    
    this->transform(&M, &AA);
    
    outs() << "Please add -ltrace for trace analysis when you compile the transformed bitcode file to an executable file. Please use pecan to predict crugs.\n";
    return true;
}