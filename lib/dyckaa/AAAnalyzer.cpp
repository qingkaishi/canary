/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "AAAnalyzer.h"

static cl::opt<bool>
NoFunctionTypeCheck("no-function-type-check", cl::init(false), cl::Hidden,
        cl::desc("Do not check function type when resolving pointer calls."));

#define ARRAY_SIMPLIFIED

AAAnalyzer::AAAnalyzer(Module* m, DyckAliasAnalysis* a, DyckGraph* d, DyckCallGraph* cg) {
    module = m;
    aa = a;
    dgraph = d;
    callgraph = cg;
}

AAAnalyzer::~AAAnalyzer() {
    this->destroyFunctionGroups();
}

void AAAnalyzer::start_intra_procedure_analysis() {
    this->initFunctionGroups();
}

void AAAnalyzer::end_intra_procedure_analysis() {
}

void AAAnalyzer::start_inter_procedure_analysis() {
    // do nothing
    Function * gf = module->getFunction("pthread_getspecific");
    Function * sf = module->getFunction("pthread_setspecific");
    Function * cf = module->getFunction("pthread_key_create");

    if (gf || sf || cf) {
        errs() << "Warning: dyckaa does not handle pthread_getspecific/setspecific/key_create\n";
    }
}

void AAAnalyzer::end_inter_procedure_analysis() {
    // @TODO delete all null-value dyckvertices
    int NumAssistantVertices = dgraph->getAssistantVertices().size();
    int NumVertices = dgraph->numVertices();
    unsigned BytesToFree = sizeof (DyckVertex) * NumAssistantVertices;
    outs() << "# Assistant nodes: " << NumAssistantVertices << "(" << (NumAssistantVertices * 100 / NumVertices) << "%), " << BytesToFree / 1024 << "KB.\n\n";

    DEBUG_WITH_TYPE("pointercalls", this->printNoAliasedPointerCalls());
}

void AAAnalyzer::intra_procedure_analysis() {
    long instNum = 0;
    long intrinsicsNum = 0;
    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function* f = iterF;
        if (f->isIntrinsic()) {
            // intrinsics are handled as instructions
            intrinsicsNum++;
            continue;
        }
        DyckCallGraphNode* df = callgraph->getOrInsertFunction(f);
        for (ilist_iterator<BasicBlock> iterB = f->getBasicBlockList().begin(); iterB != f->getBasicBlockList().end(); iterB++) {
            for (ilist_iterator<Instruction> iterI = iterB->getInstList().begin(); iterI != iterB->getInstList().end(); iterI++) {
                Instruction *inst = iterI;
                instNum++;

                handle_inst(inst, df);
            }
        }
    }
    outs() << "# Instructions: " << instNum << "\n";
    outs() << "# Functions: " << module->getFunctionList().size() - intrinsicsNum << "\n";
    return;
}

static int INTERT = 0; // how many times inter_procedure_analysis() is called
static int FUNCTION_COUNT = 0;

void AAAnalyzer::inter_procedure_analysis() {
    map<DyckCallGraphNode*, set<CommonCall*>* > handledCommonCalls;

    while (1) {
        dgraph->qirunAlgorithm();

        FUNCTION_COUNT = 0;

        bool finished = true;

        outs() << "\nIteration #" << ++INTERT << "... \n";
        { // direct calls
            outs() << "Handling direct calls...";
            outs().flush();
            auto dfit = callgraph->begin();
            while (dfit != callgraph->end()) {
                DyckCallGraphNode * df = dfit->second;
                // ----------------------------------------------------
                set<CommonCall*>* df_handledCommonCalls = NULL;
                if (handledCommonCalls.count(df)) {
                    df_handledCommonCalls = handledCommonCalls[df];
                } else {
                    df_handledCommonCalls = new set<CommonCall*>;
                    handledCommonCalls.insert(pair<DyckCallGraphNode*, set<CommonCall*>*> (df, df_handledCommonCalls));
                }
                // ----------------------------------------------------
                set<CommonCall*>& callInsts = df->getCommonCalls();

                set<CommonCall*> df_unHandledCommonCalls;
                set_difference(callInsts.begin(), callInsts.end(),
                        df_handledCommonCalls->begin(), df_handledCommonCalls->end(),
                        inserter(df_unHandledCommonCalls, df_unHandledCommonCalls.begin()));

                auto cit = df_unHandledCommonCalls.begin();
                while (cit != df_unHandledCommonCalls.end()) {
                    finished = false;
                    CommonCall * theComCall = *cit;
                    df_handledCommonCalls->insert(theComCall);

                    Value * cv = theComCall->calledValue;
                    assert(isa<Function>(cv) && "Error: it is not a function in common calls!");
                    handle_common_function_call(theComCall, df, callgraph->getOrInsertFunction((Function*) cv));
                    cit++;
                }
                // ---------------------------------------------------
                ++dfit;
            }
            outs() << "Done!\n";
        }

        { // indirect call
            auto dfit = callgraph->begin();
            while (dfit != callgraph->end()) {
                DyckCallGraphNode * df = dfit->second;

                if (handle_pointer_function_calls(df)) {
                    finished = false;
                }
                ++dfit;
            }
        }

        if (finished) {
            break;
        }
    }

    // ---------------------------------------------------------
    auto mIt = handledCommonCalls.begin();
    while (mIt != handledCommonCalls.end()) {
        delete mIt->second;
        mIt++;
    }

    return;
}

