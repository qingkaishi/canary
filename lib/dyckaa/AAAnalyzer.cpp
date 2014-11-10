/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "AAAnalyzer.h"

#define ARRAY_SIMPLIFIED

AAAnalyzer::AAAnalyzer(Module* m, AliasAnalysis* a, DyckGraph* d, bool CG) {
    module = m;
    aa = a;
    dgraph = d;

    recordCGInfo = CG;
}

AAAnalyzer::~AAAnalyzer() {
}

void AAAnalyzer::start_intra_procedure_analysis() {
    this->initFunctionGroups();
}

void AAAnalyzer::end_intra_procedure_analysis() {
    this->destroyFunctionGroups();
}

void AAAnalyzer::start_inter_procedure_analysis() {
    // do nothing
    Function * gf = module->getFunction("pthread_getspecific");
    Function * sf = module->getFunction("pthread_setspecific");
    Function * cf = module->getFunction("pthread_key_create");

    if (gf || sf || cf) {
        errs() << "Warning: dyckaa does not handle pthread_getspecific/setspecific/key_create\n";
    }

    /* for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
            Function* f = iterF;
            if(f->empty() && !f->isIntrinsic()){
                outs() << *f <<"\n";
            }
        }*/
}

void AAAnalyzer::end_inter_procedure_analysis() {
    set<FunctionWrapper *>::iterator dfit = callgraph.begin();
    while (dfit != callgraph.end()) {
        FunctionWrapper * df = *dfit;
        set<PointerCall*>& unhandled = df->getPointerCalls();

        set<PointerCall*>::iterator pcit = unhandled.begin();
        while (pcit != unhandled.end()) {
            PointerCall* pc = *pcit;
            if (!pc->handled) // if it is handled, we assume there exists one function definitely aliases with the function pointer
                unhandled_call_insts.insert((Instruction*) pc->ret);

            pcit++;
        }

        dfit++;
    }

    int num = 0;
    /// @TODO delete unnecessary null-value dyckvertices
    set<DyckVertex*>& allver = dgraph->getVertices();
    set<DyckVertex*>& reps = dgraph->getRepresentatives();
    set<DyckVertex*>::iterator rit = reps.begin();
    while (rit != reps.end()) {
        DyckVertex* rep = *rit;

        // for each value in rep's equiv class, if it's the 2nd null-value vertex, delete it
        bool has_value_null = (rep->getValue() == NULL);
        set<DyckVertex*> * eset = rep->getEquivalentSet();
        set<DyckVertex*>::iterator esetIt = eset->begin();
        while (esetIt != eset->end()) {
            DyckVertex* d = *esetIt;
            if (d->getValue() == NULL && d != rep) {
                if (!has_value_null) {
                    has_value_null = true;
                } else {
                    allver.erase(d);
                    eset->erase(esetIt++);
                    delete d;
                    num++;
                    continue;
                }
            }

            esetIt++;
        }

        if (eset->size() == 1 && rep == NULL && !rep->isBridge()) {
            allver.erase(rep);
            reps.erase(rit++);
            delete rep;
            num++;
            continue;
        }

        rit++;
    }
    outs() << "\n# Unnecessary nodes: " << num << "(" << (num * 100 / (allver.size() + num)) << "%)";
}

bool AAAnalyzer::intra_procedure_analysis() {
    long instNum = 0;
    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function* f = iterF;
        FunctionWrapper* df = callgraph.getFunctionWrapper(f);
        for (ilist_iterator<BasicBlock> iterB = f->getBasicBlockList().begin(); iterB != f->getBasicBlockList().end(); iterB++) {
            for (ilist_iterator<Instruction> iterI = iterB->getInstList().begin(); iterI != iterB->getInstList().end(); iterI++) {
                Instruction *inst = iterI;
                instNum++;

                handle_inst(inst, df);
            }
        }
    }
    outs() << "# Instructions: " << instNum << "\n";
    outs() << "# Functions: " << module->getFunctionList().size() << "\n";
    return true; // finished
}

