/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "FunctionWrapper.h"

Call::Call(Value* ret, Value * cv, vector<Value*>* args){
    this->calledValue = cv;
    this->ret = ret;
    vector<Value*>::iterator aIt = args->begin();
    while(aIt!=args->end()){
        this->args.push_back(*aIt);
        aIt++;
    }
}

CommonCall::CommonCall(Value* ret, Function * f, vector<Value*>* args) : Call(ret, f, args) {
}

PointerCall::PointerCall(Value* ret, Value* cv, set<Function *>* fs, vector<Value*>* args) : Call(ret, cv, args) {
    this->calleeCands.insert(fs->begin(), fs->end());
    this->handled = false;
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
    set<PointerCall *>::iterator it = pointerCalls.begin();
    while (it != pointerCalls.end()) {
        delete (*it);
        it++;
    }

    it = pointerCallsForCG.begin();
    while (it != pointerCallsForCG.end()) {
        delete (*it);
        it++;
    }

    set<CommonCall *>::iterator cit = commonCalls.begin();
    while (cit != commonCalls.end()) {
        delete *cit;
        cit++;
    }

    cit = commonCallsForCG.begin();
    while (cit != commonCallsForCG.end()) {
        delete *cit;
        cit++;
    }
}

int FunctionWrapper::getIndex() {
    return idx;
}

set<PointerCall *>& FunctionWrapper::getPointerCalls() {
    return pointerCalls;
}

void FunctionWrapper::addPointerCall(PointerCall* call){
    pointerCalls.insert(call);
}

Function* FunctionWrapper::getLLVMFunction() {
    return llvm_function;
}

set<CommonCall *>& FunctionWrapper::getCommonCalls() {
    return commonCalls;
}

void FunctionWrapper::addCommonCall(CommonCall * call) {
    commonCalls.insert(call);
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
    if (lpads.count(invoke)) {
        return lpads[invoke];
    }
    return NULL;
}

set<CommonCall *>* FunctionWrapper::getCommonCallsForCG() {
    return &commonCallsForCG;
}

set<PointerCall *>* FunctionWrapper::getPointerCallsForCG() {
    return &pointerCallsForCG;
}
