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

#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/InitializePasses.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils.h>

#include <memory>

#include "MemoryLeak/MLDValueFlowAnalysis.h"
#include "NullPointer/NullCheckAnalysis.h"
#include "Support/RecursiveTimer.h"
#include "Support/Statistics.h"
#include "Transform/LowerConstantExpr.h"
#include "llvm/Analysis/PostDominators.h"

using namespace llvm;

static cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<input bitcode file>"),
                                          cl::init("-"), cl::value_desc("filename"));

static cl::opt<std::string> OutputFilename("o", cl::desc("<output bitcode file>"),
                                           cl::init(""), cl::value_desc("filename"));

static cl::opt<bool> OutputAssembly("S", cl::desc("Write output as LLVM assembly"), cl::init(false));

static cl::opt<bool> OnlyStatistics("s", cl::desc("Only output statistics"), cl::init(false));

int main(int argc, char **argv) {
    InitLLVM X(argc, argv);

    // Enable debug stream buffering.
    llvm::EnableDebugBuffering = true;

    // Call llvm_shutdown() on exit.
    llvm_shutdown_obj Y;

    // Print a stack trace if a fatal signal occurs.
    sys::PrintStackTraceOnErrorSignal(argv[0]);
    PrettyStackTraceProgram Z(argc, argv);

    // Initialize passes
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeCore(Registry);
    initializeCoroutines(Registry);
    initializeScalarOpts(Registry);
    initializeIPO(Registry);
    initializeAnalysis(Registry);
    initializeTransformUtils(Registry);
    initializeInstCombine(Registry);
    initializeAggressiveInstCombine(Registry);
    initializeTarget(Registry);

    cl::ParseCommandLineOptions(argc, argv, "Bona soundly checks if a pointer may be nullptr.\n");

    SMDiagnostic Err;
    LLVMContext Context;
    std::unique_ptr<Module> M = parseIRFile(InputFilename.getValue(), Err, Context);
    if (!M) {
        Err.print(argv[0], errs());
        return 1;
    }

    if (verifyModule(*M, &errs())) {
        errs() << argv[0] << ": error: input module is broken!\n";
        return 1;
    } else {
        Statistics::run(*M);
        if (OnlyStatistics) return 0;
    }

    legacy::PassManager Passes;

    auto *TransformTimer = new RecursiveTimerPass("Transforming the bitcode");
    Passes.add(TransformTimer->start());
    // Passes.add(createLowerAtomicPass());
    // Passes.add(createLowerInvokePass());
    // Passes.add(createPromoteMemoryToRegisterPass());
    // Passes.add(createSCCPPass());
    // Passes.add(createLoopSimplifyPass());
    Passes.add(new LowerConstantExpr());
    Passes.add(TransformTimer->done());
    if (!OutputAssembly.getValue()) {
        auto *AnalysisTimer = new RecursiveTimerPass("Analyzing the bitcode");
        Passes.add(AnalysisTimer->start());
        Passes.add(new MLDValueFlowAnalysis());
        Passes.add(AnalysisTimer->done());
    }

    std::unique_ptr<ToolOutputFile> Out;
    if (!OutputFilename.getValue().empty()) {
        std::error_code EC;
        Out = std::make_unique<ToolOutputFile>(OutputFilename, EC, sys::fs::F_None);
        if (EC) {
            errs() << EC.message() << '\n';
            return 1;
        }

        if (OutputAssembly.getValue()) {
            Passes.add(createPrintModulePass(Out->os()));
        } else {
            Passes.add(createBitcodeWriterPass(Out->os()));
        }
    }

    Passes.run(*M);

    if (Out) Out->keep();

    return 0;
}