#include "AAAnalyzer.h"

#define ARRAY_SIMPLIFIED

AAAnalyzer::AAAnalyzer(Module* m, AliasAnalysis* a, DyckGraph* d, bool CG) {
    module = m;
    aa = a;
    dgraph = d;

    recordCGInfo = CG;
}

AAAnalyzer::~AAAnalyzer() {
    for (unsigned int i = 0; i < tempCalls.size(); i++) {
        // this is necessary
        tempCalls[i]->eraseFromParent();
    }

    set<FunctionWrapper*>::iterator it = wrapped_functions.begin();
    while (it != wrapped_functions.end()) {
        delete (*it);
        it++;
    }
    wrapped_functions.clear();
    wrapped_functions_map.clear();
}

void AAAnalyzer::start_intra_procedure_analysis() {
    this->initFunctionGroups();
}

void AAAnalyzer::end_intra_procedure_analysis() {
    this->destroyFunctionGroups();
}

void AAAnalyzer::start_inter_procedure_analysis() {
    // do nothing
}

void AAAnalyzer::end_inter_procedure_analysis() {
}

bool AAAnalyzer::intra_procedure_analysis() {
    long instNum = 0;
    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function* f = iterF;

        FunctionWrapper* df = new FunctionWrapper(f);
        wrapped_functions_map.insert(pair<Function*, FunctionWrapper *>(f, df));
        wrapped_functions.insert(df);
        for (ilist_iterator<BasicBlock> iterB = f->getBasicBlockList().begin(); iterB != f->getBasicBlockList().end(); iterB++) {
            for (ilist_iterator<Instruction> iterI = iterB->getInstList().begin(); iterI != iterB->getInstList().end(); iterI++) {
                Instruction *inst = iterI;
                instNum++;

                if (isa<CallInst>(inst)) {
                    CallInst* call = (CallInst*) inst;
                    Function* f = call->getCalledFunction();


                    if (f != NULL && f->getName().str() == "pthread_create") {
                        if (call->getNumArgOperands() == 4) {
                            vector<Value*>* args = new vector<Value*>;
                            args->push_back(call->getArgOperand(3));
                            ArrayRef<Value*> * Args = new ArrayRef<Value*>(*args);
                            CallInst* newCall = CallInst::Create(call->getArgOperand(2), *Args, "THREAD_CREATE", inst);
                            handle_inst(newCall, df);
                            tempCalls.push_back(newCall);
                        } else {
                            errs() << "ERROR pthread_create with less than 4 args.\n";
                            errs() << *call << "\n";
                            exit(-1);
                        }
                    }
                }
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
    set<FunctionWrapper *>::iterator dfit = wrapped_functions.begin();
    while (dfit != wrapped_functions.end()) {
        FunctionWrapper * df = *dfit;
        if (handle_functions(df)) {
            finished = false;
        }
        ++dfit;
    }

    return finished;
}

void AAAnalyzer::getValuesEscapedFromThreadCreate(set<Value*>* ret) {
    for (unsigned int i = 0; i < tempCalls.size(); i++) {
        ret->insert(tempCalls[i]->getArgOperand(0));
    }
}

//// call graph printer

void AAAnalyzer::printCallGraph(const string& mIdentifier) {
    string dotfilename("");
    dotfilename.append(mIdentifier);
    dotfilename.append(".maycg.dot");

    FILE * fout = fopen(dotfilename.data(), "w+");
    fprintf(fout, "digraph maycg {\n");

    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function* f = iterF;
        if (f->isIntrinsic()) {
            continue;
        }

        if (wrapped_functions_map.count(f)) {
            FunctionWrapper * fw = wrapped_functions_map[f];
            fprintf(fout, "\tf%d[label=\"%s\"]\n", fw->getIndex(), f->getName().data());
        } else {
            errs() << "ERROR in printCG when declare functions.\n";
            exit(-1);
        }
    }

    set<FunctionWrapper*>::iterator fwIt = wrapped_functions.begin();
    while (fwIt != wrapped_functions.end()) {
        FunctionWrapper* fw = *fwIt;
        set<CommonCall*>* commonCalls = fw->getCommonCallInstsForCG();
        set<CommonCall*>::iterator comIt = commonCalls->begin();
        while (comIt != commonCalls->end()) {
            CommonCall* cc = *comIt;

            if (wrapped_functions_map.count(cc->callee)) {
                Value * ci = cc->ret;
                std::string s;
                raw_string_ostream rso(s);
                rso << *(ci);
                string& edgelabel = rso.str();
                for (unsigned int i = 0; i < edgelabel.length(); i++) {
                    if (edgelabel[i] == '\"') {
                        edgelabel[i] = '`';
                    }

                    if (edgelabel[i] == '\n') {
                        edgelabel[i] = ' ';
                    }
                }
                fprintf(fout, "\tf%d->f%d[label=\"%s\"]\n", fw->getIndex(), wrapped_functions_map[cc->callee]->getIndex(), edgelabel.data());
            } else {
                errs() << "ERROR in printCG when print common function calls.\n";
                exit(-1);
            }
            comIt++;
        }

        map<Value *, set<Function*>*>* fpCallsMap = fw->getFPCallInstsForCG();
        map<Value *, set<Function*>*>::iterator fpIt = fpCallsMap->begin();
        while (fpIt != fpCallsMap->end()) {
            Value * callInst = fpIt->first;
            set<Function*>* mayCalled = fpIt->second;
            
            Value* calledValue = NULL;
            if(isa<CallInst>(callInst)){
                calledValue=((CallInst*)callInst)->getCalledValue();
            }else if(isa<InvokeInst>(callInst)){
                calledValue=((InvokeInst*)callInst)->getCalledValue();
            }

            std::string s;
            raw_string_ostream rso(s);
            rso << *(callInst);
            string& edgelabel = rso.str(); // edge label is the call inst
            for (unsigned int i = 0; i < edgelabel.length(); i++) {
                if (edgelabel[i] == '\"') {
                    edgelabel[i] = '`';
                }

                if (edgelabel[i] == '\n') {
                    edgelabel[i] = ' ';
                }
            }

            set<Function*>::iterator mcIt = mayCalled->begin();
            while (mcIt != mayCalled->end()) {
                Function * mcf = *mcIt;
                if (wrapped_functions_map.count(mcf)) {

                    int clevel = this->isCompatible((FunctionType*) (mcf->getType()->getPointerElementType()), (FunctionType*) (calledValue->getType()->getPointerElementType()));
                    fprintf(fout, "\tf%d->f%d[label=\"%s\"] // confidence-level = %d\n", fw->getIndex(), wrapped_functions_map[mcf]->getIndex(), edgelabel.data(), clevel);
                } else {
                    errs() << "ERROR in printCG when print fp calls.\n";
                    exit(-1);
                }

                mcIt++;
            }

            fpIt++;
        }


        fwIt++;
    }

    fprintf(fout, "}\n");
    fclose(fout);
}

// fp info

void AAAnalyzer::printFunctionPointersInformation(const string& mIdentifier) {
    string dotfilename("");
    dotfilename.append(mIdentifier);
    dotfilename.append(".fp.txt");

    FILE * fout = fopen(dotfilename.data(), "w+");

    set<FunctionWrapper*>::iterator fwIt = wrapped_functions.begin();
    while (fwIt != wrapped_functions.end()) {
        FunctionWrapper* fw = *fwIt;

        map<Value *, set<Function*>*>* fpCallsMap = fw->getFPCallInstsForCG();
        map<Value *, set<Function*>*>::iterator fpIt = fpCallsMap->begin();
        while (fpIt != fpCallsMap->end()) {
            /*Value * callInst = fpIt->first;
            std::string s;
            raw_string_ostream rso(s);
            rso << *(callInst);
            string& edgelabel = rso.str();
            for (unsigned int i = 0; i < edgelabel.length(); i++) {
                if (edgelabel[i] == '\"') {
                    edgelabel[i] = '`';
                }

                if (edgelabel[i] == '\n') {
                    edgelabel[i] = ' ';
                }
            }
            fprintf(fout, "CallInst: %s\n", edgelabel.data()); //call inst
             */
            set<Function*>* mayCalled = fpIt->second;
            fprintf(fout, "%d\n", mayCalled->size()); //number of functions

            // what functions?
            set<Function*>::iterator mcIt = mayCalled->begin();
            while (mcIt != mayCalled->end()) {
                // Function * mcf = *mcIt;
                //fprintf(fout, "%s\n", mcf->getName().data());

                mcIt++;
            }

            //fprintf(fout, "\n");

            fpIt++;
        }


        fwIt++;
    }

    fclose(fout);
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

void AAAnalyzer::initFunctionGroups() {
    for (ilist_iterator<Function> iterF = module->getFunctionList().begin(); iterF != module->getFunctionList().end(); iterF++) {
        Function* f = iterF;
        if (f->isIntrinsic()) {
            continue;
        }

        FunctionType * fty = (FunctionType *) ((PointerType*) f->getType())->getPointerElementType();

        // the type has been handled
        if (functionTyNodeMap.count(fty)) {
            functionTyNodeMap[fty]->root->compatibleFuncs.insert(f);
            continue;
        }

        // iterate all roots to check whether compatible
        // if so, add it to its set
        // otherwise, new a root
        bool found = false;
        set<FunctionTypeNode *>::iterator rit = tyroots.begin();
        while (rit != tyroots.end()) {
            if (isCompatible((*rit)->type, fty)) {
                found = true;
                (*rit)->compatibleFuncs.insert(f);

                FunctionTypeNode * tn = new FunctionTypeNode;
                tn->type = fty;
                tn->root = (*rit);

                functionTyNodeMap.insert(pair<Type*, FunctionTypeNode*>(fty, tn));
                break;
            }
            rit++;
        }

        if (!found) {
            FunctionTypeNode * tn = new FunctionTypeNode;
            tn->type = fty;
            tn->root = tn;
            tn->compatibleFuncs.insert(f);

            tyroots.insert(tn);
            functionTyNodeMap.insert(pair<Type*, FunctionTypeNode*>(fty, tn));
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

DyckVertex* AAAnalyzer::addField(DyckVertex* val, int fieldIndex, DyckVertex* field) {
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
        if (/*!isa<ConstantInt>(idx) ||*/ !GTI->isSized()) {
            // current->addProperty("unknown-offset", (void*) 1);
            break;
        }

        if ((*preGTI)->isStructTy()) {
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
            int fieldIdx = (int) (*(ci->getValue().getRawData()));
            DyckVertex* field = this->addField(theStruct, -2 - fieldIdx, NULL);

            // s2: ?3--deref-->?2
            DyckVertex* fieldPtr = this->addPtrTo(NULL, field);

            /// the label representation and feature impl is temporal. @FIXME
            // s3: y--2-->?3
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
        errs().flush();
        exit(-1);
    } else if (isa<ConstantVector>(v)) {
        errs() << "ERROR when handle the following ConstantVector\n";
        errs() << *v << "\n";
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

void AAAnalyzer::storeCandidateFunctions(FunctionWrapper* parent_func, Value * call, Value * cv) {
    Type *fpty = cv->getType();
    if (fpty->isPointerTy() && fpty->getPointerElementType()->isFunctionTy()) {
        FunctionType * fty = (FunctionType*) fpty->getPointerElementType();
        if (functionTyNodeMap.count(fty)) {
            parent_func->setCandidateFunctions(call, functionTyNodeMap[fty]->root->compatibleFuncs);
        } else {
            // unknown function pointer types (not detected when init)
            // find compatible types from root
            bool found = false;
            set<FunctionTypeNode *>::iterator rit = tyroots.begin();
            while (rit != tyroots.end()) {
                if (isCompatible((*rit)->type, fty)) {
                    found = true;

                    FunctionTypeNode * tn = new FunctionTypeNode;
                    tn->type = fty;
                    tn->root = (*rit);

                    functionTyNodeMap.insert(pair<Type*, FunctionTypeNode*>(fty, tn));

                    parent_func->setCandidateFunctions(call, (*rit)->compatibleFuncs);
                    break;
                }
                rit++;
            }

            if (!found) {
                errs() << "No functions can be aliased with the pointer.\n";
                errs() << *call << "\n";
                errs() << *fty << "\n";
                exit(-1);
            }
        }
    }
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
        case Instruction::Invoke:
        {
            InvokeInst * invoke = (InvokeInst*) inst;
            LandingPadInst* lpd = invoke->getLandingPadInst();
            parent_func->addLandingPad(invoke, lpd);

            Value * cv = invoke->getCalledValue();

            if (isa<Function>(cv)) {
                parent_func->addCommonCallInst(new CommonCall((Function*) cv, invoke));
            } else {
                wrapValue(cv);
                if (isa<ConstantExpr>(cv)) {

                    Value * cvcopy = cv;
                    while (isa<ConstantExpr>(cvcopy) && ((ConstantExpr*) cvcopy)->isCast()) {
                        cvcopy = ((ConstantExpr*) cvcopy)->getOperand(0);
                    }

                    if (isa<Function>(cvcopy)) {
                        parent_func->addCommonCallInst(new CommonCall((Function*) cvcopy, invoke));
                    } else {
                        storeCandidateFunctions(parent_func, invoke, cv);
                    }
                } else {
                    storeCandidateFunctions(parent_func, invoke, cv);
                }
            }
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
        }
            break;

            // other operations
        case Instruction::Call:
        {
            CallInst * callinst = (CallInst*) inst;

            if (callinst->isInlineAsm()) {
                break;
            }

            Value * cv = callinst->getCalledValue();

            if (isa<Function>(cv)) {
                if (((Function*) cv)->isIntrinsic()) {
                    handle_instrinsic(inst);
                } else {
                    parent_func->addCommonCallInst(new CommonCall((Function*) cv, callinst));
                }
            } else {
                wrapValue(cv);
                if (isa<ConstantExpr>(cv)) {

                    Value * cvcopy = cv;
                    while (isa<ConstantExpr>(cvcopy) && ((ConstantExpr*) cvcopy)->isCast()) {
                        cvcopy = ((ConstantExpr*) cvcopy)->getOperand(0);
                    }

                    if (isa<Function>(cvcopy)) {
                        parent_func->addCommonCallInst(new CommonCall((Function*) cvcopy, callinst));
                    } else {
                        storeCandidateFunctions(parent_func, callinst, cv);
                    }
                } else {
                    storeCandidateFunctions(parent_func, callinst, cv);
                }
            }
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

// ci is callinst or invokeinst

void AAAnalyzer::handle_common_function_call(Value* ci, FunctionWrapper* caller, FunctionWrapper* callee) {
    //landingpad<->resume
    Value* lpd = caller->getLandingPad(ci);
    if (lpd != NULL) {
        DyckVertex* lpdVertex = wrapValue(lpd);
        set<Value*>& res = callee->getResumes();
        set<Value*>::iterator resIt = res.begin();
        while (resIt != res.end()) {
            makeAlias(wrapValue(*resIt), lpdVertex);
            resIt++;
        }
    }

    //return<->call
    set<Value*>& rets = callee->getReturns();
    set<Value*>::iterator retIt = rets.begin();
    while (retIt != rets.end()) {
        makeAlias(wrapValue(*retIt), wrapValue(ci));
        retIt++;
    }
    // parameter<->arg
    unsigned int numOfArgOp = 0;
    bool isCallInst = false;
    if (isa<CallInst>(*ci)) {
        isCallInst = true;
        numOfArgOp = ((CallInst*) ci)->getNumArgOperands();
    } else {
        numOfArgOp = ((InvokeInst*) ci)->getNumArgOperands();
    }
    unsigned argIdx = 0;
    Function * func = callee->getLLVMFunction();
    if (!func->isIntrinsic()) {
        vector<Value*>& parameters = callee->getArgs();

        vector<Value*>::iterator pit = parameters.begin();
        while (pit != parameters.end()) {
            if (numOfArgOp <= argIdx) {
                errs() << "Warning the number of args is less than that of parameters\n";
                errs() << func->getName() << "\n";
                errs() << *ci << "\n";
                break; //exit(-1);
            }

            Value * arg = NULL;
            if (isCallInst) {
                arg = ((CallInst*) ci)->getArgOperand(argIdx);
            } else {
                arg = ((InvokeInst*) ci)->getArgOperand(argIdx);
            }
            Value * par = *pit;
            makeAlias(wrapValue(par), wrapValue(arg));

            argIdx++;
            pit++;
        }

        if (func->isVarArg()) {
            vector<Value*>& va_parameters = callee->getVAArgs();
            for (unsigned int i = argIdx; i < numOfArgOp; i++) {

                Value * arg = NULL;
                if (isCallInst) {
                    arg = ((CallInst*) ci)->getArgOperand(i);
                } else {
                    arg = ((InvokeInst*) ci)->getArgOperand(i);
                }
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
        commonCalls = caller->getCommonCallInstsForCG();
    }
    set<CommonCall*>& callInsts = caller->getCommonCallInsts();
    set<CommonCall*>::iterator cit = callInsts.begin();
    while (cit != callInsts.end()) {
        Function * cv = (*cit)->callee;

        if (isa<Function>(cv)) {
            ret = true;
            //(*cit)->ret is the return value, it is also the call inst
            handle_common_function_call((*cit)->ret, caller, (wrapped_functions_map)[cv]);

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

    map<Value *, set<Function*>*>& ci_cand_map = caller->getFPCallInsts();
    map<Value *, set<Function*>*>::iterator mit = ci_cand_map.begin();
    while (mit != ci_cand_map.end()) {
        Value * inst = mit->first;
        set<Function*>* cands = mit->second;

        set<Function*>* maycallfuncs = NULL;
        if (recordCGInfo) {
            maycallfuncs = caller->getFPCallInstsForCG(mit->first);
        }

        // cv, numOfArguments
        int numOfArguments = 0;
        Value * cv = NULL;
        if (isa<CallInst>(inst)) {
            cv = ((CallInst*) (inst))->getCalledValue();
            numOfArguments = ((CallInst*) (inst))->getNumArgOperands();
        } else {
            cv = ((InvokeInst*) (inst))->getCalledValue();
            numOfArguments = ((InvokeInst*) (inst))->getNumArgOperands();
        }

        set<Function*>* funcs = cands;
        set<Function*>::iterator pfit = funcs->begin();
        while (pfit != funcs->end()) {
            if (aa->alias(*pfit, cv) == AliasAnalysis::MayAlias) {
                ret = true;
                handle_common_function_call(inst, caller, (wrapped_functions_map)[*pfit]);

                if (recordCGInfo) {
                    maycallfuncs->insert(*pfit);
                }

                if (numOfArguments == 4 && (*pfit)->getName().str() == "pthread_create") {
                    // create a new call instruction, and record it.
                    // handle the new call inst
                    /// @FIXME may have problems
                    errs() << "WARNING the block in handle_function(...) is not fully tested.\n\n";
                    vector<Value*>* args = new vector<Value*>;
                    args->push_back(((CallInst*) (inst))->getArgOperand(3));
                    ArrayRef<Value*> * Args = new ArrayRef<Value*>(*args);
                    CallInst* newCall = CallInst::Create(((CallInst*) (inst))->getArgOperand(2), *Args, "THREAD_CREATE", (Instruction*) (*cit));

                    tempCalls.push_back(newCall);
                    handle_inst(newCall, caller);
                }

                funcs->erase(pfit++);
            } else {
                pfit++;
            }
        }

        mit++;
    }

    return ret;
}
