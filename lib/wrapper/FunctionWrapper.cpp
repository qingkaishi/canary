#include "FunctionWrapper.h"

CommonCall::CommonCall(Function* f, Value* ci) {
    this->callee = f;
    this->ret = ci;
}

int FunctionWrapper::global_idx = 0;

FunctionWrapper::FunctionWrapper(Function *f) {
    llvm_function = f;
    idx = global_idx++;

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
    
    it = fpMapForCG.begin();
    while (it != fpMapForCG.end()) {
        delete (it->second);
        it++;
    }
    
    set<CommonCall *>::iterator cit =  callInsts.begin();
    while(cit!=callInsts.end()){
        delete *cit;
        cit++;
    }
    
    cit =  callInstsForCG.begin();
    while(cit!=callInstsForCG.end()){
        delete *cit;
        cit++;
    }
}

int FunctionWrapper::getIndex(){
    return idx;
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

set<CommonCall *>* FunctionWrapper::getCommonCallInstsForCG() {
    return &callInstsForCG;
}

set<Function*>* FunctionWrapper::getFPCallInstsForCG(Value * ci){
    if(fpMapForCG.count(ci)){
        return fpMapForCG[ci];
    } else {
        set<Function*> * ciset = new set<Function*>;
        fpMapForCG.insert(pair<Value*, set<Function*> *>(ci, ciset));
        return ciset;
    }
}
    
map<Value *, set<Function*>*>* FunctionWrapper::getFPCallInstsForCG(){
    return &fpMapForCG;
}
