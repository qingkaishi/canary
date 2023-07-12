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

#include <llvm/IR/Instructions.h>
#include <set>
#include <string>
#include "Support/API.h"

static std::set<std::string> MallocFuncs = {
        "malloc",
        "calloc",
        "memalign",
        "aligned_alloc",
        "pvalloc",
        "valloc",
        "strdup",
        "strndup",
        "kmalloc",
        "mmap",
        "mmap64",
        "get_current_dir_name",
        "_Znwj",     // operator new(unsigned int)
        "_Znwm",     // operator new(unsigned long)
        "_Znaj",     // operator new[](unsigned int)
        "_Znam",     // operator new[](unsigned long)
        "_ZnwjRKSt9nothrow_t",     // operator new(unsigned int)
        "_ZnwmRKSt9nothrow_t",     // operator new(unsigned long)
        "_ZnajRKSt9nothrow_t",     // operator new[](unsigned int)
        "_ZnamRKSt9nothrow_t",     // operator new[](unsigned long)
        "realloc",
        "reallocf",
        "getline",
        "getwline",
        "getdelim",
        "getwdelim",
};


bool API::isMemoryAllocate(Instruction *I) {
    return isHeapAllocate(I) || isStackAllocate(I);
}

bool API::isHeapAllocate(Instruction *I) {
    if (auto CI = dyn_cast<CallInst>(I)) {
        if (auto Callee = CI->getCalledFunction()) {
            return MallocFuncs.count(Callee->getName().str());
        }
    }
    return false;
}

bool API::isStackAllocate(Instruction *I) {
    return isa<AllocaInst>(I);
}