/*
 *  Popeye lifts protocol source code in C to its specification in BNF
 *  Copyright (C) 2023 Qingkai Shi <qingkaishi@gmail.com>
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

#include "Support/RecursiveTimer.h"

static unsigned DepthOfTimeRecorder = 0;

static inline std::string Tab(unsigned N) {
    std::string Ret;
    while (N-- > 0) Ret.append("    ");
    return Ret;
}

RecursiveTimer::RecursiveTimer(const char *Prefix) : Begin(std::chrono::steady_clock::now()), Prefix(Prefix) {
    outs() << Tab(DepthOfTimeRecorder++) << Prefix << "...\n";
}

RecursiveTimer::RecursiveTimer(const std::string &Prefix) : Begin(std::chrono::steady_clock::now()), Prefix(Prefix) {
    outs() << Tab(DepthOfTimeRecorder++) << Prefix << "...\n";
}

RecursiveTimer::~RecursiveTimer() {
    std::chrono::steady_clock::time_point End = std::chrono::steady_clock::now();
    auto Milli = std::chrono::duration_cast<std::chrono::milliseconds>(End - Begin).count();
    auto Time = Milli > 1000 ? Milli / 1000 : Milli;
    auto Unit = Milli > 1000 ? "s" : "ms";
    outs() << Tab(--DepthOfTimeRecorder) << Prefix << " takes " << Time << Unit << "!\n";
}

char RecursiveTimerPass::ID = 0;