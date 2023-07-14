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

#ifndef DYCKAA_DYCKCALLGRAPHNODE_H
#define DYCKAA_DYCKCALLGRAPHNODE_H

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Pass.h>
#include <llvm/Analysis/CaptureTracking.h>
#include <llvm/Analysis/MemoryBuiltins.h>
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/Support/raw_ostream.h>

#include <set>
#include <map>

using namespace llvm;

/// The class does not model inline asm and intrinsics
class Call {
public:
    // if it has a return value, this is the return value; 
    // it may be null, because there exists some implicit calls, like those in pthread_create
    // it may be a callinst or invoke inst, currently only call inst because all invokes are lowered to call
    Instruction *Inst;

    Value *CalledValue;
    std::vector<Value *> Args;

    Call(Instruction *Inst, Value *CalledValue, std::vector<Value *> *Args);
};

class CommonCall : public Call {
public:
    CommonCall(Instruction *Inst, Function *Func, std::vector<Value *> *Args);
};

class PointerCall : public Call {
public:
    std::set<Function *> MayAliasedCallees;
    bool MustAliasedPointerCall;

    PointerCall(Instruction *Inst, Value *CalledValue, std::vector<Value *> *Args);
};

class DyckCallGraphNode {
private:
    int Idx;

    Function *Func;
    std::set<Value *> Rets;

    std::vector<Value *> Args;
    std::vector<Value *> VAArgs;

    std::set<Value *> Resumes;
    std::map<Value *, Value *> LPads; // invoke <-> lpad

    // call instructions in the function
    std::set<CommonCall *> CommonCalls; // common calls
    std::set<PointerCall *> PointerCalls; // pointer calls

    std::map<Instruction *, Call *> InstructionCallMap;

    std::set<CallInst *> InlineAsmSet; // inline asm must be a call inst

private:
    static int GlobalIdx;

public:
    explicit DyckCallGraphNode(Function *);

    ~DyckCallGraphNode();

    int getIndex() const;

    Function *getLLVMFunction();

    std::set<CommonCall *> &getCommonCalls();

    void addCommonCall(CommonCall *);

    std::set<PointerCall *> &getPointerCalls();

    void addPointerCall(PointerCall *);

    void addResume(Value *Res);

    void addLandingPad(Value *, Value *);

    void addRet(Value *);

    void addArg(Value *);

    void addVAArg(Value *);

    void addInlineAsm(CallInst *);

    std::set<CallInst *> &getInlineAsmSet();

    std::vector<Value *> &getArgs();

    std::vector<Value *> &getVAArgs();

    std::set<Value *> &getReturns();

    std::set<Value *> &getResumes();

    Value *getLandingPad(Value *);

    Call *getCall(Instruction *);
};


#endif // DYCKAA_DYCKCALLGRAPHNODE_H

