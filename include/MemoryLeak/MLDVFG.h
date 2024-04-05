#ifndef MEMORYLEAK_MLDVFG_H
#define MEMORYLEAK_MLDVFG_H
#include "DyckAA/DyckCallGraph.h"
#include "DyckAA/DyckGraphNode.h"
#include "DyckAA/DyckVFG.h"
#include <iterator>
#include <llvm/IR/Value.h>
#include <map>
#include <set>

class Slice {

friend class MLDVFG;
    DyckVFGNode *Source;
    std::set<DyckVFGNode *> ReachableSet;
    std::set<DyckVFGNode *> Sinks;

  public:
    Slice(DyckVFGNode *Source)
        : Source(Source){};
    ~Slice(){};

    void addReachable(DyckVFGNode *Node) {
        ReachableSet.insert(Node);
    }
    void addSink(DyckVFGNode *Node) {
        Sinks.insert(Node);
    }
    bool contains(DyckVFGNode *Node){
        return ReachableSet.find(Node) != ReachableSet.end();
    }
    bool sinkSetEmpty(){
        return Sinks.empty();
    }
    std::set<DyckVFGNode *>::const_iterator sinksBegin(){
        return Sinks.begin();
    }
    std::set<DyckVFGNode*>::const_iterator sinksEnd(){
        return Sinks.end();
    }
};

class MLDVFG {
    DyckVFG *VFG;
    DyckCallGraph *DyckCG;
    std::map<DyckVFGNode *, Slice> LeakMap;
    std::set<DyckVFGNode *> SourcesSet;
    using Context = std::stack<int>;
    // std::unordered_map<DyckVFGNode * , std::set<DyckVFGNode *>> ValSrcReachable;
    // std::unordered_map<DyckVFGNode * , std::set<DyckVFGNode *>> ValSrcSinks;
    void ForwardReachable(DyckVFGNode *);
    void BackwardReachable(DyckVFGNode *);
    void dyckReachable(DyckVFGNode *);

  public:
    MLDVFG(DyckVFG *VFG, DyckCallGraph *DyckCG);
    ~MLDVFG();
    void printVFG() const;
};

#endif // MEMORYLEAK_MLDVFG_H