void AAAnalyzer::printNoAliasedPointerCalls() {
    unsigned size = 0;

    outs() << ">>>>>>>>>> Pointer calls that do not find any aliased function\n";
    auto dfit = callgraph->begin();
    while (dfit != callgraph->end()) {
        DyckCallGraphNode * df = dfit->second;
        set<PointerCall*>& unhandled = df->getPointerCalls();

        auto pcit = unhandled.begin();
        while (pcit != unhandled.end()) {
            PointerCall* pc = *pcit;
            if (pc->mayAliasedCallees.empty()) {
                size++;
                if (pc->instruction != NULL) {
                    outs() << *(pc->instruction) << "\n";
                } else {
                    outs() << "Implicit calls in " << df->getLLVMFunction()->getName() << "\n";
                }
            }

            pcit++;
        }

        dfit++;
    }
    outs() << "<<<<<<<<<< Total: " << size << ".\n\n";
}

//// The followings are private functions

int AAAnalyzer::isCompatible(FunctionType * t1, FunctionType * t2) {
    if (NoFunctionTypeCheck) {
        return 1;
    }

    if (t1 == t2) {
        return 1;
    }

    if (t1->isVarArg() != t2->isVarArg()) {
        return 0;
    }

    Type* retty1 = t1->getReturnType();
    Type* retty2 = t2->getReturnType();

    if (retty1->isVoidTy() != retty2->isVoidTy()) {
        return 0;
    }

    unsigned pn1 = t1->getNumParams();
    unsigned pn2 = t2->getNumParams();

    if (pn1 == pn2) {
        bool level2 = true;
        for (unsigned i = 0; i < pn1; i++) {
            if (aa->getTypeStoreSize(t1->getParamType(i))
                    != aa->getTypeStoreSize(t2->getParamType(i))) {
                level2 = false;
                break;
            }
        }

        if (level2)
            return 2;

        bool level3 = true;
        for (unsigned i = 0; i < pn1; i++) {
            Type* tp1 = t1->getParamType(i);
            Type* tp2 = t2->getParamType(i);

            if (tp1->isIntegerTy() != tp2->isIntegerTy()) {
                level3 = false;
                break;
            }

            if (tp1->isPointerTy() != tp2->isPointerTy()) {
                level3 = false;
                break;
            }
        }

        if (level3)
            return 3;

        return 4;
    } else {
        return 0;
    }
}

FunctionTypeNode* AAAnalyzer::initFunctionGroup(FunctionType* fty) {
    if (functionTyNodeMap.count(fty)) {
        return functionTyNodeMap[fty]->root;
    }

    // iterate all roots to check whether compatible
    // if so, add it to its set
    // otherwise, new a root
    set<FunctionTypeNode *>::iterator rit = tyroots.begin();
    while (rit != tyroots.end()) {
        if (isCompatible((*rit)->type, fty)) {
            FunctionTypeNode * tn = new FunctionTypeNode;
            tn->type = fty;
            tn->root = (*rit);

            functionTyNodeMap.insert(pair<Type*, FunctionTypeNode*>(fty, tn));
            return *rit;
        }
        rit++;
    }

    // not found compatible ones, create a new one
    FunctionTypeNode * tn = new FunctionTypeNode;
    tn->type = fty;
    tn->root = tn;

    tyroots.insert(tn);
    functionTyNodeMap.insert(pair<Type*, FunctionTypeNode*>(fty, tn));
    return tn;
}

void AAAnalyzer::initFunctionGroups() {
    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function* f = iterF;
        if (f->isIntrinsic()) {
            continue;
        }

        bool onlyUsedAsCallFunc = true;
        for (Value::user_iterator I = f->user_begin(), E = f->user_end(); I != E; ++I) {
            User *U = (User*) (*I);
            if (isa<CallInst>(U) && ((CallInst*) U)->getCalledFunction() == f) { // all invoke -> call
                continue;
            } else {
                onlyUsedAsCallFunc = false;
                break;
            }
        }

        // the function only used in call instructions cannot alias with
        // certain function pointers
        if (!onlyUsedAsCallFunc) {
            FunctionType * fty = (FunctionType *) ((PointerType*) f->getType())->getPointerElementType();

            FunctionTypeNode * root = this->initFunctionGroup(fty);
            root->compatibleFuncs.insert(f);
        }
    }
}

void AAAnalyzer::destroyFunctionGroups() {
    map<Type*, FunctionTypeNode*>::iterator mit = functionTyNodeMap.begin();
    while (mit != functionTyNodeMap.end()) {
        FunctionTypeNode* tn = mit->second;
        delete tn;
        mit++;
    }
    functionTyNodeMap.clear();
    tyroots.clear();
}

void AAAnalyzer::combineFunctionGroups(FunctionType * ft1, FunctionType* ft2) {
    if (!this->isCompatible(ft1, ft2)) {

        FunctionTypeNode * ftn1 = this->initFunctionGroup(ft1);
        FunctionTypeNode * ftn2 = this->initFunctionGroup(ft2);

        if (ftn1->root == ftn2->root) return;

        outs() << "[CANARY] Combining " << *ft1 << " and " << *ft2 << "... \n";

        if (ftn1->root->compatibleFuncs.size() > ftn2->root->compatibleFuncs.size()) {
            FunctionTypeNode * temp = ftn1;
            ftn1 = ftn2;
            ftn2 = temp;
        }

        ftn1->root = ftn2->root; // 1->2
        set<Function*>::iterator ftnIt = ftn1->compatibleFuncs.begin();
        while (ftnIt != ftn1->compatibleFuncs.end()) {
            FunctionType* f = (*ftnIt)->getFunctionType();

            if (!functionTyNodeMap.count(f)) {
                errs() << "Error in combination of function groups!" << "\n";
                exit(-1);
            }

            FunctionTypeNode * ftn = functionTyNodeMap[f];
            ftn->root = ftn2->root;

            ftnIt++;
        }

        ftn2->compatibleFuncs.insert(ftn1->compatibleFuncs.begin(), ftn1->compatibleFuncs.end());
        ftn1->compatibleFuncs.clear();
    }
}

