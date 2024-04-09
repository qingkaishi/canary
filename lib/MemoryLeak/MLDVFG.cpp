#include "MemoryLeak/MLDVFG.h"
// #include "DyckAA/DyckGraphNode.h"
#include "DyckAA/DyckVFG.h"
#include "Support/API.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <system_error>
#include <utility>

MLDVFG::MLDVFG(DyckVFG *VFG, DyckCallGraph *DyckCG)
    : VFG(VFG),
      DyckCG(DyckCG) {
    for (auto VFGNodeIt = VFG->node_begin(); VFGNodeIt != VFG->node_end(); VFGNodeIt++) {
        if (isa<Instruction>((*VFGNodeIt)->getValue()) && API::isHeapAllocate((Instruction *)(*VFGNodeIt)->getValue())) {
            SourcesSet.insert(*VFGNodeIt);
            ForwardReachable(*VFGNodeIt);
            BackwardReachable(*VFGNodeIt);
            // printSlice(LeakMap[*VFGNodeIt]);
        }
    }
}

MLDVFG::~MLDVFG() {
    delete this;
}

void MLDVFG::ForwardReachable(DyckVFGNode *VFGNode) {
    Slice ForwardSlice(VFGNode);
    std::map<DyckVFGNode *, std::set<Context>> ForwardCFLReach;
    std::queue<NodeContextTy> Worklist;
    Worklist.push({VFGNode, Context()});
    int idx = 0;
    while (!Worklist.empty()) {
        auto CurrPair = Worklist.front();
        Worklist.pop();
        /// add into reachable set
        if (isa<Argument>(CurrPair.first->getValue()) && API::isRustSinkForC(dyn_cast<Argument>(CurrPair.first->getValue()))) {
            ForwardSlice.addSink(CurrPair.first);
        }
        ForwardSlice.addReachable(CurrPair.first);
        for (auto Edge = CurrPair.first->begin(); Edge != CurrPair.first->end(); Edge++) {
            /// cfl match
            /// if label > 0, then it's a call edge, push into context stack
            auto NewCxt(CurrPair.second);
            if (Edge->second > 0) {
                if (!NewCxt.count(Edge->second))
                    NewCxt.push(Edge->second);
            }
            /// if label <0 , then it'a a ret edge, pop call info from stack
            else if (Edge->second < 0) {
                if (!NewCxt.empty()) {
                    if (NewCxt.top() == -Edge->second) {
                        NewCxt.pop();
                    }
                    else {
                        continue;
                    }
                }
            }
            if (ForwardCFLReach.count(Edge->first)) {
                auto It = ForwardCFLReach.find(Edge->first);
                if (!It->second.count(NewCxt)) {
                    ForwardCFLReach[Edge->first].emplace(NewCxt);
                    auto NodeCxt = std::make_pair(Edge->first, NewCxt);
                    Worklist.push(NodeCxt);
                }
            }
            else {
                ForwardCFLReach[Edge->first].emplace(NewCxt);
                auto NodeCxt = std::make_pair(Edge->first, NewCxt);
                Worklist.push(NodeCxt);
            }
        }
    }
    LeakMap[VFGNode] = std::move(ForwardSlice);
}

void MLDVFG::BackwardReachable(DyckVFGNode *VFGNode) {
    Slice BackwardSlice(VFGNode);
    Slice &ForwardSlice = LeakMap[VFGNode];
    if (LeakMap[VFGNode].sinkSetEmpty()) {
        return;
    }
    for (auto Sink = ForwardSlice.sinks_begin(); Sink != ForwardSlice.sinks_end(); Sink++) {
        std::queue<DyckVFGNode *> worklist;
        BackwardSlice.addSink(*Sink);
        BackwardSlice.addReachable(*Sink);
        worklist.push(*Sink);
        while (!worklist.empty()) {
            DyckVFGNode *Curr = worklist.front();
            worklist.pop();
            for (auto Edge = Curr->in_begin(); Edge != Curr->in_end(); Edge++) {
                if (BackwardSlice.contains(Edge->first) || !ForwardSlice.contains(Edge->first))
                    continue;
                BackwardSlice.addReachable(Edge->first);
                worklist.push(Edge->first);
            }
        }
    }
    ForwardSlice = std::move(BackwardSlice);
}

void MLDVFG::printVFG() const {
    std::set<DyckVFGNode *> AllInOne;
    for (auto ValReach : LeakMap) {
        AllInOne.insert(ValReach.second.reach_begin(), ValReach.second.reach_end());
    }
    printSubGraph(AllInOne, "vfg.dot");
}

void MLDVFG::printSlice(Slice &S) const {
    std::set<DyckVFGNode *> AllInOne;
    AllInOne.insert(S.reach_begin(), S.reach_end());
    printSubGraph(AllInOne, ("slice" + std::to_string((uint64_t)S.getSource()) + ".dot"));
}

void MLDVFG::printSubGraph(std::set<DyckVFGNode *> AllInOne, std::string Filename) const {
    auto LabelGenerator = [](Value *Val) {
        std::string Label;
        raw_string_ostream LabselBuilder(Label);
        if (auto Inst = dyn_cast<Instruction>(Val)) {

            LabselBuilder << *Inst;
        }
        else if (auto Arg = dyn_cast<Argument>(Val)) {
            LabselBuilder << Arg->getParent()->getName() << "\n" << *Arg;
        }
        auto LabelIdx = Label.find('\"');
        while (LabelIdx != std::string::npos) {
            Label.insert(LabelIdx, "\\");
            LabelIdx = Label.find('\"', LabelIdx + 2);
        }
        return Label;
    };

    std::map<DyckVFGNode *, int> VFGNode2Idx;

    int NodeIdx = 0;
    std::error_code EC;
    raw_fd_stream VFGDot(Filename.c_str(), EC);
    VFGDot << "digraph rel{shape=box\n";
    for (auto VFGNode : AllInOne) {
        Value *Val = VFGNode->getValue();
        std::string Color("black");
        if (isa<Instruction>(Val) && API::isHeapAllocate((Instruction *)Val)) {
            Color = "red";
        }
        else if (isa<Argument>(Val) && API::isRustSinkForC((Argument *)Val)) {
            Color = "blue";
        }
        std::string Label = LabelGenerator(Val);
        VFGDot << "a" << NodeIdx << "[color=\"" << Color << "\""
               << "label=\"" << Label << "\"];\n";
        VFGNode2Idx.insert({VFGNode, NodeIdx});
        NodeIdx++;
    }
    for (auto VFGNode : AllInOne) {
        for (auto OutNode = VFGNode->begin(); OutNode != VFGNode->end(); OutNode++) {
            if (!VFGNode2Idx.count(OutNode->first)) {
                continue;
            }
            VFGDot << "a" << VFGNode2Idx[VFGNode] << "->"
                   << "a" << VFGNode2Idx[OutNode->first];
            VFGDot << "[label=\"" << OutNode->second << "\"]"
                   << "\n";
        }
    }
    VFGDot << "}";
    VFGDot.close();
}
