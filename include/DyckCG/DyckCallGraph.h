/*
 * This class is used for inter-procedure analysis
 * 
 * Created on November 9, 2013, 12:35 PM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef DYCKCALLGRAPH_H
#define	DYCKCALLGRAPH_H
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

#include "DyckCallGraphNode.h"

#include <set>
#include <map>
#include <vector>
#include <stdio.h>


using namespace llvm;
using namespace std;

class DyckCallGraph {
private:
    typedef std::map<Function *, DyckCallGraphNode *> FunctionMapTy;
    FunctionMapTy FunctionMap;

public:

    ~DyckCallGraph() {
        FunctionMapTy::iterator it = FunctionMap.begin();
        while (it != FunctionMap.end()) {
            delete (it->second);
            it++;
        }
        FunctionMap.clear();
    }

public:
    
   FunctionMapTy::iterator begin(){
        return FunctionMap.begin();
    }
    
    FunctionMapTy::iterator end(){
        return FunctionMap.end();
    }

    size_t size() const {
        return FunctionMap.size();
    }

    DyckCallGraphNode * getOrInsertFunction(Function * f) {
        DyckCallGraphNode * parent = NULL;
        if (!FunctionMap.count(f)) {
            parent = new DyckCallGraphNode(f);
            FunctionMap.insert(pair<Function*, DyckCallGraphNode *>(f, parent));
        } else {
            parent = FunctionMap[f];
        }
        return parent;
    }
    
    void dotCallGraph(const string& mIdentifier);
    void printFunctionPointersInformation(const string& mIdentifier);
    
};


#endif	/* DYCKCALLGRAPH_H */

