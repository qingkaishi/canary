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

#ifndef DYCKAA_DYCKVERTEX_H
#define DYCKAA_DYCKVERTEX_H

#include <map>
#include <set>

class DyckGraph;

class DyckVertex {
private:
    static int global_indx;
    int index;
    const char *name;

    std::set<void *> in_lables;
    std::set<void *> out_lables;

    std::map<void *, std::set<DyckVertex *>> in_vers;
    std::map<void *, std::set<DyckVertex *>> out_vers;

    /// only store non-null value
    std::set<void *> equivclass;

    /// The constructor is not visible. The first argument is the pointer of the value that you want to encapsulate.
    /// The second argument is the name of the vertex, which will be used in void DyckGraph::printAsDot() function.
    /// please use DyckGraph::retrieveDyckVertex for initialization.
    explicit DyckVertex(void *v, const char *itsname = nullptr);

public:
    friend class DyckGraph;

public:
    ~DyckVertex();

    /// Get its index
    /// The index of the first vertex you create is 0, the second one is 1, ...
    int getIndex() const;

    /// Get its name
    const char *getName();

    /// Get the source vertices corresponding the label
    std::set<DyckVertex *> *getInVertices(void *label);

    /// Get the target vertices corresponding the label
    std::set<DyckVertex *> *getOutVertices(void *label);

    /// Get the number of vertices that are the targets of this vertex, and have the edge label: label.
    unsigned int outNumVertices(void *label);

    /// Get the number of vertices that are the sources of this vertex, and have the edge label: label.
    unsigned int inNumVertices(void *label);

    /// Total degree of the vertex
    unsigned int degree();

    /// Get all the labels in the edges that point to the vertex's targets.
    std::set<void *> &getOutLabels();

    /// Get all the labels in the edges that point to the vertex.
    std::set<void *> &getInLabels();

    /// Get all the vertex's targets.
    /// The return value is a map which maps labels to a set of vertices.
    std::map<void *, std::set<DyckVertex *>> &getOutVertices();

    /// Get all the vertex's sources.
    /// The return value is a map which maps labels to a set of vertices.
    std::map<void *, std::set<DyckVertex *>> &getInVertices();

    /// Add a target with a label. Meanwhile, this vertex will be a source of ver.
    void addTarget(DyckVertex *ver, void *label);

    /// Remove a target. Meanwhile, this vertex will be removed from ver's sources
    void removeTarget(DyckVertex *ver, void *label);

    /// Return true if the vertex contains a target ver, and the edge label is "label"
    bool containsTarget(DyckVertex *ver, void *label);

    /// For qirun's algorithm DyckGraph::qirunAlgorithm().
    /// The representatives of all the vertices in the equivalent set of this vertex
    /// will be set to be rep.
    void mvEquivalentSetTo(DyckVertex *rep);

    /// Get the equivalent set of non-null value.
    /// Use it after you call DyckGraph::qirunAlgorithm().
    std::set<void *> *getEquivalentSet();

private:
    void addSource(DyckVertex *ver, void *label);

    void removeSource(DyckVertex *ver, void *label);
};

#endif // DYCKAA_DYCKVERTEX_H

