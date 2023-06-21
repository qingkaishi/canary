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

#include <cassert>
#include <cstdio>
#include "DyckAA/DyckGraph.h"

void DyckGraph::printAsDot(const char *filename) const {
    FILE *f = fopen(filename, "w+");

    fprintf(f, "digraph ptg {\n");

    auto vit = vertices.begin();
    while (vit != vertices.end()) {
        if ((*vit)->getName() != nullptr)
            fprintf(f, "\ta%d[label=\"%s\"];\n", (*vit)->getIndex(), (*vit)->getName());
        else
            fprintf(f, "\ta%d;\n", (*vit)->getIndex());

        std::map<void *, std::set<DyckVertex *>> &outs = (*vit)->getOutVertices();
        auto it = outs.begin();
        while (it != outs.end()) {
            long label = (long) (it->first);
            std::set<DyckVertex *> *tars = &it->second;

            auto tarit = tars->begin();
            while (tarit != tars->end()) {
                fprintf(f, "\ta%d->a%d [label=\"%ld\"];\n", (*vit)->getIndex(), (*tarit)->getIndex(), label);
                tarit++;
            }
            it++;
        }

        ++vit;
    }

    fprintf(f, "}\n");

    fclose(f);
}

void DyckGraph::removeFromWorkList(std::multimap<DyckVertex *, void *> &list, DyckVertex *v, void *l) {
    typedef std::multimap<DyckVertex *, void *>::iterator CIT;
    typedef std::pair<CIT, CIT> Range;
    Range range = list.equal_range(v);
    auto next = range.first;
    while (next != range.second) {
        if ((next)->second == l) {
            list.erase(next);
            return;
        }
        next++;
    }
}

bool DyckGraph::containsInWorkList(std::multimap<DyckVertex *, void *> &list, DyckVertex *v, void *l) {
    typedef std::multimap<DyckVertex *, void *>::iterator CIT;
    typedef std::pair<CIT, CIT> Range;
    Range range = list.equal_range(v);
    auto next = range.first;
    while (next != range.second) {
        if ((next)->second == l) {
            return true;
        }
        next++;
    }
    return false;
}

DyckVertex *DyckGraph::combine(DyckVertex *x, DyckVertex *y) {
    assert(vertices.count(x));
    assert(vertices.count(y));

    if (x == y) {
        return x;
    }

    if (x->degree() < y->degree()) {
        DyckVertex *temp = x;
        x = y;
        y = temp;
    }

    std::set<void *> &youtlabels = y->getOutLabels();
    auto yolit = youtlabels.begin();
    while (yolit != youtlabels.end()) {
        if (y->containsTarget(y, *yolit)) {
            if (!x->containsTarget(x, *yolit)) {
                x->addTarget(x, *yolit);
            }
            y->removeTarget(y, *yolit);

        }
        yolit++;
    }

    yolit = youtlabels.begin();
    while (yolit != youtlabels.end()) {
        std::set<DyckVertex *> *ws = &y->getOutVertices()[*yolit];
        auto w = ws->begin();
        while (w != ws->end()) {
            if (!x->containsTarget(*w, *yolit)) {
                x->addTarget(*w, *yolit);
                //this->addEdge(x, *w, *yolit);

            }
            // cannot use removeTarget function, which will affect iterator
            // y remove target *w
            DyckVertex *wtemp = *w;
            ws->erase(w++);
            // *w remove src y
            ((wtemp)->getInVertices())[*yolit].erase(y);
        }
        yolit++;
    }

    std::set<void *> &yinlabels = y->getInLabels();
    auto yilit = yinlabels.begin();
    while (yilit != yinlabels.end()) {
        std::set<DyckVertex *> *ws = &y->getInVertices()[*yilit];
        auto w = ws->begin();
        while (w != ws->end()) {
            if (!(*w)->containsTarget(x, *yilit)) {
                (*w)->addTarget(x, *yilit);
                //this->addEdge(*w, x, *yilit);
            }

            // cannot use removeTarget function, which will affect iterator
            DyckVertex *wtemp = *w;
            ws->erase(w++);
            ((wtemp)->getOutVertices())[*yilit].erase(y);
        }

        yilit++;
    }
//     printf("+++++++++++++++++++++++++++++++++\n");
    auto vals = y->getEquivalentSet();
    for (auto &val: *vals) {
        val_ver_map[val] = x;
//             printf("+ %d (%p) -> %d (%p)\n", y->getIndex(), val, x->getIndex(), x);
    }
//     printf("+++++++++++++++++++++++++++++++++\n");
    y->mvEquivalentSetTo(x);
    vertices.erase(y);
//     printf("DELETE %d\n", y->getIndex());
    delete y;
    return x;
}

