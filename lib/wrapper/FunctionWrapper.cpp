#include "FunctionWrapper.h"

CommonCall::CommonCall(Function* f, Value* ci) {
    this->callee = f;
    this->ret = ci;

    if (isa<CallInst>(ci)) {
        unsigned num = ((CallInst*) ci)->getNumArgOperands();
        for (unsigned i = 0; i < num; i++) {
            this->args.push_back(((CallInst*) ci)->getArgOperand(i));
        }
    } else if (isa<InvokeInst>(ci)) {
        unsigned num = ((InvokeInst*) ci)->getNumArgOperands();
        for (unsigned i = 0; i < num; i++) {
            this->args.push_back(((InvokeInst*) ci)->getArgOperand(i));
        }
    } else {
        outs() << "Error in CommonCall\n";
        outs() << *ci;
    }
}

FunctionWrapper::FunctionWrapper(Function *f) {
    llvm_function = f;

    iplist<Argument>& alt = f->getArgumentList();
    iplist<Argument>::iterator it = alt.begin();
    while (it != alt.end()) {
        args.push_back(it);

        it++;
    }
}

FunctionWrapper::~FunctionWrapper() {
    map<Value *, set<Function*>*>::iterator it = ci_cand_map.begin();
    while (it != ci_cand_map.end()) {
        delete (it->second);
        it++;
    }
}

void FunctionWrapper::setCandidateFunctions(Value * ci, set<Function*>& fs) {
    if (!ci_cand_map.count(ci)) {
        set<Function*>* fset = new set<Function*>;
        fset->insert(fs.begin(), fs.end());
        ci_cand_map.insert(pair<Value *, set<Function*>*>(ci, fset));
    } else {
        ci_cand_map[ci]->insert(fs.begin(), fs.end());
    }
}

map<Value *, set<Function*>*>& FunctionWrapper::getFPCallInsts() {
    return ci_cand_map;
}

Function* FunctionWrapper::getLLVMFunction() {
    return llvm_function;
}

set<CommonCall *>& FunctionWrapper::getCommonCallInsts() {
    return callInsts;
}

void FunctionWrapper::addCommonCallInst(CommonCall * call) {
    callInsts.insert(call);
}

void FunctionWrapper::addResume(Value * res) {
    resumes.insert(res);
}

void FunctionWrapper::addLandingPad(Value * invoke, Value * lpad) {
    lpads.insert(pair<Value*, Value*>(invoke, lpad));
}

void FunctionWrapper::addRet(Value * ret) {
    rets.insert(ret);
}

void FunctionWrapper::addArg(Value * arg) {
    args.push_back(arg);
}

void FunctionWrapper::addVAArg(Value* arg) {
    va_args.push_back(arg);
}

vector<Value*>& FunctionWrapper::getArgs() {
    return args;
}

vector<Value*>& FunctionWrapper::getVAArgs() {
    return va_args;
}

set<Value*>& FunctionWrapper::getReturns() {
    return rets;
}

set<Value*>& FunctionWrapper::getResumes() {
    return resumes;
}

Value* FunctionWrapper::getLandingPad(Value * invoke) {
    if(lpads.count(invoke)){
        return lpads[invoke];
    }
    return NULL;
}
