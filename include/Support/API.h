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

#ifndef SUPPORT_API_H
#define SUPPORT_API_H

#include "llvm/IR/Argument.h"
#include <llvm/IR/Instruction.h>
#include <set>
using namespace llvm;

class API {
public:
    static bool isMemoryAllocate(Instruction *);

    static bool isHeapAllocate(Instruction *);

    static bool isStackAllocate(Instruction *);

    static bool isRustSinkForC(Argument *);
    static std::set<std::string> HeapAllocFunctions;
    static std::map<std::string, int> RustSinkFunctionsForC;
};

#endif //SUPPORT_API_H