bool DyckGraph::qirunAlgorithm() {
    bool ret = true;

    std::multimap<DyckVertex *, void *> worklist;

    auto vit = vertices.begin();
    while (vit != vertices.end()) {
        std::set<void *> &outlabels = (*vit)->getOutLabels();
        auto lit = outlabels.begin();
        while (lit != outlabels.end()) {
            if ((*vit)->outNumVertices(*lit) > 1) {
                worklist.insert(std::pair<DyckVertex *, void *>(*vit, *lit));
            }
            lit++;
        }

        vit++;
    }

    if (!worklist.empty()) {
        ret = false;
    }

    while (!worklist.empty()) {
        //outs()<<"HERE0\n"; outs().flush();
        auto z_i_it = worklist.begin();
        //outs()<<"HERE0.1\n"; outs().flush();
        std::set<DyckVertex *> *vers = &z_i_it->first->getOutVertices()[z_i_it->second];
        auto versIt = vers->begin();
        DyckVertex *x = *(versIt);
        versIt++;
        //outs()<<"HERE0.2\n"; outs().flush();
        DyckVertex *y = *(versIt);
        if (x->degree() < y->degree()) {
            DyckVertex *temp = x;
            x = y;
            y = temp;
        }
        //outs()<<"HERE0.3\n"; outs().flush();
        assert(x != y);
        vertices.erase(y);
        auto vals = y->getEquivalentSet();
        for (auto &val: *vals) {
            val_ver_map[val] = x;
        }
        //outs()<<"HERE0.4\n"; outs().flush();
        y->mvEquivalentSetTo(x/*->getRepresentative()*/);
        //outs()<<"HERE1\n"; outs().flush();
        std::set<void *> &youtlabels = y->getOutLabels();
        auto yolit = youtlabels.begin();
        while (yolit != youtlabels.end()) {
            if (y->containsTarget(y, *yolit)) {
                //if (this->containsEdge(y, y, *yolit)) {
                if (!x->containsTarget(x, *yolit)) {
                    x->addTarget(x, *yolit);
                    //this->addEdge(x, x, *yolit);
                    if (x->outNumVertices(*yolit) > 1 && !containsInWorkList(worklist, x, *yolit)) {
                        worklist.insert(std::pair<DyckVertex *, void *>(x, *yolit));
                    }
                }
                y->removeTarget(y, *yolit);
                if (y->outNumVertices(*yolit) < 2) {
                    removeFromWorkList(worklist, y, *yolit);
                }
            }
            yolit++;
        }
        //outs()<<"HERE2\n"; outs().flush();
        yolit = youtlabels.begin();
        while (yolit != youtlabels.end()) {
            std::set<DyckVertex *> *ws = &y->getOutVertices()[*yolit];
            auto w = ws->begin();
            while (w != ws->end()) {
                if (!x->containsTarget(*w, *yolit)) {
                    x->addTarget(*w, *yolit);
                    //this->addEdge(x, *w, *yolit);
                    if (x->outNumVertices(*yolit) > 1 && !containsInWorkList(worklist, x, *yolit)) {
                        worklist.insert(std::pair<DyckVertex *, void *>(x, *yolit));
                    }
                }
                // cannot use removeTarget function, which will affect iterator
                // y remove target *w
                DyckVertex *wtemp = *w;
                ws->erase(w++);
                // *w remove src y
                ((wtemp)->getInVertices())[*yolit].erase(y);
                if (y->outNumVertices(*yolit) < 2) {
                    removeFromWorkList(worklist, y, *yolit);
                }

                //w++;
            }
            yolit++;
        }
        //outs()<<"HERE3\n"; outs().flush();
        std::set<void *> &yinlabels = y->getInLabels();
        auto yilit = yinlabels.begin();
        while (yilit != yinlabels.end()) {
            std::set<DyckVertex *> *ws = &y->getInVertices()[*yilit];
            auto w = ws->begin();
            while (w != ws->end()) {
                if (!(*w)->containsTarget(x, *yilit)) {
                    (*w)->addTarget(x, *yilit);
                    //this->addEdge(*w, x, *yilit);
                }

                // cannot use removeTarget function, which will affect iterator
                DyckVertex *wtemp = *w;
                ws->erase(w++);
                ((wtemp)->getOutVertices())[*yilit].erase(y);
                if ((wtemp)->outNumVertices(*yilit) < 2) {
                    removeFromWorkList(worklist, wtemp, *yilit);
                }

                //w++;
            }

            yilit++;
        }

        delete y;
    }

    return ret;
}

std::pair<DyckVertex *, bool> DyckGraph::retrieveDyckVertex(void *value, const char *name) {
    if (value == nullptr) {
        auto *ver = new DyckVertex(nullptr);
        vertices.insert(ver);
        return std::make_pair(ver, false);
    }

    auto it = val_ver_map.find(value);
    if (it != val_ver_map.end()) {
        return std::make_pair(it->second, true);
    } else {
        auto *ver = new DyckVertex(value, name);
        vertices.insert(ver);
        val_ver_map.insert(std::pair<void *, DyckVertex *>(value, ver));
        return std::make_pair(ver, false);
    }
}

DyckVertex *DyckGraph::findDyckVertex(void *value) {
    auto it = val_ver_map.find(value);
    if (it != val_ver_map.end()) {
        return it->second;
    }
    return nullptr;
}

unsigned int DyckGraph::numVertices() {
    return vertices.size();
}

unsigned int DyckGraph::numEquivalentClasses() {
    return vertices.size();
}

std::set<DyckVertex *> &DyckGraph::getVertices() {
    return vertices;
}

void DyckGraph::validation(const char *file, int line) {
    printf("Start validation... ");
    std::set<DyckVertex *> &reps = this->getVertices();
    auto repsIt = reps.begin();
    while (repsIt != reps.end()) {
        DyckVertex *rep = *repsIt;

        auto repVal = rep->getEquivalentSet();
        for (auto val: *repVal) {
            assert(val_ver_map[val] == rep);
        }

        repsIt++;
    }
    printf("Done!\n\n");
}