#ifndef ARRAY_SIMPLIFIED
/// return the offset vertex

static DyckVertex* addPtrOffset(DyckVertex* val, int offset, DyckGraph* dgraph) {
    DyckVertex * valrep = val->getRepresentative();
    if (offset == 0 || valrep->hasProperty("unknown-offset")) {
        return valrep;
    } else if (offset > 0) {
        DyckVertex * dv = init(NULL, dgraph);
        val->getRepresentative()->addTarget(dv, (void*) offset);
        return dv;
    } else {
        DyckVertex * dv = init(NULL, dgraph);
        dv->addTarget(val->getRepresentative(), (void*) (-offset));
        return dv;
    }
}
#endif

/// return the structure's field vertex

DyckVertex* AAAnalyzer::addField(DyckVertex* val, long fieldIndex, DyckVertex* field) {
    DyckVertex* valrep = val->getRepresentative();

    if (field == NULL) {
        set<DyckVertex*>* valrepset = valrep->getOutVertices((void*) (aa->getOrInsertIndexEdgeLabel(fieldIndex)));
        if (valrepset != NULL && !valrepset->empty()) {
            field = *(valrepset->begin());
        } else {
            field = dgraph->retrieveDyckVertex(NULL).first;
            valrep->addTarget(field, (void*) (aa->getOrInsertIndexEdgeLabel(fieldIndex)));
        }
    } else {
        valrep->addTarget(field->getRepresentative(), (void*) (aa->getOrInsertIndexEdgeLabel(fieldIndex)));
    }

    return field;
}

/// if one of add and val is null, create and return it
/// otherwise return the ptr;

DyckVertex* AAAnalyzer::addPtrTo(DyckVertex* address, DyckVertex* val) {
    assert((address != NULL || val != NULL) && "ERROR in addPtrTo\n");

    if (address == NULL) {
        address = dgraph->retrieveDyckVertex(NULL).first;
        address->addTarget(val->getRepresentative(), (void*) aa->DEREF_LABEL);
        return address;
    } else if (val == NULL) {
        DyckVertex* addrep = address->getRepresentative();
        set<DyckVertex*>* derefset = addrep->getOutVertices((void*) aa->DEREF_LABEL);
        if (derefset != NULL && !derefset->empty()) {
            val = *(derefset->begin());
        } else {
            val = dgraph->retrieveDyckVertex(NULL).first;
            addrep->addTarget(val, (void*) aa->DEREF_LABEL);
        }

        return val;
    } else {
        DyckVertex* addrep = address->getRepresentative();
        addrep->addTarget(val->getRepresentative(), (void*) aa->DEREF_LABEL);
        return addrep;
    }

}

void AAAnalyzer::makeAlias(DyckVertex* x, DyckVertex* y) {
    // combine x's rep and y's rep
    dgraph->combine(x, y);
}

void AAAnalyzer::makeContentAlias(DyckVertex* x, DyckVertex* y) {
    addPtrTo(y, addPtrTo(x, NULL));
}

DyckVertex* AAAnalyzer::handle_gep(GEPOperator* gep) {
    Value * ptr = gep->getPointerOperand();
    DyckVertex* current = wrapValue(ptr);

    gep_type_iterator GTI = gep_type_begin(gep); // preGTI is the PointerTy of ptr

    int num_indices = gep->getNumIndices();
    int idxidx = 0;
    while (idxidx < num_indices) {
        Value * idx = gep->getOperand(++idxidx);
        Type * AggOrPointerTy = *(GTI++);
        Type * ElmtTy = *GTI;

        if (/*!isa<ConstantInt>(idx) ||*/ !AggOrPointerTy->isSized()) {
            // current->addProperty("unknown-offset", (void*) 1);
            //break;
        } else if (AggOrPointerTy->isStructTy()) {
            // example: gep y 0 constIdx
            // s1: y--deref-->?1--(fieldIdx idxLabel)-->?2
            DyckVertex* theStruct = this->addPtrTo(current, NULL);

            ConstantInt * ci = cast<ConstantInt>(idx);
            if (ci == NULL) {
                errs() << ("ERROR: when dealing with gep: \n");
                errs() << *gep << "\n";
                exit(1);
            }
            // field index need not be the same as original value
            // make it be a negative integer
            long fieldIdx = (long) (*(ci->getValue().getRawData()));
            long addroffset = 0; // use addr offset as field index
            for (long x = 0; x < fieldIdx; x++) {
                Type* tx = AggOrPointerTy->getStructElementType(x);
                addroffset += aa->getTypeStoreSize(tx);
            }

            DyckVertex* field = this->addField(theStruct, addroffset, NULL);
            DyckVertex* fieldPtr = current;
            if (addroffset != 0) {
                // s2: ?3--deref-->?2 (see else: if idx = 0, offset = 0, then the same ptr)
                fieldPtr = this->addPtrTo(NULL, field);
                /// the label representation and feature impl is temporal. @FIXME
                // s3: y--(fieldIdx offLabel)-->?3
                current->getRepresentative()->addTarget(fieldPtr->getRepresentative(), (void*) (aa->getOrInsertOffsetEdgeLabel(addroffset)));
            } else {
                this->addPtrTo(fieldPtr, field);
            }

            // update current
            current = fieldPtr;
        } else if (AggOrPointerTy->isPointerTy() || AggOrPointerTy->isArrayTy()) {
#ifndef ARRAY_SIMPLIFIED
            current = addPtrOffset(current, getConstantIntRawData(cast<ConstantInt>(idx)) * dl.getTypeAllocSize(*GTI), dgraph);
#endif
        } else {
            errs() << "ERROR in handle_gep: unknown type:\n";
            errs() << "Type Id: " << AggOrPointerTy->getTypeID() << "\n";
            exit(1);
        }
    }

    return current;
}

