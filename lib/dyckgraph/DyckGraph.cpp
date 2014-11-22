/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "DyckGraph.h"
#include <stdio.h>
#include <stdlib.h>
#include <tr1/hashtable.h>

void DyckGraph::printAsDot(const char* filename) const {
    FILE * f = fopen(filename, "w+");

    fprintf(f, "digraph ptg {\n");

    set<DyckVertex*>::iterator vit = reps.begin();
    while (vit != reps.end()) {
        if ((*vit)->getName() != NULL)
            fprintf(f, "\ta%d[label=\"%s\"];\n", (*vit)->getIndex(), (*vit)->getName());
        else
            fprintf(f, "\ta%d;\n", (*vit)->getIndex());

        map<void*, set<DyckVertex*>* >& outs = (*vit)->getOutVertices();
        map<void*, set<DyckVertex*>* >::iterator it = outs.begin();
        while (it != outs.end()) {
            long label = (long) (it->first);
            set<DyckVertex*>* tars = it->second;

            set<DyckVertex*>::iterator tarit = tars->begin();
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

void DyckGraph::removeFromWorkList(multimap<DyckVertex*, void*>& list, DyckVertex* v, void* l) {
    typedef multimap<DyckVertex*, void*>::iterator CIT;
    typedef pair<CIT, CIT> Range;
    Range range = list.equal_range(v);
    CIT next = range.first;
    while (next != range.second) {
        if ((next)->second == l) {
            list.erase(next);
            return;
        }
        next++;
    }
}

bool DyckGraph::containsInWorkList(multimap<DyckVertex*, void*>& list, DyckVertex* v, void* l) {
    typedef multimap<DyckVertex*, void*>::iterator CIT;
    typedef pair<CIT, CIT> Range;
    Range range = list.equal_range(v);
    CIT next = range.first;
    while (next != range.second) {
        if ((next)->second == l) {
            return true;
        }
        next++;
    }
    return false;
}

void DyckGraph::combine(DyckVertex* x, DyckVertex* y) {

    x = x->getRepresentative();
    y = y->getRepresentative();

    if (x == y) {
        return;
    }

    if (x->degree() < y->degree()) {
        DyckVertex* temp = x;
        x = y;
        y = temp;
    }

    set<void*>& youtlabels = y->getOutLabels();
    set<void*>::iterator yolit = youtlabels.begin();
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
        set<DyckVertex*>* ws = y->getOutVertices()[*yolit];
        set<DyckVertex*>::iterator w = ws->begin();
        while (w != ws->end()) {
            if (!x->containsTarget(*w, *yolit)) {
                x->addTarget(*w, *yolit);
                //this->addEdge(x, *w, *yolit);

            }
            // cannot use removeTarget function, which will affect iterator
            // y remove target *w
            DyckVertex* wtemp = *w;
            ws->erase(w++);
            // *w remove src y
            ((wtemp)->getInVertices())[*yolit] ->erase(y);
        }
        yolit++;
    }

    set<void*>& yinlabels = y->getInLabels();
    set<void*>::iterator yilit = yinlabels.begin();
    while (yilit != yinlabels.end()) {
        set<DyckVertex*>* ws = y->getInVertices()[*yilit];
        set<DyckVertex*>::iterator w = ws->begin();
        while (w != ws->end()) {
            if (!(*w)->containsTarget(x, *yilit)) {
                (*w)->addTarget(x, *yilit);
                //this->addEdge(*w, x, *yilit);
            }

            // cannot use removeTarget function, which will affect iterator
            DyckVertex* wtemp = *w;
            ws->erase(w++);
            ((wtemp)->getOutVertices())[*yilit]->erase(y);
        }

        yilit++;
    }

    this->getRepresentatives().erase(y);
    y->setRepresentative(x->getRepresentative());
}

bool DyckGraph::qirunAlgorithm() {
    bool ret = true;

    multimap<DyckVertex*, void*> worklist;

    set<DyckVertex*>::iterator vit = reps.begin();
    while (vit != reps.end()) {
        set<void*>& outlabels = (*vit)->getOutLabels();
        set<void*>::iterator lit = outlabels.begin();
        while (lit != outlabels.end()) {
            if ((*vit)->outNumVertices(*lit) > 1) {
                worklist.insert(pair<DyckVertex*, void*>(*vit, *lit));
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
        multimap<DyckVertex*, void*>::iterator z_i_it = worklist.begin();
        //outs()<<"HERE0.1\n"; outs().flush();
        set<DyckVertex*>* vers = z_i_it->first->getOutVertices()[z_i_it->second];
        set<DyckVertex*>::iterator versIt = vers->begin();
        DyckVertex* x = *(versIt);
        versIt++;
        //outs()<<"HERE0.2\n"; outs().flush();
        DyckVertex* y = *(versIt);
        if (x->degree() < y->degree()) {
            DyckVertex* temp = x;
            x = y;
            y = temp;
        }
        //outs()<<"HERE0.3\n"; outs().flush();
        if (x->getRepresentative() != y->getRepresentative()) {
            reps.erase(y->getRepresentative());
        }
        //outs()<<"HERE0.4\n"; outs().flush();
        y->setRepresentative(x->getRepresentative());
        //outs()<<"HERE1\n"; outs().flush();
        set<void*>& youtlabels = y->getOutLabels();
        set<void*>::iterator yolit = youtlabels.begin();
        while (yolit != youtlabels.end()) {
            if (y->containsTarget(y, *yolit)) {
                //if (this->containsEdge(y, y, *yolit)) {
                if (!x->containsTarget(x, *yolit)) {
                    x->addTarget(x, *yolit);
                    //this->addEdge(x, x, *yolit);
                    if (x->outNumVertices(*yolit) > 1 && !containsInWorkList(worklist, x, *yolit)) {
                        worklist.insert(pair<DyckVertex*, void*>(x, *yolit));
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
            set<DyckVertex*>* ws = y->getOutVertices()[*yolit];
            set<DyckVertex*>::iterator w = ws->begin();
            while (w != ws->end()) {
                if (!x->containsTarget(*w, *yolit)) {
                    x->addTarget(*w, *yolit);
                    //this->addEdge(x, *w, *yolit);
                    if (x->outNumVertices(*yolit) > 1 && !containsInWorkList(worklist, x, *yolit)) {
                        worklist.insert(pair<DyckVertex*, void*>(x, *yolit));
                    }
                }
                // cannot use removeTarget function, which will affect iterator
                // y remove target *w
                DyckVertex* wtemp = *w;
                ws->erase(w++);
                // *w remove src y
                ((wtemp)->getInVertices())[*yolit] ->erase(y);
                if (y->outNumVertices(*yolit) < 2) {
                    removeFromWorkList(worklist, y, *yolit);
                }

                //w++;
            }
            yolit++;
        }
        //outs()<<"HERE3\n"; outs().flush();
        set<void*>& yinlabels = y->getInLabels();
        set<void*>::iterator yilit = yinlabels.begin();
        while (yilit != yinlabels.end()) {
            set<DyckVertex*>* ws = y->getInVertices()[*yilit];
            set<DyckVertex*>::iterator w = ws->begin();
            while (w != ws->end()) {
                if (!(*w)->containsTarget(x, *yilit)) {
                    (*w)->addTarget(x, *yilit);
                    //this->addEdge(*w, x, *yilit);
                }

                // cannot use removeTarget function, which will affect iterator
                DyckVertex* wtemp = *w;
                ws->erase(w++);
                ((wtemp)->getOutVertices())[*yilit]->erase(y);
                if ((wtemp)->outNumVertices(*yilit) < 2) {
                    removeFromWorkList(worklist, wtemp, *yilit);
                }

                //w++;
            }

            yilit++;
        }
    }

    return ret;
}

pair<DyckVertex*, bool> DyckGraph::retrieveDyckVertex(void* value, const char* name) {
    if (value == NULL) {
        DyckVertex* ver = new DyckVertex(NULL);
        this->addVertex(ver);
        return std::make_pair(ver, false);
    }

    if (val_ver_map.count(value)) {
        return std::make_pair(val_ver_map[value], true);
    } else {
        DyckVertex* ver = new DyckVertex(value, name);
        this->addVertex(ver);
        val_ver_map.insert(pair<void *, DyckVertex*>(value, ver));
        return std::make_pair(ver, false);
    }
}

unsigned int DyckGraph::numVertices() {
    return vertices.size();
}

unsigned int DyckGraph::numEquivalentClasses() {
    return reps.size();
}

bool DyckGraph::addVertex(DyckVertex* ver) {
    if (vertices.insert(ver).second) {
        DyckVertex * rep = ver->getRepresentative();
        if (!reps.count(rep))
            reps.insert(rep);
        return true;
    }
    return false;
}

set<DyckVertex*>& DyckGraph::getVertices() {
    return vertices;
}

set<DyckVertex*>& DyckGraph::getRepresentatives() {
    return reps;
}
