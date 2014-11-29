/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "DyckCallGraphNode.h"

Call::Call(Value* ret, Value * cv, vector<Value*>* args) {
    this->calledValue = cv;
    this->instruction = ret;
    vector<Value*>::iterator aIt = args->begin();
    while (aIt != args->end()) {
        this->args.push_back(*aIt);
        aIt++;
    }
}

CommonCall::CommonCall(Value* ret, Function * f, vector<Value*>* args) : Call(ret, f, args) {
}

PointerCall::PointerCall(Value* ret, Value* cv, set<Function *>* fs, vector<Value*>* args) : Call(ret, cv, args) {
    this->calleeCands.insert(fs->begin(), fs->end());
}

int DyckCallGraphNode::global_idx = 0;

DyckCallGraphNode::DyckCallGraphNode(Function *f) {
    llvm_function = f;
    idx = global_idx++;

    iplist<Argument>& alt = f->getArgumentList();
    iplist<Argument>::iterator it = alt.begin();
    while (it != alt.end()) {
        args.push_back(it);

        it++;
    }
}

DyckCallGraphNode::~DyckCallGraphNode() {
    set<PointerCall *>::iterator it = pointerCalls.begin();
    while (it != pointerCalls.end()) {
        delete (*it);
        it++;
    }

    set<CommonCall *>::iterator cit = commonCalls.begin();
    while (cit != commonCalls.end()) {
        delete *cit;
        cit++;
    }

}

int DyckCallGraphNode::getIndex() {
    return idx;
}

set<PointerCall *>& DyckCallGraphNode::getPointerCalls() {
    return pointerCalls;
}

void DyckCallGraphNode::addPointerCall(PointerCall* call) {
    pointerCalls.insert(call);
}

Function* DyckCallGraphNode::getLLVMFunction() {
    return llvm_function;
}

set<CommonCall *>& DyckCallGraphNode::getCommonCalls() {
    return commonCalls;
}

void DyckCallGraphNode::addCommonCall(CommonCall * call) {
    commonCalls.insert(call);
}

void DyckCallGraphNode::addResume(Value * res) {
    resumes.insert(res);
}

void DyckCallGraphNode::addLandingPad(Value * invoke, Value * lpad) {
    lpads.insert(pair<Value*, Value*>(invoke, lpad));
}

void DyckCallGraphNode::addRet(Value * ret) {
    rets.insert(ret);
}

void DyckCallGraphNode::addArg(Value * arg) {
    args.push_back(arg);
}

void DyckCallGraphNode::addVAArg(Value* arg) {
    va_args.push_back(arg);
}

vector<Value*>& DyckCallGraphNode::getArgs() {
    return args;
}

vector<Value*>& DyckCallGraphNode::getVAArgs() {
    return va_args;
}

set<Value*>& DyckCallGraphNode::getReturns() {
    return rets;
}

set<Value*>& DyckCallGraphNode::getResumes() {
    return resumes;
}

Value* DyckCallGraphNode::getLandingPad(Value * invoke) {
    if (lpads.count(invoke)) {
        return lpads[invoke];
    }
    return NULL;
}

