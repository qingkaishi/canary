#ifndef MEMORYLEAK_MLDVALUEFLOWANALYSIS_H
#define MEMORYLEAK_MLDVALUEFLOWANALYSIS_H

#include "MemoryLeak/MLDVFG.h"
#include <llvm/Pass.h>

class MLDValueFlowAnalysis : public llvm::ModulePass {
private:
    MLDVFG *VFG;
public:
    static char ID;
    MLDValueFlowAnalysis();
    ~MLDValueFlowAnalysis();
    void getAnalysisUsage(AnalysisUsage & AU) const override;
    bool runOnModule(llvm::Module &M) override;
};

#endif // MEMORYLEAK_MLDVFGANALYSIS_H