bool AAAnalyzer::inter_procedure_analysis() {
    bool finished = true;
    set<FunctionWrapper *>::iterator dfit = callgraph.begin();
    while (dfit != callgraph.end()) {
        FunctionWrapper * df = *dfit;
        if (handle_functions(df)) {
            finished = false;
        }
        ++dfit;
    }

    return finished;
}

void AAAnalyzer::getUnhandledCallInstructions(set<Instruction*>* ret) {
    ret->insert(unhandled_call_insts.begin(), unhandled_call_insts.end());
}

//// The followings are private functions

int AAAnalyzer::isCompatible(FunctionType * t1, FunctionType * t2) {
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

        for (Value::use_iterator I = f->use_begin(), E = f->use_end(); I != E; ++I) {
            User *U = *I;
            Type* origTy = NULL, *castTy = NULL;
            if (isa<Instruction>(U)) {
                if (((Instruction*) U)->isCast()) {
                    Value* v1 = (Value*) U;
                    Value *v2 = ((Instruction*) U)->getOperand(0);

                    origTy = v2->getType();
                    castTy = v1->getType();
                }
            } else if (isa<ConstantExpr>(U)) {
                if (((ConstantExpr*) U)->isCast()) {
                    Value* v1 = (Value*) U;
                    Value *v2 = ((ConstantExpr*) U)->getOperand(0);

                    origTy = v2->getType();
                    castTy = v1->getType();
                }
            } else {
                errs() << "Warning: unknown user of a function" << *U << "\n";
            }

            if (origTy != NULL && origTy->isPointerTy() && origTy->getPointerElementType()->isFunctionTy()
                    && castTy->isPointerTy() && castTy->getPointerElementType()->isFunctionTy()) {
                combineFunctionGroups((FunctionType*) origTy->getPointerElementType(), (FunctionType*) castTy->getPointerElementType());
            }
        }

        FunctionType * fty = (FunctionType *) ((PointerType*) f->getType())->getPointerElementType();

        FunctionTypeNode * root = this->initFunctionGroup(fty);
        root->compatibleFuncs.insert(f);
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

        outs() << "[CANARY] Combining " << *ft1 << " and " << *ft2 << "... \n";

        FunctionTypeNode * ftn1 = this->initFunctionGroup(ft1);
        FunctionTypeNode * ftn2 = this->initFunctionGroup(ft2);

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
    if (fieldIndex >= -1) {
        errs() << "ERROR in addField: " << fieldIndex << "\n";
        exit(1);
    }

    DyckVertex* valrep = val->getRepresentative();

    if (field == NULL) {
        set<DyckVertex*>* valrepset = valrep->getOutVertices((void*) (fieldIndex));
        if (valrepset != NULL && !valrepset->empty()) {
            field = *(valrepset->begin());
        } else {
            field = dgraph->retrieveDyckVertex(NULL).first;
            valrep->addTarget(field, (void*) (fieldIndex));
        }
    } else {
        valrep->addTarget(field, (void*) (fieldIndex));
    }

    return field;
}

/// if one of add and val is null, create and return it
/// otherwise return the ptr;

DyckVertex* AAAnalyzer::addPtrTo(DyckVertex* address, DyckVertex* val) {
    if (address == NULL && val == NULL) {
        errs() << "ERROR in addPtrTo\n";
        exit(1);
    } else if (address == NULL) {
        address = dgraph->retrieveDyckVertex(NULL).first;
        address->addTarget(val->getRepresentative(), (void*) DEREF_LABEL);
        return address;
    } else if (val == NULL) {
        DyckVertex* addrep = address->getRepresentative();
        set<DyckVertex*>* derefset = addrep->getOutVertices((void*) DEREF_LABEL);
        if (derefset != NULL && !derefset->empty()) {
            val = *(derefset->begin());
        } else {
            val = dgraph->retrieveDyckVertex(NULL).first;
            addrep->addTarget(val, (void*) DEREF_LABEL);
        }

        return val;
    } else {
        DyckVertex* addrep = address->getRepresentative();
        addrep->addTarget(val->getRepresentative(), (void*) DEREF_LABEL);
        return addrep;
    }

}

