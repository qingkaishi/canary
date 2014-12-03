/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "DyckCallGraphNode.h"

Call::Call(Instruction* inst, Value * calledValue, vector<Value*>* args) {
    assert(calledValue!=NULL && "Error when create a call: called value is null!");
    assert(args!=NULL && "Error when create a call: args is null!");
    
    this->calledValue = calledValue;
    this->instruction = inst;
    vector<Value*>::iterator aIt = args->begin();
    while (aIt != args->end()) {
        this->args.push_back(*aIt);
        aIt++;
    }
}

CommonCall::CommonCall(Instruction* inst, Function * func, vector<Value*>* args) : Call(inst, func, args) {
}

PointerCall::PointerCall(Instruction* inst, Value* calledValue, vector<Value*>* args) : Call(inst, calledValue, args), mustAliasedPointerCall(false) {
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
    instructionCallMap.insert(pair<Instruction*, Call*>(call->instruction, call));
    pointerCalls.insert(call);
}

Function* DyckCallGraphNode::getLLVMFunction() {
    return llvm_function;
}

set<CommonCall *>& DyckCallGraphNode::getCommonCalls() {
    return commonCalls;
}

void DyckCallGraphNode::addCommonCall(CommonCall * call) {
    instructionCallMap.insert(pair<Instruction*, Call*>(call->instruction, call));
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

void DyckCallGraphNode::addInlineAsm(CallInst* inlineAsm){
    this->inlineAsms.insert(inlineAsm);
}
    
set<CallInst*>& DyckCallGraphNode::getInlineAsms(){
    return this->inlineAsms;
}

Call* DyckCallGraphNode::getCall(Instruction* inst) {
    if(this->instructionCallMap.count(inst)) {
        return instructionCallMap[inst];
    }
    return NULL;
}