// constant is handled here.

DyckVertex* AAAnalyzer::wrapValue(Value * v) {
    // if the vertex of v exists, return it, otherwise create one
    pair < DyckVertex*, bool> retpair = dgraph->retrieveDyckVertex(v);
    if (retpair.second || v == NULL) {
        return retpair.first;
    }
    DyckVertex* vdv = retpair.first;
    // constantTy are handled as below.
    if (isa<ConstantExpr>(v)) {
        // constant expr should be handled like a assignment instruction
        if (isa<GEPOperator>(v)) {
            DyckVertex * got = handle_gep((GEPOperator*) v);
            makeAlias(vdv, got);
        } else if (((ConstantExpr*) v)->isCast()) {
            // errs() << *v << "\n";
            DyckVertex * got = wrapValue(((ConstantExpr*) v)->getOperand(0));
            makeAlias(vdv, got);

            // combine function groups
            if (!isa<Function>(((ConstantExpr*) v)->getOperand(0))) {
                Type* origTy = ((ConstantExpr*) v)->getOperand(0)->getType();
                Type* castTy = v->getType();
                if (origTy->isPointerTy() && origTy->getPointerElementType()->isFunctionTy()
                        && castTy->isPointerTy() && castTy->getPointerElementType()->isFunctionTy()) {
                    combineFunctionGroups((FunctionType*) origTy->getPointerElementType(), (FunctionType*) castTy->getPointerElementType());
                }
            } else {
                // This case has been handled in initFunctionGroup
            }
        } else {
            unsigned opcode = ((ConstantExpr*) v)->getOpcode();
            if (opcode >= Instruction::BinaryOpsBegin && opcode <= Instruction::BinaryOpsEnd) {
                // do nothing
            } else {
                errs() << "ERROR when handle the following constant expression\n";
                errs() << *v << "\n";
                errs() << ((ConstantExpr*) v)->getOpcode() << "\n";
                errs() << ((ConstantExpr*) v)->getOpcodeName() << "\n";
                errs().flush();
                exit(-1);
            }
        }
    } else if (isa<ConstantStruct>(v) || isa<ConstantArray>(v)) {
        Constant * vAgg = (Constant*) v;
        unsigned numElmt = vAgg->getNumOperands();
        for (unsigned i = 0; i < numElmt; i++) {
            Value * vi = vAgg->getOperand(i);

            std::vector<unsigned> indices;
            indices.push_back(i);
            ArrayRef<unsigned> indicesRef(indices);
            this->handle_extract_insert_value_inst(vdv, vAgg->getType(), indicesRef, vi);
        }
    } else if (isa<GlobalValue>(v)) {
        if (isa<GlobalVariable>(v)) {
            GlobalVariable * global = (GlobalVariable *) v;
            if (global->hasInitializer()) {
                Value * initializer = global->getInitializer();
                if (!isa<UndefValue>(initializer)) {
                    DyckVertex * initVer = wrapValue(initializer);
                    addPtrTo(vdv, initVer);
                }
            }
        } else if (isa<GlobalAlias>(v)) {
            GlobalAlias * global = (GlobalAlias *) v;
            Value * aliasee = global->getAliasee();
            makeAlias(vdv, wrapValue(aliasee));
        } else if (isa<Function>(v)) {
            // do nothing
        } // no else
    } else if (isa<ConstantInt>(v) || isa<ConstantFP>(v) || isa<ConstantPointerNull>(v) || isa<UndefValue>(v)) {
        // do nothing
    } else if (isa<ConstantDataArray>(v) || isa<ConstantAggregateZero>(v)) {
        // do nothing
    } else if (isa<BlockAddress>(v)) {
        // do nothing
    } else if (isa<ConstantDataVector>(v)) {
        errs() << "ERROR when handle the following ConstantDataSequential, ConstantDataVector\n";
        errs() << *v << "\n";
        errs() << "PROMPT: Please use -fno-vectorize and -fno-slp-vectorize at compile time.\n";
        errs().flush();
        exit(-1);
    } else if (isa<ConstantVector>(v)) {
        errs() << "ERROR when handle the following ConstantVector\n";
        errs() << *v << "\n";
        errs() << "PROMPT: Please use -fno-vectorize and -fno-slp-vectorize at compile time.\n";
        errs().flush();
        exit(-1);
    } else if (isa<Constant>(v)) {
        errs() << "ERROR when handle the following constant value\n";
        errs() << *v << "\n";
        errs().flush();
        exit(-1);
    }

    return vdv;
}

