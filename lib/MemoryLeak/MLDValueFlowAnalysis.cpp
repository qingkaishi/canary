

#include "MemoryLeak/MLDValueFlowAnalysis.h"
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/DyckCallGraph.h"
#include "DyckAA/DyckModRefAnalysis.h"
#include "DyckAA/DyckValueFlowAnalysis.h"
#include "MemoryLeak/MLDVFG.h"
#include "MemoryLeak/VFGReachable.h"
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
    DyckAA->getDyckCallGraph()->constructCallSiteMap();
    VFG = new MLDVFG(DyckVFA->getDyckVFGraph(), DyckAA->getDyckCallGraph());
    VFG->printVFG();
    VFGReachable VFGSolver(M, VFG, DyckAA);
    VFGSolver.solveAllSlices();
    return false;
    ;
}