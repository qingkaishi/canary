/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "LibcAnnotation.h"

#include <stdio.h>
#include <algorithm>
#include <stack>

LibcAnnotation::LibcAnnotation() : ModulePass(ID) {
}

void LibcAnnotation::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<TargetLibraryInfo>();
    AU.addRequired<DataLayout>();
}

RegisterPass<LibcAnnotation> Z("libca", "Annotate library c libs for optimization!");

// Register this pass...
char LibcAnnotation::ID = 0;

bool LibcAnnotation::runOnModule(Module & M) {
    for (ilist_iterator<Function> iterF = M.getFunctionList().begin(); iterF != M.getFunctionList().end(); iterF++) {
        Function* f = iterF;
        if (f->empty() && !f->isIntrinsic()) {
            std::string name = f->getName().str();
            if (name == "ftw") {
                f->addAttribute(1, Attribute::ReadOnly);
            }

            if (name == "setlocale" || name == "setrlimit") {
                f->addAttribute(2, Attribute::ReadOnly);
            }

            if (name == "getopt") {
                f->addAttribute(3, Attribute::ReadOnly);
            }
        }
    }
    return true;
}

ModulePass *createLibcAnnotationPass() {
    return new LibcAnnotation();
}