void AAAnalyzer::handle_instrinsic(Instruction *inst) {
    if (inst == NULL) return;
    IntrinsicInst * call = (IntrinsicInst*) inst;
    switch (call->getIntrinsicID()) {
            // Variable Argument Handling Intrinsics
        case Intrinsic::vastart:
        {
            Value * va_list_ptr = call->getArgOperand(0);
            wrapValue(va_list_ptr);
        }
            break;
        case Intrinsic::vaend:
        {
        }
            break;
        case Intrinsic::vacopy: // the same with memmove/memcpy

            //Standard C Library Intrinsics
        case Intrinsic::memmove:
        case Intrinsic::memcpy:
        {
            Value * src_ptr = call->getArgOperand(0);
            Value * dst_ptr = call->getArgOperand(1);

            DyckVertex* src_ptr_ver = wrapValue(src_ptr);
            DyckVertex* dst_ptr_ver = wrapValue(dst_ptr);

            DyckVertex* src_ver = addPtrTo(src_ptr_ver, NULL);
            DyckVertex* dst_ver = addPtrTo(dst_ptr_ver, NULL);

            makeAlias(src_ver, dst_ver);
        }
            break;
        case Intrinsic::memset:
        {
            Value * ptr = call->getArgOperand(0);
            Value * val = call->getArgOperand(1);
            addPtrTo(wrapValue(ptr), wrapValue(val));
        }
            break;
            /// @todo other C lib intrinsics

            //Accurate Garbage Collection Intrinsics
            //Code Generator Intrinsics
            //Bit Manipulation Intrinsics
            //Exception Handling Intrinsics
            //Trampoline Intrinsics
            //Memory Use Markers
            //General Intrinsics

            //Arithmetic with Overflow Intrinsics
            //Specialised Arithmetic Intrinsics
            //Half Precision Floating Point Intrinsics
            //Debugger Intrinsics
        default:break;
    }
}

set<Function*>* AAAnalyzer::getCompatibleFunctions(FunctionType * fty) {
    FunctionTypeNode * ftn = this->initFunctionGroup(fty);
    return &(ftn->root->compatibleFuncs);
}

void AAAnalyzer::handle_inst(Instruction *inst, DyckCallGraphNode * parent_func) {
    DEBUG_WITH_TYPE("inst", errs() << *inst << "\n");
    switch (inst->getOpcode()) {
            // common/bitwise binary operations
            // Terminator instructions
        case Instruction::Ret:
        {
            ReturnInst* retInst = ((ReturnInst*) inst);
            if (retInst->getNumOperands() > 0 && !retInst->getOperandUse(0)->getType()->isVoidTy()) {
                parent_func->addRet(retInst->getOperandUse(0));
            }
        }
            break;
        case Instruction::Resume:
        {
            Value* resume = ((ResumeInst*) inst)->getOperand(0);
            parent_func->addResume(resume);
        }
            break;
        case Instruction::Switch:
        case Instruction::Br:
        case Instruction::IndirectBr:
        case Instruction::Unreachable:
            break;

            // vector operations
        case Instruction::ExtractElement:
        {
        }
            break;
        case Instruction::InsertElement:
        {
        }
            break;
        case Instruction::ShuffleVector:
        {
        }
            break;

            // aggregate operations
        case Instruction::ExtractValue:
        {
            Value * agg = ((ExtractValueInst*) inst)->getAggregateOperand();
            ArrayRef<unsigned> indices = ((ExtractValueInst*) inst)->getIndices();

            this->handle_extract_insert_value_inst(wrapValue(agg), agg->getType(), indices, inst);
        }
            break;
        case Instruction::InsertValue:
        {
            DyckVertex* resultV = wrapValue(inst);
            Value * agg = ((InsertValueInst*) inst)->getAggregateOperand();
            if (!isa<UndefValue>(agg)) {
                makeAlias(resultV, wrapValue(agg));
            }

            ArrayRef<unsigned> indices = ((InsertValueInst*) inst)->getIndices();

            this->handle_extract_insert_value_inst(resultV, inst->getType(), indices, ((InsertValueInst*) inst)->getInsertedValueOperand());
        }
            break;

            // memory accessing and addressing operations
        case Instruction::Alloca:
        {
        }
            break;
        case Instruction::Fence:
        {
        }
            break;
        case Instruction::AtomicCmpXchg:
        {
            Value * retXchg = inst;
            Value * ptrXchg = inst->getOperand(0);
            Value * newXchg = inst->getOperand(2);
            addPtrTo(wrapValue(ptrXchg), wrapValue(retXchg));
            addPtrTo(wrapValue(ptrXchg), wrapValue(newXchg));
        }
            break;
        case Instruction::AtomicRMW:
        {
            Value * retRmw = inst;
            Value * ptrRmw = ((AtomicRMWInst*) inst)->getPointerOperand();
            addPtrTo(wrapValue(ptrRmw), wrapValue(retRmw));

            switch (((AtomicRMWInst*) inst)->getOperation()) {
                case AtomicRMWInst::Max:
                case AtomicRMWInst::Min:
                case AtomicRMWInst::UMax:
                case AtomicRMWInst::UMin:
                case AtomicRMWInst::Xchg:
                {
                    Value * newRmw = ((AtomicRMWInst*) inst)->getValOperand();
                    addPtrTo(wrapValue(ptrRmw), wrapValue(newRmw));
                }
                    break;
                default:
                    //others are binary ops like add/sub/...
                    ///@TODO
                    break;
            }
        }
            break;
        case Instruction::Load:
        {
            Value *lval = inst;
            Value *ladd = inst->getOperand(0);
            addPtrTo(wrapValue(ladd), wrapValue(lval));
        }
            break;
        case Instruction::Store:
        {
            Value * sval = inst->getOperand(0);
            Value * sadd = inst->getOperand(1);
            addPtrTo(wrapValue(sadd), wrapValue(sval));
        }
            break;
        case Instruction::GetElementPtr:
        {
            makeAlias(wrapValue(inst), handle_gep((GEPOperator*) inst));
        }
            break;

            // conversion operations
        case Instruction::AddrSpaceCast:
        {
            outs() << "[INFO] Detect AddrSpaceCast Instruction: " << *inst << "\n";
        }
        case Instruction::Trunc:
        case Instruction::ZExt:
        case Instruction::SExt:
        case Instruction::FPTrunc:
        case Instruction::FPExt:
        case Instruction::FPToUI:
        case Instruction::FPToSI:
        case Instruction::UIToFP:
        case Instruction::SIToFP:
        case Instruction::BitCast:
        case Instruction::PtrToInt:
        case Instruction::IntToPtr:
        {
            Value * itpv = inst->getOperand(0);
            makeAlias(wrapValue(inst), wrapValue(itpv));

            ///  function pointer cast
            Type* origTy = itpv->getType();
            Type* castTy = inst->getType();

            if (!isa<Function>(itpv)) {
                if (origTy->isPointerTy() && origTy->getPointerElementType()->isFunctionTy()
                        && castTy->isPointerTy() && castTy->getPointerElementType()->isFunctionTy()) {
                    combineFunctionGroups((FunctionType*) origTy->getPointerElementType(), (FunctionType*) castTy->getPointerElementType());
                }
            } else {
                // this case has been handled in initFunctionGroups
            }
        }
            break;

            // other operations
        case Instruction::Invoke: // invoke is a terminal operation
        {
            errs() << "Error during alias analysis. Please add -lowerinvoke -simplifycfg before -dyckaa!\n";
            exit(-1);

            // for later use
            //            InvokeInst * invoke = (InvokeInst*) inst;
            //            LandingPadInst* lpd = invoke->getLandingPadInst();
            //            parent_func->addLandingPad(invoke, lpd);
            //
            //            Value * cv = invoke->getCalledValue();
            //            vector<Value*> args;
            //            for (unsigned i = 0; i < invoke->getNumArgOperands(); i++) {
            //                args.push_back(invoke->getArgOperand(i));
            //            }
            //
            //            this->handle_invoke_call_inst(invoke, cv, &args, parent_func);
        }
            break;
        case Instruction::Call:
        {
            CallInst * callinst = (CallInst*) inst;

            if (callinst->isInlineAsm()) {
                parent_func->addInlineAsm(callinst);
                break;
            }

            Value * cv = callinst->getCalledValue();
            vector<Value*> args;
            for (unsigned i = 0; i < callinst->getNumArgOperands(); i++) {
                args.push_back(callinst->getArgOperand(i));
            }

            this->handle_invoke_call_inst(callinst, cv, &args, parent_func);
        }
            break;
        case Instruction::PHI:
        {
            PHINode *phi = (PHINode *) inst;
            int nums = phi->getNumIncomingValues();
            for (int i = 0; i < nums; i++) {
                Value * p = phi->getIncomingValue(i);
                makeAlias(wrapValue(inst), wrapValue(p));
            }
        }
            break;
        case Instruction::Select:
        {
            Value *first = ((SelectInst*) inst)->getTrueValue();
            Value *second = ((SelectInst*) inst)->getFalseValue();
            makeAlias(wrapValue(inst), wrapValue(first));
            makeAlias(wrapValue(inst), wrapValue(second));
        }
            break;
        case Instruction::VAArg:
        {
            parent_func->addVAArg(inst);

            DyckVertex* vaarg = wrapValue(inst);
            Value * ptrVaarg = inst->getOperand(0);
            addPtrTo(wrapValue(ptrVaarg), vaarg);
        }
            break;
        case Instruction::LandingPad: // handled with invoke inst
        case Instruction::ICmp:
        case Instruction::FCmp:
        default:
            break;
    }
}

