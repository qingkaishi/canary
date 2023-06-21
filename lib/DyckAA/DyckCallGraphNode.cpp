/*
 *  Canary features a fast unification-based alias analysis for C programs
 *  Copyright (C) 2021 Qingkai Shi <qingkaishi@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "DyckAA/DyckCallGraphNode.h"

Call::Call(Instruction *inst, Value *calledValue, std::vector<Value *> *args) {
    assert(calledValue != nullptr && "Error when create a call: called value is null!");
    assert(args != nullptr && "Error when create a call: args is null!");

    this->calledValue = calledValue;
    this->instruction = inst;
    auto aIt = args->begin();
    while (aIt != args->end()) {
        this->args.push_back(*aIt);
        aIt++;
    }
}

CommonCall::CommonCall(Instruction *inst, Function *func, std::vector<Value *> *args) : Call(inst, func, args) {
}

PointerCall::PointerCall(Instruction *inst, Value *calledValue, std::vector<Value *> *args) : Call(inst, calledValue, args),
                                                                                         mustAliasedPointerCall(false) {
}

int DyckCallGraphNode::global_idx = 0;

DyckCallGraphNode::DyckCallGraphNode(Function *f) {
    llvm_function = f;
    idx = global_idx++;
    for (unsigned i = 0; i < f->arg_size(); ++i) {
        args.push_back(f->getArg(i));
    }
}

DyckCallGraphNode::~DyckCallGraphNode() {
    auto it = pointerCalls.begin();
    while (it != pointerCalls.end()) {
        delete (*it);
        it++;
    }

    auto cit = commonCalls.begin();
    while (cit != commonCalls.end()) {
        delete *cit;
        cit++;
    }

}

int DyckCallGraphNode::getIndex() const {
    return idx;
}

std::set<PointerCall *> &DyckCallGraphNode::getPointerCalls() {
    return pointerCalls;
}

void DyckCallGraphNode::addPointerCall(PointerCall *call) {
    instructionCallMap.insert(std::pair<Instruction *, Call *>(call->instruction, call));
    pointerCalls.insert(call);
}

Function *DyckCallGraphNode::getLLVMFunction() {
    return llvm_function;
}

std::set<CommonCall *> &DyckCallGraphNode::getCommonCalls() {
    return commonCalls;
}

void DyckCallGraphNode::addCommonCall(CommonCall *call) {
    instructionCallMap.insert(std::pair<Instruction *, Call *>(call->instruction, call));
    commonCalls.insert(call);
}

void DyckCallGraphNode::addResume(Value *res) {
    resumes.insert(res);
}

void DyckCallGraphNode::addLandingPad(Value *invoke, Value *lpad) {
    lpads.insert(std::pair<Value *, Value *>(invoke, lpad));
}

void DyckCallGraphNode::addRet(Value *ret) {
    rets.insert(ret);
}

void DyckCallGraphNode::addArg(Value *arg) {
    args.push_back(arg);
}

void DyckCallGraphNode::addVAArg(Value *arg) {
    va_args.push_back(arg);
}

std::vector<Value *> &DyckCallGraphNode::getArgs() {
    return args;
}

std::vector<Value *> &DyckCallGraphNode::getVAArgs() {
    return va_args;
}

std::set<Value *> &DyckCallGraphNode::getReturns() {
    return rets;
}

std::set<Value *> &DyckCallGraphNode::getResumes() {
    return resumes;
}

Value *DyckCallGraphNode::getLandingPad(Value *invoke) {
    if (lpads.count(invoke)) {
        return lpads[invoke];
    }
    return nullptr;
}

void DyckCallGraphNode::addInlineAsm(CallInst *inlineAsm) {
    this->inlineAsms.insert(inlineAsm);
}

std::set<CallInst *> &DyckCallGraphNode::getInlineAsms() {
    return this->inlineAsms;
}

Call *DyckCallGraphNode::getCall(Instruction *inst) {
    if (this->instructionCallMap.count(inst)) {
        return instructionCallMap[inst];
    }
    return nullptr;
}
