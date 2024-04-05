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

#include "Support/API.h"
#include "Support/RNameDemangle.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Casting.h"
#include <linux/limits.h>
#include <llvm/IR/Instructions.h>
#include <map>
#include <set>
#include <string>
#include <utility>

std::set<std::string> API::HeapAllocFunctions = {
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
    "_Znwj",               // operator new(unsigned int)
    "_Znwm",               // operator new(unsigned long)
    "_Znaj",               // operator new[](unsigned int)
    "_Znam",               // operator new[](unsigned long)
    "_ZnwjRKSt9nothrow_t", // operator new(unsigned int)
    "_ZnwmRKSt9nothrow_t", // operator new(unsigned long)
    "_ZnajRKSt9nothrow_t", // operator new[](unsigned int)
    "_ZnamRKSt9nothrow_t", // operator new[](unsigned long)
    "realloc",
    "reallocf",
    "getline",
    "getwline",
    "getdelim",
    "getwdelim",
};

std::map<std::string, int> API::RustSinkFunctionsForC = {
    {"free", 0},
    {"alloc::rc::Rc<T>::from_raw", 0},
    {"alloc::rc::Weak<T>::from_raw", 0},
    {"alloc::vec::Vec<T>::from_raw_parts", 1},
    {"alloc::sync::Arc<T>::from_raw", 0},
    {"alloc::boxed::Box<T>::from_raw", 0},
    {"alloc::string::String::from_raw_parts", 1},
    {"std::ffi::c_str::CString::from_raw", 0},
    {"std::ffi::c_str::CStr::from_ptr", 0},
};

bool API::isMemoryAllocate(Instruction *I) {
    return isHeapAllocate(I) || isStackAllocate(I);
}

bool API::isHeapAllocate(Instruction *I) {
    if (auto CI = dyn_cast<CallInst>(I)) {
        if (auto Callee = CI->getCalledFunction()) {
            return HeapAllocFunctions.count(Callee->getName().str());
        }
    }
    return false;
}

bool API::isStackAllocate(Instruction *I) {
    return isa<AllocaInst>(I);
}

bool API::isRustSinkForC(Argument *Arg) {
    Function *Func = Arg->getParent();
    std::string FuncName = RNameDemangle::legacyDemangle(Func->getName().str());
    auto SinkPoint = RustSinkFunctionsForC.find(FuncName);
    if (SinkPoint == RustSinkFunctionsForC.end()) {
        return false;
    }
    return Func->getArg(SinkPoint->second) == Arg;
}