//===----------------------------------------------------------------------===//
// LLVM 'embec' UTILITY : Checks codes for safety as per the EmbeC language
// rules. Targetted at embedded systems.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/DebugInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/LinkAllIR.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/PassManager.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PassNameParser.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <iostream>
#include <fstream>
#include <memory>

#include "DyckAliasAnalysis.h"

using namespace llvm;
using namespace std;

static cl::opt<string>
InputFilename(cl::Positional, cl::desc("<input bytecode>"), cl::init("-"));

static cl::opt<string>
OutputFilename("o", cl::desc("Override output filename"),
               cl::value_desc("filename"));

int main(int argc, char **argv) {

  // Initialize passes
  PassRegistry &Registry = *PassRegistry::getPassRegistry();
  initializeAnalysis(Registry);
  /*initializeCore(Registry);
  initializeDebugIRPass(Registry);
  initializeScalarOpts(Registry);
  initializeObjCARCOpts(Registry);
  initializeVectorization(Registry);
  initializeIPO(Registry);
  initializeIPA(Registry);
  initializeTransformUtils(Registry);
  initializeInstCombine(Registry);
  initializeInstrumentation(Registry);
  initializeTarget(Registry);*/

  cl::ParseCommandLineOptions(argc, argv,
			      " leap .bc -> .bc record/replay transformer\n");

  // Load the input module...
  LLVMContext &Context = getGlobalContext();
  SMDiagnostic Err;
  OwningPtr<Module> M;
  M.reset(ParseIRFile(InputFilename, Err, Context));

  if (M.get() == 0) {
    cerr << "bytecode didn't read correctly.\n";
    return 1;
  }

  // Figure out what stream we are supposed to write to...
  std::ostream *Out = &std::cout;  // Default to printing to stdout...


  if (OutputFilename != "") {
    Out = new std::ofstream(OutputFilename.c_str());
    

    if (!Out->good()) {
      cerr << "Error opening " << OutputFilename << "!\n";
      return 1;
    }
    
  } else {
    cerr<< "Warning using default output file name \"a.t.bc\"!\n";
    Out = new std::ofstream("a.leap.bc");
    if (!Out->good()) {
      cerr << "Error opening " << "a.leap.bc" << "!\n";
      return 1;
    }
  }
    
    
  //
  PassManager Passes;

  // Add an appropriate TargetLibraryInfo pass for the module's triple.
  TargetLibraryInfo *TLI = new TargetLibraryInfo(Triple(M->getTargetTriple()));
  Passes.add(TLI);

  // Add an appropriate DataLayout instance for this module.
  DataLayout *TD = 0;
  const std::string &ModuleDataLayout = M.get()->getDataLayout();
  if (!ModuleDataLayout.empty())
    TD = new DataLayout(ModuleDataLayout);
  
  if (TD)
    Passes.add(TD);
  else {
    cerr << "Error no data layout!\n";
    return 1;
  }
  
  //Add passes
  Passes.add(createDyckAliasAnalysisPass(true, false, false, false, false));
 
  // Now that we have all of the passes ready, run them.
  Passes.run(*M.get());
  (*Out) << M.get();

  return 0;
}