/// make them alias

void AAAnalyzer::makeAlias(DyckVertex* x, DyckVertex* y) {
    // combine x's rep and y's rep
    dgraph->combine(x, y);
}

void AAAnalyzer::makeContentAlias(DyckVertex* x, DyckVertex* y) {
    DyckVertex * x_content = addPtrTo(x, NULL);
    DyckVertex * y_content = addPtrTo(y, NULL);

    this->makeAlias(x_content, y_content);
}

DyckVertex* AAAnalyzer::handle_gep(GEPOperator* gep) {
    Value * ptr = gep->getPointerOperand();
    DyckVertex* current = wrapValue(ptr);

    gep_type_iterator preGTI = gep_type_begin(gep); // preGTI is the PointerTy of ptr
    gep_type_iterator GTI = gep_type_begin(gep); // GTI is the PointerTy of ptr
    if (GTI != gep_type_end(gep))
        GTI++; // ptr's element type, e.g. struct

    int num_indices = gep->getNumIndices();
    int idxidx = 0;
    while (idxidx < num_indices) {
        Value * idx = gep->getOperand(++idxidx);
        if (/*!isa<ConstantInt>(idx) ||*/ !preGTI->isSized()) {
            // current->addProperty("unknown-offset", (void*) 1);
            //break;
        } else if ((*preGTI)->isStructTy()) {
            // example: gep y 0 constIdx
            // s1: y--deref-->?1--(-2-constIdx)-->?2
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
            DyckVertex* field = this->addField(theStruct, -2 - fieldIdx, NULL);

            // s2: ?3--deref-->?2
            DyckVertex* fieldPtr = this->addPtrTo(NULL, field);

            /// the label representation and feature impl is temporal. @FIXME
            // s3: y--fieldIdx-->?3
            current->getRepresentative()->addTarget(fieldPtr->getRepresentative(), (void*) (fieldIdx));

            // update current
            current = fieldPtr;
        } else if ((*preGTI)->isPointerTy() || (*preGTI)->isArrayTy()) {
#ifndef ARRAY_SIMPLIFIED
            current = addPtrOffset(current, getConstantIntRawData(cast<ConstantInt>(idx)) * dl.getTypeAllocSize(*GTI), dgraph);
#endif
        } else {
            errs() << "ERROR in handle_gep: unknown type:\n";
            errs() << "Type Id: " << (*preGTI)->getTypeID() << "\n";
            exit(1);
        }

        if (GTI != gep_type_end(gep))
            GTI++;
        if (preGTI != gep_type_end(gep))
            preGTI++;
    }

    return current;
}

// constant is handled here.