void AAAnalyzer::handle_extract_insert_value_inst(DyckVertex* aggV, Type* aggTy, ArrayRef<unsigned>& indices, Value* insertedOrExtractedValue) {
    DyckVertex* currentStruct = aggV;

    for (unsigned int i = 0; i < indices.size(); i++) {
        assert(aggTy->isAggregateType() && "Error in handle_extract_insert_value_inst, not an agg (array/struct) type!");

        if (!aggTy->isSized()) {
        } else if (aggTy->isArrayTy()) {
            aggTy = ((CompositeType*) aggTy)->getTypeAtIndex(indices[i]);
#ifndef ARRAY_SIMPLIFIED
            current = addPtrOffset(current, (int) indices[i] * dl.getTypeAllocSize(aggTy), dgraph);
#endif
            if (i == indices.size() - 1) {
                this->makeAlias(currentStruct, wrapValue(insertedOrExtractedValue));
            }
        } else /*if (aggTy->isStructTy())*/ {
            long offset = 0; // use addr offset as index
            for (int x = 0; x < indices[i]; x++) {
                offset += aa->getTypeStoreSize(((CompositeType*) aggTy)->getTypeAtIndex(x));
            }

            if (i != indices.size() - 1) {
                currentStruct = this->addField(currentStruct, offset, NULL);
            } else {
                currentStruct = this->addField(currentStruct, offset, wrapValue(insertedOrExtractedValue));
            }

            aggTy = ((CompositeType*) aggTy)->getTypeAtIndex(indices[i]);
        }
    }
}

