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

#include "DyckAA/DyckGraphNode.h"
#include "DyckAA/DyckGraphEdgeLabel.h"

int DyckGraphNode::GlobalNodeIndex = 0;

DyckGraphNode::DyckGraphNode(llvm::Value *V, const char *Name) {
    NodeName = Name;
    NodeIndex = GlobalNodeIndex++;
    if (V) EquivClass.insert(V);
}

DyckGraphNode::~DyckGraphNode() = default;

const char *DyckGraphNode::getName() {
    return NodeName;
}

unsigned int DyckGraphNode::outNumVertices(DyckGraphEdgeLabel *Label) {
    auto It = OutNodes.find(Label);
    if (It != OutNodes.end()) {
        return It->second.size();
    }
    return 0;
}

unsigned int DyckGraphNode::inNumVertices(DyckGraphEdgeLabel *Label) {
    auto It = InNodes.find(Label);
    if (It != InNodes.end()) {
        return It->second.size();
    }
    return 0;
}

unsigned int DyckGraphNode::degree() {
    unsigned int Ret = 0;

    auto IIt = InNodes.begin();
    while (IIt != InNodes.end()) {
        Ret = Ret + IIt->second.size();
        IIt++;
    }

    auto OIt = OutNodes.begin();
    while (OIt != OutNodes.end()) {
        Ret = Ret + OIt->second.size();
        OIt++;
    }

    return Ret;
}

std::set<llvm::Value *> *DyckGraphNode::getEquivalentSet() {
    return &this->EquivClass;
}

void DyckGraphNode::mvEquivalentSetTo(DyckGraphNode *RootRep) {
    if (RootRep == this) return;

    std::set<llvm::Value *> *RootEC = RootRep->getEquivalentSet();
    std::set<llvm::Value *> *ThisEC = this->getEquivalentSet();
    RootEC->insert(ThisEC->begin(), ThisEC->end());
}

std::set<DyckGraphEdgeLabel *> &DyckGraphNode::getOutLabels() {
    return OutLables;
}

std::map<DyckGraphEdgeLabel *, std::set<DyckGraphNode *>> &DyckGraphNode::getOutVertices() {
    return OutNodes;
}

std::set<DyckGraphEdgeLabel *> &DyckGraphNode::getInLabels() {
    return InLables;
}

std::map<DyckGraphEdgeLabel *, std::set<DyckGraphNode *>> &DyckGraphNode::getInVertices() {
    return InNodes;
}

int DyckGraphNode::getIndex() const {
    return NodeIndex;
}

void DyckGraphNode::addTarget(DyckGraphNode *Node, DyckGraphEdgeLabel *Label) {
    OutLables.insert(Label);
    OutNodes[Label].insert(Node);
    Node->addSource(this, Label);
}

void DyckGraphNode::removeTarget(DyckGraphNode *Node, DyckGraphEdgeLabel *Label) {
    auto It = OutNodes.find(Label);
    if (It != OutNodes.end())
        It->second.erase(Node);
    Node->removeSource(this, Label);
}

bool DyckGraphNode::containsTarget(DyckGraphNode *Tar, DyckGraphEdgeLabel *Label) {
    auto It = OutNodes.find(Label);
    if (It != OutNodes.end()) return It->second.count(Tar);
    return false;
}

std::set<DyckGraphNode *> *DyckGraphNode::getInVertices(DyckGraphEdgeLabel *Label) {
    auto It = InNodes.find(Label);
    if (It != InNodes.end()) return &It->second;
    return nullptr;
}

std::set<DyckGraphNode *> *DyckGraphNode::getOutVertices(DyckGraphEdgeLabel *Label) {
    auto It = OutNodes.find(Label);
    if (It != OutNodes.end()) return &It->second;
    return nullptr;
}

DyckGraphNode *DyckGraphNode::getInVertex(DyckGraphEdgeLabel *Label) {
    auto *Set = getInVertices(Label);
    if (Set && Set->size() == 1) return *Set->begin();
    return nullptr;
}

DyckGraphNode *DyckGraphNode::getOutVertex(DyckGraphEdgeLabel *Label) {
    auto *Set = getOutVertices(Label);
    if (Set && Set->size() == 1) return *Set->begin();
    return nullptr;
}

// the followings are private functions

void DyckGraphNode::addSource(DyckGraphNode *Node, DyckGraphEdgeLabel *Label) {
    InLables.insert(Label);
    InNodes[Label].insert(Node);
}

void DyckGraphNode::removeSource(DyckGraphNode *Node, DyckGraphEdgeLabel *Label) {
    auto It = InNodes.find(Label);
    if (It != InNodes.end()) It->second.erase(Node);
}
