#include "MemoryLeak/MLDVFG.h"
// #include "DyckAA/DyckGraphNode.h"
#include "DyckAA/DyckGraphNode.h"
#include "DyckAA/DyckVFG.h"
#include "Support/API.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <queue>
#include <set>
#include <stack>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>
// #include <vector>

MLDVFG::MLDVFG(DyckVFG *VFG, DyckCallGraph *DyckCG)
    : VFG(VFG),
      DyckCG(DyckCG) {
    for (auto VFGNodeIt = VFG->node_begin(); VFGNodeIt != VFG->node_end(); VFGNodeIt++) {
        if (isa<Instruction>((*VFGNodeIt)->getValue()) && API::isHeapAllocate((Instruction *)(*VFGNodeIt)->getValue())) {
            SourcesSet.insert(*VFGNodeIt);
            ForwardReachable(*VFGNodeIt);

            // dyckReachable(*VFGNodeIt);
        }
    }
}

void MLDVFG::ForwardReachable(DyckVFGNode *VFGNode) {
    Slice ForwardSlice(VFGNode);
    std::stack<std::pair<DyckVFGNode *, Context>> DfsStack;
    DfsStack.push({VFGNode, Context()});
    while (!DfsStack.empty()) {
        auto CurrPair = DfsStack.top();
        DfsStack.pop();
        if (ForwardSlice.contains(CurrPair.first)) {
            continue;
        }
        /// add into reachable set
        if (isa<Argument>(CurrPair.first) && API::isRustSinkForC(dyn_cast<Argument>(CurrPair.first))) {
            ForwardSlice.addSink(CurrPair.first);
        }
        ForwardSlice.addReachable(CurrPair.first);
        for (auto Edge = CurrPair.first->begin(); Edge != CurrPair.first->end(); Edge++) {
            if (ForwardSlice.contains(Edge->first))
                continue;
            /// cfl match
            /// if label > 0, then it's a call edge, push into context stack
            if (Edge->second > 0) {
                auto NewCxt(CurrPair.second);
                NewCxt.push(Edge->second);
                DfsStack.push({Edge->first, NewCxt});
            }
            /// if label <0 , then it'a a ret edge, pop call info from stack
            else if (Edge->second < 0) {
                if (!CurrPair.second.empty() && CurrPair.second.top() != -Edge->second)
                    continue;
                auto NewCxt(CurrPair.second);
                NewCxt.pop();
                DfsStack.push({Edge->first, NewCxt});
            }
            // normal edge, just push
            else {
                auto NewCxt(CurrPair.second);
                DfsStack.push({Edge->first, NewCxt});
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
    for (auto Sink = ForwardSlice.sinksBegin(); Sink != ForwardSlice.sinksEnd(); Sink++) {
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

// void MLDVFG::printVFG() const {
//     auto LabelGenerator = [](Value *Val) {
//         std::string Label;
//         raw_string_ostream LabselBuilder(Label);
//         if (auto Inst = dyn_cast<Instruction>(Val)) {

//             LabselBuilder << *Inst;
//         }
//         else if (auto Arg = dyn_cast<Argument>(Val)) {
//             LabselBuilder << Arg->getParent()->getName() << "\n" << *Arg;
//         }
//         auto LabelIdx = Label.find('\"');
//         while (LabelIdx != std::string::npos) {
//             Label.insert(LabelIdx, "\\");
//             LabelIdx = Label.find('\"', LabelIdx + 2);
//         }
//         return Label;
//     };
//     std::set<DyckVFGNode *> AllInOne;
//     for (auto ValReach : ValSrcReachable) {
//         AllInOne.insert(ValReach.second.begin(), ValReach.second.end());
//     }
//     AllInOne.insert(ValSrcSet.begin(), ValSrcSet.end());

//     std::map<DyckVFGNode *, int> VFGNode2Idx;

//     int NodeIdx = 0;
//     std::error_code EC;
//     raw_fd_stream VFGDot("vfg.dot", EC);
//     VFGDot << "digraph rel{shape=box\n";
//     for (auto VFGNode : AllInOne) {
//         Value *Val = VFGNode->getValue();
//         std::string Color("black");
//         if (isa<Instruction>(Val) && API::isHeapAllocate((Instruction *)Val)) {
//             Color = "red";
//         }
//         else if (isa<Argument>(Val) && API::isRustSinkForC((Argument *)Val)) {
//             Color = "blue";
//         }
//         std::string Label = LabelGenerator(Val);
//         VFGDot << "a" << NodeIdx << "[color=\"" << Color << "\""
//                << "label=\"" << Label << "\"];\n";
//         VFGNode2Idx.insert({VFGNode, NodeIdx});
//         NodeIdx++;
//     }
//     for (auto VFGNode : AllInOne) {
//         for (auto OutNode = VFGNode->begin(); OutNode != VFGNode->end(); OutNode++) {
//             if (!VFGNode2Idx.count(OutNode->first)) {
//                 continue;
//             }
//             VFGDot << "a" << VFGNode2Idx[VFGNode] << "->"
//                    << "a" << VFGNode2Idx[OutNode->first];
//             VFGDot << "[label=\"" << OutNode->second << "\"]"
//                    << "\n";
//         }
//     }
//     VFGDot << "}";
//     VFGDot.close();
// }