DyckVertex* AAAnalyzer::wrapValue(Value * v) {
    // if the vertex of v exists, return it, otherwise create one
    pair < DyckVertex*, bool> retpair = dgraph->retrieveDyckVertex(v);
    if (retpair.second) {
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
            switch (opcode) {
                case 23: // BinaryConstantExpr "and"
                case 24: // BinaryConstantExpr "or"
                {
                    // do nothing
                }
                    break;
                default:
                {
                    errs() << "ERROR when handle the following constant expression\n";
                    errs() << *v << "\n";
                    errs() << ((ConstantExpr*) v)->getOpcode() << "\n";
                    errs() << ((ConstantExpr*) v)->getOpcodeName() << "\n";
                    errs().flush();
                    exit(-1);
                }
                    break;
            }
        }
    } else if (isa<ConstantArray>(v)) {
#ifndef ARRAY_SIMPLIFIED
        DyckVertex* ptr = addPtrTo(NULL, vdv, dgraph);
        DyckVertex* current = ptr;

        Constant * vAgg = (Constant*) v;
        int numElmt = vAgg->getNumOperands();
        for (int i = 0; i < numElmt; i++) {
            Value * vi = vAgg->getOperand(i);
            DyckVertex* viptr = addPtrOffset(current, i * dl.getTypeAllocSize(vi->getType()), dgraph);
            addPtrTo(viptr, wrapValue(vi, dgraph, dl), dgraph);
        }
#else
        Constant * vAgg = (Constant*) v;
        int numElmt = vAgg->getNumOperands();
        for (int i = 0; i < numElmt; i++) {
            Value * vi = vAgg->getOperand(i);
            makeAlias(vdv, wrapValue(vi));
        }
#endif
    } else if (isa<ConstantStruct>(v)) {
        //DyckVertex* ptr = addPtrTo(NULL, vdv);
        //DyckVertex* current = ptr;

        Constant * vAgg = (Constant*) v;
        int numElmt = vAgg->getNumOperands();
        for (int i = 0; i < numElmt; i++) {
            Value * vi = vAgg->getOperand(i);
            addField(vdv, -2 - i, wrapValue(vi));
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

void AAAnalyzer::handle_inst(Instruction *inst, FunctionWrapper * parent_func) {
    //outs()<<*inst<<"\n"; outs().flush();
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
            DyckVertex* aggV = wrapValue(agg);

            Type* aggTy = agg->getType();

            ArrayRef<unsigned> indices = ((ExtractValueInst*) inst)->getIndices();
            DyckVertex* currentStruct = aggV;

            for (unsigned int i = 0; i < indices.size(); i++) {
                if (isa<CompositeType>(aggTy) && aggTy->isSized()) {
                    if (!aggTy->isStructTy()) {
                        aggTy = ((CompositeType*) aggTy)->getTypeAtIndex(indices[i]);
#ifndef ARRAY_SIMPLIFIED
                        current = addPtrOffset(current, (int) indices[i] * dl.getTypeAllocSize(aggTy), dgraph);
#endif
                        if (i == indices.size() - 1) {
                            this->makeAlias(currentStruct, wrapValue(inst));
                        }
                    } else {
                        aggTy = ((CompositeType*) aggTy)->getTypeAtIndex(indices[i]);

                        if (i != indices.size() - 1) {
                            currentStruct = this->addField(currentStruct, -2 - (int) indices[i], NULL);
                        } else {
                            currentStruct = this->addField(currentStruct, -2 - (int) indices[i], wrapValue(inst));
                        }
                    }
                } else {
                    break;
                }
            }
        }
            break;
        case Instruction::InsertValue:
        {
            DyckVertex* resultV = wrapValue(inst);
            Value * agg = ((InsertValueInst*) inst)->getAggregateOperand();
            if (!isa<UndefValue>(agg)) {
                makeAlias(resultV, wrapValue(agg));
            }

            Value * val = ((InsertValueInst*) inst)->getInsertedValueOperand();
            DyckVertex* insertedVal = wrapValue(val);

            Type *aggTy = inst->getType();

            ArrayRef<unsigned> indices = ((InsertValueInst*) inst)->getIndices();

            DyckVertex* currentStruct = resultV;

            for (unsigned int i = 0; i < indices.size(); i++) {
                if (isa<CompositeType>(aggTy) && aggTy->isSized()) {
                    if (!aggTy->isStructTy()) {
                        aggTy = ((CompositeType*) aggTy)->getTypeAtIndex(indices[i]);
#ifndef ARRAY_SIMPLIFIED
                        current = addPtrOffset(current, (int) indices[i] * dl.getTypeAllocSize(aggTy), dgraph);
#endif
                        if (i == indices.size() - 1) {
                            this->makeAlias(currentStruct, insertedVal);
                        }
                    } else {
                        aggTy = ((CompositeType*) aggTy)->getTypeAtIndex(indices[i]);

                        if (i != indices.size() - 1) {
                            currentStruct = this->addField(currentStruct, -2 - (int) indices[i], NULL);
                        } else {
                            currentStruct = this->addField(currentStruct, -2 - (int) indices[i], insertedVal);
                        }
                    }
                } else {
                    break;
                }
            }
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
            InvokeInst * invoke = (InvokeInst*) inst;
            LandingPadInst* lpd = invoke->getLandingPadInst();
            parent_func->addLandingPad(invoke, lpd);

            Value * cv = invoke->getCalledValue();
            vector<Value*> args;
            for (unsigned i = 0; i < invoke->getNumArgOperands(); i++) {
                args.push_back(invoke->getArgOperand(i));
            }

            this->handle_invoke_call_inst(invoke, cv, &args, parent_func);
        }
            break;
        case Instruction::Call:
        {
            CallInst * callinst = (CallInst*) inst;

            if (callinst->isInlineAsm()) {
                unhandled_call_insts.insert(callinst);
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

void AAAnalyzer::handle_invoke_call_inst(Value* ret, Value* cv, vector<Value*>* args, FunctionWrapper* parent) {
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
                PointerCall* pcall = new PointerCall(ret, cv, getCompatibleFunctions((FunctionType*) (cvcopy->getType()->getPointerElementType())), args);
                parent->addPointerCall(pcall);
            }
        } else if (isa<GlobalAlias>(cv)) {
            Value * cvcopy = cv;
            while (isa<GlobalAlias>(cvcopy)) {
                cvcopy = ((GlobalAlias*) cvcopy)->getAliasedGlobal()->stripPointerCastsNoFollowAliases();
            }

            if (isa<Function>(cvcopy)) {
                this->handle_lib_invoke_call_inst(ret, (Function*) cvcopy, args, parent);
                parent->addCommonCall(new CommonCall(ret, (Function*) cvcopy, args));
            } else {
                PointerCall* pcall = new PointerCall(ret, cv, getCompatibleFunctions((FunctionType*) (cvcopy->getType()->getPointerElementType())), args);
                parent->addPointerCall(pcall);
            }
        } else {
            PointerCall * pcall = new PointerCall(ret, cv, getCompatibleFunctions((FunctionType*) (cv->getType()->getPointerElementType())), args);
            parent->addPointerCall(pcall);
        }
    }
}

// ci is callinst or invokeinst

void AAAnalyzer::handle_common_function_call(Call* c, FunctionWrapper* caller, FunctionWrapper* callee) {
    //landingpad<->resume
    if (c->ret != NULL) {
        Value* lpd = caller->getLandingPad(c->ret);
        if (lpd != NULL) {
            DyckVertex* lpdVertex = wrapValue(lpd);
            set<Value*>& res = callee->getResumes();
            set<Value*>::iterator resIt = res.begin();
            while (resIt != res.end()) {
                makeAlias(wrapValue(*resIt), lpdVertex);
                resIt++;
            }
        }
    }

    //return<->call
    set<Value*>& rets = callee->getReturns();
    if (c->ret != NULL) {
        set<Value*>::iterator retIt = rets.begin();
        while (retIt != rets.end()) {
            makeAlias(wrapValue(*retIt), wrapValue(c->ret));
            retIt++;
        }
    }
    // parameter<->arg
    unsigned int numOfArgOp = c->args.size();

    unsigned argIdx = 0;
    Function * func = callee->getLLVMFunction();
    if (!func->isIntrinsic()) {
        vector<Value*>& parameters = callee->getArgs();

        vector<Value*>::iterator pit = parameters.begin();
        while (pit != parameters.end()) {
            if (numOfArgOp <= argIdx) {
                errs() << "Warning the number of args is less than that of parameters\n";
                errs() << func->getName() << "\n";
                errs() << *(c->ret) << "\n";
                break; //exit(-1);
            }

            Value * arg = c->args[argIdx];
            Value * par = *pit;
            if (!func->empty()) {
                makeAlias(wrapValue(par), wrapValue(arg));
            }

            argIdx++;
            pit++;
        }

        if (func->isVarArg()) {
            vector<Value*>& va_parameters = callee->getVAArgs();
            for (unsigned int i = argIdx; i < numOfArgOp; i++) {

                Value * arg = c->args[i];
                DyckVertex* arg_v = wrapValue(arg);

                vector<Value*>::iterator va_par_it = va_parameters.begin();
                while (va_par_it != va_parameters.end()) {
                    Value* va_par = *va_par_it;
                    makeAlias(arg_v, wrapValue(va_par));

                    va_par_it++;
                }
            }
        }
    } else {
        errs() << "ERROR in handle_common_function_call(...):\n";
        errs() << func->getName() << " can not be an intrinsic.\n";
        exit(-1);
    }
}

bool AAAnalyzer::handle_functions(FunctionWrapper* caller) {
    bool ret = false;

    set<CommonCall*>* commonCalls = NULL;
    if (recordCGInfo) {
        commonCalls = caller->getCommonCallsForCG();
    }
    set<CommonCall*>& callInsts = caller->getCommonCalls();
    set<CommonCall*>::iterator cit = callInsts.begin();
    while (cit != callInsts.end()) {
        Value * cv = (*cit)->calledValue;

        if (isa<Function>(cv)) {
            ret = true;
            //(*cit)->ret is the return value, it is also the call inst
            handle_common_function_call((*cit), caller, callgraph.getFunctionWrapper((Function*)cv));

            if (recordCGInfo) {
                commonCalls->insert(*cit);
            } else {
                delete *cit;
            }

            callInsts.erase(cit);
        } else {
            // errs
            errs() << "WARNING in handle_function(...):\n";
            errs() << *cv << " should be handled.\n";
            exit(-1);
        }
        cit++;
    }

    set<PointerCall*>& pointercalls = caller->getPointerCalls();
    set<PointerCall*>::iterator mit = pointercalls.begin();
    while (mit != pointercalls.end()) {
        //Value * inst = mit->first;
        PointerCall * pcall = *mit;
        set<Function*>* cands = &(pcall->calleeCands);

        set<Function*>* maycallfuncs = NULL;
        if (recordCGInfo) {
            maycallfuncs = &(pcall->handledCallees);
        }

        // cv, numOfArguments
        set<Function*>::iterator pfit = cands->begin();
        while (pfit != cands->end()) {
            AliasAnalysis::AliasResult ar = ((DyckAliasAnalysis*) aa)->function_alias(*pfit, (CallInst*) pcall->ret); // all invokes have been lowered to calls
            if (ar == AliasAnalysis::MayAlias || ar == AliasAnalysis::MustAlias) {
                ret = true;
                pcall->handled = true;

                handle_common_function_call(pcall, caller, callgraph.getFunctionWrapper(*pfit));

                if (recordCGInfo) {
                    maycallfuncs->insert(*pfit);
                }

                this->handle_lib_invoke_call_inst(pcall->ret, *pfit, &(pcall->args), caller);

                cands->erase(pfit++);

                if (ar == AliasAnalysis::MustAlias) {
                    cands->clear();
                    break;
                }
            } else {
                pfit++;
            }
        }

        mit++;
    }

    return ret;
}

void AAAnalyzer::handle_lib_invoke_call_inst(Value* ret, Function* f, vector<Value*>* args, FunctionWrapper* parent) {
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
            if (functionName == "strncat" || functionName == "strncpy") {
                if (ret != NULL) {
                    DyckVertex * dst_ptr = wrapValue(args->at(0));
                    DyckVertex * src_ptr = wrapValue(args->at(1));

                    this->makeContentAlias(dst_ptr, src_ptr);
                    this->makeAlias(wrapValue(ret), dst_ptr);
                } else {
                    errs() << "ERROR strncat/cpy does not return.\n";
                    exit(1);
                }
            } else if (functionName == "memchr" || functionName == "memrchr") {
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
                Value * ret = NULL;
                vector<Value*> xargs;
                xargs.push_back(args->at(3));
                FunctionWrapper* parent = callgraph.getFunctionWrapper(f);
                this->handle_invoke_call_inst(ret, args->at(2), &xargs, parent);
            }
        }
            break;
        default: break;
    }
}