void AAAnalyzer::handle_invoke_call_inst(Instruction* ret, Value* cv, vector<Value*>* args, DyckCallGraphNode* parent) {
    if (isa<Function>(cv)) {
        if (((Function*) cv)->isIntrinsic()) {
            handle_instrinsic((Instruction*) ret);
        } else {
            this->handle_lib_invoke_call_inst(ret, (Function*) cv, args, parent);
            parent->addCommonCall(new CommonCall(ret, (Function*) cv, args));
        }
    } else {
        wrapValue(cv);
        if (isa<ConstantExpr>(cv)) {

            Value * cvcopy = cv;
            while (isa<ConstantExpr>(cvcopy) && ((ConstantExpr*) cvcopy)->isCast()) {
                cvcopy = ((ConstantExpr*) cvcopy)->getOperand(0);
            }

            if (isa<Function>(cvcopy)) {
                this->handle_lib_invoke_call_inst(ret, (Function*) cvcopy, args, parent);
                parent->addCommonCall(new CommonCall(ret, (Function*) cvcopy, args));
            } else {
                PointerCall* pcall = new PointerCall(ret, cv, args);
                parent->addPointerCall(pcall);
            }
        } else if (isa<GlobalAlias>(cv)) {
            Value * cvcopy = cv;
            while (isa<GlobalAlias>(cvcopy)) {
                cvcopy = ((GlobalAlias*) cvcopy)->getAliasee()->stripPointerCastsNoFollowAliases();
            }

            if (isa<Function>(cvcopy)) {
                this->handle_lib_invoke_call_inst(ret, (Function*) cvcopy, args, parent);
                parent->addCommonCall(new CommonCall(ret, (Function*) cvcopy, args));
            } else {
                PointerCall* pcall = new PointerCall(ret, cv, args);
                parent->addPointerCall(pcall);
            }
        } else {
            PointerCall * pcall = new PointerCall(ret, cv, args);
            parent->addPointerCall(pcall);
        }
    }
}

void AAAnalyzer::handle_common_function_call(Call* c, DyckCallGraphNode* caller, DyckCallGraphNode* callee) {
    // for better precise, if callee is an empty function, we do not
    // match the args and parameters.
    if (callee->getLLVMFunction()->empty()) {
        return;
    }

    // since invoke has been lowered to call, no landingpads
    // and resumes. The following codes are for later use.
    // landingpad<->resume
    //    if (c->instruction != NULL) {
    //        Value* lpd = caller->getLandingPad(c->instruction);
    //        if (lpd != NULL) {
    //            DyckVertex* lpdVertex = wrapValue(lpd);
    //            set<Value*>& res = callee->getResumes();
    //            set<Value*>::iterator resIt = res.begin();
    //            while (resIt != res.end()) {
    //                makeAlias(wrapValue(*resIt), lpdVertex);
    //                resIt++;
    //            }
    //        }
    //    }

    //return<->call
    if (c->instruction != NULL) {
        Type * calledValueTy = ((CallInst*) c->instruction)->getCalledValue()->getType();
        assert(calledValueTy->isPointerTy() && "A called value is not a pointer type!");
        Type * calledFuncTy = calledValueTy->getPointerElementType();
        assert(calledFuncTy->isFunctionTy() && "A called value is not a function pointer type!");

        set<Value*>& rets = callee->getReturns();
        if (!((FunctionType*) calledFuncTy)->getReturnType()->isVoidTy()) {
            Type* retTy = c->instruction->getType();

            set<Value*>::iterator retIt = rets.begin();
            while (retIt != rets.end()) {
                Value* val = *retIt;
                if (aa->getTypeStoreSize(retTy) >= aa->getTypeStoreSize(val->getType())) {
                    makeAlias(wrapValue(val), wrapValue(c->instruction));
                }
                retIt++;
            }
        }
    }

    // parameter<->arg
    Function * func = callee->getLLVMFunction();
    if (!func->isIntrinsic()) {
        vector<Value*>& parameters = callee->getArgs();
        vector<Value*>& arguments = c->args;

        unsigned NumPars = parameters.size();
        unsigned NumArgs = arguments.size();

        unsigned iterationNum = NumPars < NumArgs ? NumPars : NumArgs;
        for (unsigned i = 0; i < iterationNum; i++) {
            Value* par = parameters[i];
            Value* arg = arguments[i];

            makeAlias(wrapValue(par), wrapValue(arg));

            if (aa->getTypeStoreSize(par->getType()) < aa->getTypeStoreSize(arg->getType())) {
                // the first pair of arg and par that are not type matched can be aliased,
                // but the followings are rarely possible.
                return;
            }
        }

        if (NumArgs > NumPars && func->isVarArg()) {
            vector<Value*>& var_parameters = callee->getVAArgs();
            unsigned NumVarPars = var_parameters.size();

            for (unsigned int i = NumPars; i < NumArgs; i++) {
                Value * arg = arguments[i];
                DyckVertex* argV = wrapValue(arg);

                for (unsigned j = 0; j < NumVarPars; j++) {
                    Value * var_par = var_parameters[j];

                    // for var arg function, we only can get var args according to exact types.
                    if (aa->getTypeStoreSize(var_par->getType()) == aa->getTypeStoreSize(arg->getType())) {
                        makeAlias(argV, wrapValue(var_par));
                    }
                }
            }
        }
    }
}

