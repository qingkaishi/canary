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

#ifndef DYCKAA_DYCKGRAPHNODE_H
#define DYCKAA_DYCKGRAPHNODE_H

#include "DyckAA/DyckGraphEdgeLabel.h"
#include <map>
#include <set>
#include <llvm/IR/Value.h>
class DyckGraph;

class DyckGraphNode {
    friend class DyckGraph;
private:
    static int GlobalNodeIndex;
    int NodeIndex;
    const char *NodeName;
    bool ContainsNull = false;
    bool AliasOfHeapAlloc = false;
    std::set<DyckGraphEdgeLabel *> InLables;
    std::set<DyckGraphEdgeLabel *> OutLables;

    std::map<DyckGraphEdgeLabel *, std::set<DyckGraphNode *>> InNodes;
    std::map<DyckGraphEdgeLabel *, std::set<DyckGraphNode *>> OutNodes;

    /// only store non-null value
    std::set<llvm::Value *> EquivClass;

    /// The constructor is not visible. The first argument is the pointer of the value that you want to encapsulate.
    /// The second argument is the name of the vertex, which will be used in void DyckGraph::printAsDot() function.
    /// please use DyckGraph::retrieveDyckVertex for initialization.
    explicit DyckGraphNode(llvm::Value *V, const char *Name = nullptr);

public:
    ~DyckGraphNode();

    /// Get its index
    /// The index of the first vertex you create is 0, the second one is 1, ...
    int getIndex() const;

    /// Get its name
    const char *getName();

    /// Get the source vertices corresponding the label
    std::set<DyckGraphNode *> *getInVertices(DyckGraphEdgeLabel *Label);

    /// Get the target vertices corresponding the label
    std::set<DyckGraphNode *> *getOutVertices(DyckGraphEdgeLabel *Label);

    /// Get a single source vertex corresponding the label
    /// if there are multiple such vertices or zero, return null
    DyckGraphNode *getInVertex(DyckGraphEdgeLabel *Label);

    /// Get a single target vertex corresponding the label
    /// if there are multiple such vertices or zero, return null
    DyckGraphNode *getOutVertex(DyckGraphEdgeLabel *Label);

    /// Get the number of vertices that are the targets of this vertex, and have the edge label: label.
    unsigned int outNumVertices(DyckGraphEdgeLabel *Label);

    /// Get the number of vertices that are the sources of this vertex, and have the edge label: label.
    unsigned int inNumVertices(DyckGraphEdgeLabel *Label);

    /// Total degree of the vertex
    unsigned int degree();

    /// Get all the labels in the edges that point to the vertex's targets.
    std::set<DyckGraphEdgeLabel *> &getOutLabels();

    /// Get all the labels in the edges that point to the vertex.
    std::set<DyckGraphEdgeLabel *> &getInLabels();

    /// Get all the vertex's targets.
    /// The return value is a map which maps labels to a set of vertices.
    std::map<DyckGraphEdgeLabel *, std::set<DyckGraphNode *>> &getOutVertices();

    /// Get all the vertex's sources.
    /// The return value is a map which maps labels to a set of vertices.
    std::map<DyckGraphEdgeLabel *, std::set<DyckGraphNode *>> &getInVertices();

    /// Add a target with a label. Meanwhile, this vertex will be a source of ver.
    void addTarget(DyckGraphNode *Node, DyckGraphEdgeLabel *Label);

    /// Remove a target. Meanwhile, this vertex will be removed from ver's sources
    void removeTarget(DyckGraphNode *Node, DyckGraphEdgeLabel *Label);

    /// Return true if the vertex contains a target ver, and the edge label is "label"
    bool containsTarget(DyckGraphNode *Tar, DyckGraphEdgeLabel *Label);

    /// For qirun's algorithm DyckGraph::qirunAlgorithm().
    /// The representatives of all the vertices in the equivalent set of this vertex
    /// will be set to be Rep.
    void mvEquivalentSetTo(DyckGraphNode *RootRep);

    /// Get the equivalent set of non-null value.
    /// Use it after you call DyckGraph::qirunAlgorithm().
    std::set<llvm::Value *> *getEquivalentSet();

    /// the equivalent set contains null pointer
    void setContainsNull() { ContainsNull = true; }

    /// return true if the equivalent set contains null pointer
    bool containsNull() const { return ContainsNull; }
    
    void setAliasOfHeapAlloc(){ AliasOfHeapAlloc = true;}
    
    bool isAliasOfHeapAlloc(){ return AliasOfHeapAlloc;}

private:
    void addSource(DyckGraphNode *, DyckGraphEdgeLabel *Label);

    void removeSource(DyckGraphNode *, DyckGraphEdgeLabel *Label);
};

#endif // DYCKAA_DYCKGRAPHNODE_H

