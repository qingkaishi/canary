/*
 * This class is used for inter-procedure analysis
 * 
 * Created on November 9, 2013, 12:35 PM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef CALLGRAPH_H
#define	CALLGRAPH_H
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include "FunctionWrapper.h"

#include <set>
#include <map>
#include <vector>
#include <stdio.h>


using namespace llvm;
using namespace std;

class CallGraph {
private:
    map<Function*, FunctionWrapper *> wrapped_functions_map;
    set<FunctionWrapper*> callgraph_nodes;

public:

    ~CallGraph() {
        set<FunctionWrapper*>::iterator it = callgraph_nodes.begin();
        while (it != callgraph_nodes.end()) {
            delete (*it);
            it++;
        }
        callgraph_nodes.clear();
        wrapped_functions_map.clear();
    }

public:
    
    set<FunctionWrapper *>::iterator begin(){
        return callgraph_nodes.begin();
    }
    
    set<FunctionWrapper *>::iterator end(){
        return callgraph_nodes.end();
    }

    FunctionWrapper * getFunctionWrapper(Function * f) {
        FunctionWrapper * parent = NULL;
        if (!wrapped_functions_map.count(f)) {
            parent = new FunctionWrapper(f);
            wrapped_functions_map.insert(pair<Function*, FunctionWrapper *>(f, parent));
            callgraph_nodes.insert(parent);
        } else {
            parent = wrapped_functions_map[f];
        }
        return parent;
    }
    
    void dotCallGraph(const string& mIdentifier);
    void printFunctionPointersInformation(const string& mIdentifier);
    
};


#endif	/* CALLGRAPH_H */