bool AAAnalyzer::handle_pointer_function_calls(DyckCallGraphNode* caller) {
    FUNCTION_COUNT++;

    bool ret = false;

    set<PointerCall*>& pointercalls = caller->getPointerCalls();
    set<PointerCall*>::iterator mit = pointercalls.begin();

    // print in console
    int PTCALL_TOTAL = pointercalls.size();
    int PTCALL_COUNT = 0;
    if (PTCALL_TOTAL == 0) outs() << "Handling indirect calls in Function #" << FUNCTION_COUNT << "... 100%, 100%. Done!\r";

    while (mit != pointercalls.end()) {
        // print in console
        int percentage = ((++PTCALL_COUNT)*100 / PTCALL_TOTAL);
        outs() << "Handling indirect calls in Function #" << FUNCTION_COUNT << "... " << percentage << "%, \r";

        PointerCall * pcall = *mit;
        Type* fty = pcall->calledValue->getType()->getPointerElementType();
        assert(fty->isFunctionTy() && "Error in AAAnalyzer::handle_pointer_function_calls!");

        // handle each unhandled, possible function
        set<Value*> equivAndTypeCompSet;
        const set<Value*>* equivSet = aa->getAliasSet(pcall->calledValue);//dgraph->retrieveDyckVertex(pcall->calledValue).first->getEquivalentSet();
        set<Function*>* cands = this->getCompatibleFunctions((FunctionType*) fty);
        set_intersection(cands->begin(), cands->end(),
                equivSet->begin(), equivSet->end(),
                inserter(equivAndTypeCompSet, equivAndTypeCompSet.begin()));


        set<Value*> unhandled_function;
        set<Function*>* maycallfuncs = &(pcall->mayAliasedCallees);
        set_difference(equivAndTypeCompSet.begin(), equivAndTypeCompSet.end(),
                maycallfuncs->begin(), maycallfuncs->end(),
                inserter(unhandled_function, unhandled_function.begin()));

        // print in console
        int CAND_TOTAL = unhandled_function.size();
        int CAND_COUNT = 0;
        if (CAND_TOTAL == 0 || pcall->mustAliasedPointerCall) {
            outs() << "Handling indirect calls in Function #" << FUNCTION_COUNT << "... " << "100%, 100%. Done!\r";
            mit++;
            continue;
        }

        auto pfit = unhandled_function.begin();
        while (pfit != unhandled_function.end()) {
            Function * mayAliasedFunctioin = (Function*) (*pfit);
            // print in console
            outs() << "Handling indirect calls in Function #" << FUNCTION_COUNT << "... " << percentage << "%, " << ((100 * (++CAND_COUNT)) / CAND_TOTAL) << "%         \r";

            AliasAnalysis::AliasResult ar = aa->alias(mayAliasedFunctioin, pcall->calledValue);
            //if (ar == AliasAnalysis::MayAlias || ar == AliasAnalysis::MustAlias) {
            ret = true;
            maycallfuncs->insert(mayAliasedFunctioin);

            handle_common_function_call(pcall, caller, callgraph->getOrInsertFunction(mayAliasedFunctioin));
            handle_lib_invoke_call_inst(pcall->instruction, mayAliasedFunctioin, &(pcall->args), caller);

            if (ar == AliasAnalysis::MustAlias) {
                // print in console
                pcall->mustAliasedPointerCall = true;
                pcall->mayAliasedCallees.clear();
                pcall->mayAliasedCallees.insert(mayAliasedFunctioin);
                outs() << "Handling indirect calls in Function #" << FUNCTION_COUNT << "... " << "100%, 100%. Done!\r";
                break;
            }
            //}
            pfit++;
        }

        mit++;
    }

    return ret;
}

void AAAnalyzer::handle_lib_invoke_call_inst(Value* ret, Function* f, vector<Value*>* args, DyckCallGraphNode* parent) {
    // args must be the real arguments, not the parameters.
    if (!f->empty() || f->isIntrinsic())
        return;

    const string& functionName = f->getName().str();
    switch (args->size()) {
        case 1:
        {
            if (functionName == "strdup" || functionName == "__strdup" || functionName == "strdupa") {
                // content alias r/1st
                this->makeContentAlias(wrapValue(args->at(0)), wrapValue(ret));
            }
        }
            break;
        case 2:
        {
            if (functionName == "strcat" || functionName == "strcpy") {
                if (ret != NULL) {
                    DyckVertex * dst_ptr = wrapValue(args->at(0));
                    DyckVertex * src_ptr = wrapValue(args->at(1));

                    this->makeContentAlias(dst_ptr, src_ptr);
                    this->makeAlias(wrapValue(ret), dst_ptr);
                } else {
                    errs() << "ERROR strcat/cpy does not return.\n";
                    exit(1);
                }
            } else if (functionName == "strndup" || functionName == "strndupa") {
                // content alias r/1st
                this->makeContentAlias(wrapValue(args->at(0)), wrapValue(ret));
            } else if (functionName == "strstr" || functionName == "strcasestr") {
                // content alias r/2nd
                this->makeContentAlias(wrapValue(args->at(1)), wrapValue(ret));
                // alias r/1st
                this->makeAlias(wrapValue(ret), wrapValue(args->at(0)));
            } else if (functionName == "strchr" || functionName == "strrchr" || functionName == "strchrnul" || functionName == "rawmemchr") {
                // alias r/1st
                this->makeAlias(wrapValue(ret), wrapValue(args->at(0)));
            } else if (functionName == "strtok") {
                // content alias r/1st
                this->makeContentAlias(wrapValue(args->at(0)), wrapValue(ret));
            }
        }
            break;
        case 3:
        {
            if (functionName == "strncat"
                    || functionName == "strncpy"
                    || functionName == "memcpy"
                    || functionName == "memmove") {
                if (ret != NULL) {
                    DyckVertex * dst_ptr = wrapValue(args->at(0));
                    DyckVertex * src_ptr = wrapValue(args->at(1));

                    this->makeContentAlias(dst_ptr, src_ptr);
                    this->makeAlias(wrapValue(ret), dst_ptr);
                } else {
                    errs() << "ERROR strncat/cpy does not return.\n";
                    exit(1);
                }
            } else if (functionName == "memchr" || functionName == "memrchr" || functionName == "memset") {
                // alias r/1st
                this->makeAlias(wrapValue(ret), wrapValue(args->at(0)));
            } else if (functionName == "strtok_r" || functionName == "__strtok_r") {
                // content alias r/1st
                this->makeContentAlias(wrapValue(args->at(0)), wrapValue(ret));
            }
        }
            break;
        case 4:
        {
            if (functionName == "pthread_create") {
                vector<Value*> xargs;
                xargs.push_back(args->at(3));
                DyckCallGraphNode* parent = callgraph->getOrInsertFunction(f);
                this->handle_invoke_call_inst(NULL, args->at(2), &xargs, parent);
            }
        }
            break;
        default: break;
    }
}
