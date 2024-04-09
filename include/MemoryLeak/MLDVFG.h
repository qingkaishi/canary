#ifndef MEMORYLEAK_MLDVFG_H
#define MEMORYLEAK_MLDVFG_H
#include "DyckAA/DyckCallGraph.h"
#include "DyckAA/DyckGraphNode.h"
#include "DyckAA/DyckVFG.h"
#include <llvm/IR/Value.h>
#include <map>
#include <set>
#include <stack>
#include <utility>
#include <vector>

class Slice {

    friend class MLDVFG;
    DyckVFGNode *Source;
    std::set<DyckVFGNode *> ReachableSet;
    std::set<DyckVFGNode *> Sinks;

  public:
    Slice()
        : Source(nullptr) {}
    Slice(DyckVFGNode *Source)
        : Source(Source) {}
    // Slice(Slice &other):Source(other.Source), ReachableSet(other.ReachableSet), Sinks(other.Sinks){
    // }
    ~Slice() {}

    void addReachable(DyckVFGNode *Node) {
        ReachableSet.insert(Node);
    }
    void addSink(DyckVFGNode *Node) {
        Sinks.insert(Node);
    }
    bool contains(DyckVFGNode *Node) {
        return ReachableSet.find(Node) != ReachableSet.end();
    }
    bool contains(DyckVFGNode *Node) const {
        return ReachableSet.find(Node) != ReachableSet.end();
    }
    bool sinkSetEmpty() {
        return Sinks.empty();
    }
    DyckVFGNode *getSource() const {
        return Source;
    }
    DyckVFGNode *getSource() {
        return Source;
    }
    std::set<DyckVFGNode *>::const_iterator sinks_begin() {
        return Sinks.begin();
    }
    std::set<DyckVFGNode *>::const_iterator sinks_end() {
        return Sinks.end();
    }

    std::set<DyckVFGNode *>::const_iterator reach_begin() {
        return ReachableSet.begin();
    }
    std::set<DyckVFGNode *>::const_iterator reach_end() {
        return ReachableSet.end();
    }
};

class MLDVFG {
    DyckVFG *VFG;
    DyckCallGraph *DyckCG;
    std::map<DyckVFGNode *, Slice> LeakMap;
    std::set<DyckVFGNode *> SourcesSet;
    void ForwardReachable(DyckVFGNode *);
    void BackwardReachable(DyckVFGNode *);
    // void dyckForwardReachable(DyckVFGNode *);

    struct Context {
        std::vector<int> instance;
        Context()
            : instance() {}
        Context(const Context &other)
            : instance(other.instance) {}
        int top() const {
            return instance.back();
        }
        void push(int i) {
            instance.push_back(i);
        }
        bool empty() const {
            return instance.empty();
        }
        void pop() {
            instance.pop_back();
        }
        bool count(int i) const {
            for(auto j : instance){
                if(i == j){
                    return true;
                }
            }
            return false;
        }
        bool const operator<(const Context &other) const {
            bool less = true;
            for (int i = 0; i < other.instance.size() && i < instance.size(); i++) {
                if (less) {
                    less = less && instance[i] < other.instance[i];
                }
                else {
                    break;
                }
            }
            return less;
        }
    };
    using NodeContextTy = std::pair<DyckVFGNode *, Context>;

  public:
    MLDVFG(DyckVFG *VFG, DyckCallGraph *DyckCG);

    std::map<DyckVFGNode *, Slice>::iterator begin() {
        return LeakMap.begin();
    }
    std::map<DyckVFGNode *, Slice>::iterator end() {
        return LeakMap.end();
    }
    ~MLDVFG();

    void printVFG() const;
    void printSlice(Slice &Slice) const;
    void printSubGraph(std::set<DyckVFGNode *>, std::string) const;
};

#endif // MEMORYLEAK_MLDVFG_H