

#include "MemoryLeak/MLDValueFlowAnalysis.h"
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckCallGraph.h"
#include "DyckAA/DyckModRefAnalysis.h"
#include "DyckAA/DyckValueFlowAnalysis.h"
#include "MemoryLeak/MLDVFG.h"
#include "llvm/Pass.h"

char MLDValueFlowAnalysis::ID = 0;

MLDValueFlowAnalysis::MLDValueFlowAnalysis()
    : llvm::ModulePass(ID) {
    VFG = nullptr;
}

MLDValueFlowAnalysis::~MLDValueFlowAnalysis() {}

void MLDValueFlowAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<DyckAliasAnalysis>();
    AU.addRequired<DyckModRefAnalysis>();
    AU.addRequired<DyckValueFlowAnalysis>();
}

bool MLDValueFlowAnalysis::runOnModule(llvm::Module &M) {
    auto *DyckVFA = &getAnalysis<DyckValueFlowAnalysis>();
    auto *DyckAA = &getAnalysis<DyckAliasAnalysis>();
    VFG = new MLDVFG(DyckVFA->getDyckVFGraph(), DyckAA->getDyckCallGraph());
    VFG->printVFG();
    return false;
    ;
}