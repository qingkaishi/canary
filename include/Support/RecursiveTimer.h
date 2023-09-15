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

#ifndef SUPPORT_RECURSIVETIMER_H
#define SUPPORT_RECURSIVETIMER_H

#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Pass.h>

#include <chrono>
#include <string>

using namespace llvm;

class RecursiveTimer {
private:
    std::chrono::steady_clock::time_point Begin;
    std::string Prefix;

public:
    /// the prefix should be in a style of "Doing sth" or "Sth"
    /// @{
    explicit RecursiveTimer(const char *Prefix);

    explicit RecursiveTimer(const std::string &Prefix);
    /// @}

    /// end of the recorder
    ~RecursiveTimer();
};

class RecursiveTimerPass : public ModulePass {
private:
    RecursiveTimer *&T;
    const char *Msg;

public:
    static char ID;

    explicit RecursiveTimerPass(RecursiveTimer *&T) : ModulePass(ID), T(T), Msg(nullptr) {
    }

    RecursiveTimerPass(RecursiveTimer *&T, const char *M) : ModulePass(ID), T(T), Msg(M) {
    }

    ~RecursiveTimerPass() override = default;

    void getAnalysisUsage(AnalysisUsage &AU) const override { AU.setPreservesAll(); }

    bool runOnModule(Module &) override {
        if (!T) {
            T = new RecursiveTimer(Msg);
        } else {
            delete T;
            T = nullptr;
        }
        return false;
    }
};

#endif //SUPPORT_RECURSIVETIMER_H