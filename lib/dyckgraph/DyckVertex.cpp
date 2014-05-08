#include "DyckVertex.h"

int DyckVertex::global_indx = 0;

DyckVertex::DyckVertex(void * v, const char * itsname) {
    name = itsname;
    index = global_indx++;
    value = v;
    representative = this;
    equivclass = new set<DyckVertex*>;
    equivclass->insert(representative);
}

DyckVertex::~DyckVertex() {
    delete equivclass;
}

const char * DyckVertex::getName() {
    return name;
}

unsigned int DyckVertex::outNumVertices(void* label) {
    if (out_vers.count(label)) {
        return out_vers[label]->size();
    }
    return 0;
}

unsigned int DyckVertex::inNumVertices(void* label) {
    if (in_vers.count(label)) {
        return in_vers[label]->size();
    }
    return 0;
}

unsigned int DyckVertex::degree() {
    unsigned int ret = 0;

    map<void*, set<DyckVertex*>* >::iterator iit = in_vers.begin();
    while (iit != in_vers.end()) {

        ret = ret + iit->second->size();
        iit++;
    }

    map<void*, set<DyckVertex*>* >::iterator oit = out_vers.begin();
    while (oit != out_vers.end()) {

        ret = ret + oit->second->size();
        oit++;
    }

    return ret;
}

set<DyckVertex*>* DyckVertex::getEquivalentSet() {
    set<DyckVertex*>* alias = this->getRepresentative()->getEquivClass();
    if (alias == NULL) {
        fprintf(stderr, "ERROR in DyckGraph::getAliases(DyckVertex*) for following vertex.\n");
        fprintf(stderr, "%s\n", this->getName());
        exit(-1);
    }
    return alias;
}

bool DyckVertex::inSameEquivalentSet(DyckVertex* v1) {
    return this->getRepresentative() == v1->getRepresentative();
}

void DyckVertex::setRepresentative(DyckVertex* root) {
    /// @FIXME properties of this's rep should be added to root
    DyckVertex * rootRep = root->getRepresentative();
    DyckVertex * thisRep = this->getRepresentative();

    if (rootRep == thisRep) {
        return;
    }

    map<const char*, void*>& props = thisRep->getAllProperties();
    map<const char*, void*>::iterator mit = props.begin();
    while (mit != props.end()) {
        rootRep->addProperty(mit->first, mit->second);
        mit++;
    }

    set<DyckVertex*> * rootecls = rootRep->getEquivClass();
    set<DyckVertex*> * thisecls = thisRep->getEquivClass();

    rootecls->insert(thisecls->begin(), thisecls->end());
    thisRep->representative = rootRep;
    delete thisecls;
}

DyckVertex* DyckVertex::getRepresentative() {
    if (this == representative) {
        return representative;
    } else {
        DyckVertex* ret = representative->getRepresentative();
        if (this->representative != ret)
            this->representative = ret; // it will be efficient next time
        return ret;
    }
}

set<void*>& DyckVertex::getOutLabels() {
    return out_lables;
}

map<void*, set<DyckVertex*>* >& DyckVertex::getOutVertices() {
    return out_vers;
}

set<void*>& DyckVertex::getInLabels() {
    return in_lables;
}

map<void*, set<DyckVertex*>* >& DyckVertex::getInVertices() {
    return in_vers;
}

int DyckVertex::getIndex() {
    return index;
}

void DyckVertex::addTarget(DyckVertex* ver, void* label) {
    out_lables.insert(label);
    if (!out_vers.count(label)) {
        set<DyckVertex*>* tars = new set<DyckVertex*>;
        tars->insert(ver);
        out_vers.insert(pair<void*, set<DyckVertex*>* >(label, tars));
    } else {
        out_vers[label] ->insert(ver);
    }

    ver->addSource(this, label);
}

void DyckVertex::removeTarget(DyckVertex* ver, void* label) {
    if (out_vers.count(label)) {
        out_vers[label]->erase(ver);
    }

    ver->removeSource(this, label);
}

void DyckVertex::removeTarget(DyckVertex* ver) {
    set<void*>& olabels = this->getOutLabels();

    set<void*>::iterator olit = olabels.begin();
    while (olit != olabels.end()) {
        this->removeTarget(ver, *olit);
        olit++;
    }
}

void * DyckVertex::getValue() {
    return value;
}

bool DyckVertex::containsTarget(DyckVertex* tar, void* label) {
    if (out_vers.count(label)) {
        return out_vers[label]->find(tar) != out_vers[label]->end();
    }

    return false;
}

void DyckVertex::setValue(void * v) {
    this->value = v;
}

set<DyckVertex*>* DyckVertex::getInVertices(void * label) {
    if (in_vers.count(label)) {
        return this->in_vers[label];
    }
    return NULL;
}

set<DyckVertex*>* DyckVertex::getOutVertices(void * label) {
    if (out_vers.count(label)) {
        return this->out_vers[label];
    }
    return NULL;
}

void DyckVertex::setEquivalentSet(set<DyckVertex*>* eset) {
    this->equivclass = eset;
}

DyckVertex* DyckVertex::clone() {
    DyckVertex* copy = new DyckVertex(this->getValue(), this->getName());

    // copy in_labels/out_labels
    // copy in_vertices/out_vertices
    map<void*, set<DyckVertex*>*>& ins = this->getInVertices();
    map<void*, set<DyckVertex*>*>::iterator iit = ins.begin();
    while (iit != ins.end()) {
        void * label = iit->first;
        set<DyckVertex*>* ltars = iit->second;
        set<DyckVertex*>::iterator lrit = ltars->begin();
        while (lrit != ltars->end()) {
            (*lrit)->addTarget(copy, label);
            lrit++;
        }
        iit++;
    }

    map<void*, set<DyckVertex*>*>& ous = this->getOutVertices();
    map<void*, set<DyckVertex*>*>::iterator oit = ous.begin();
    while (oit != ous.end()) {
        void * label = oit->first;
        set<DyckVertex*>* ltars = oit->second;
        set<DyckVertex*>::iterator lrit = ltars->begin();
        while (lrit != ltars->end()) {
            copy->addTarget(*lrit, label);
            lrit++;
        }
        oit++;
    }

    return copy;
}

/* unsafe methods, do not use it
 * void DyckVertex::getInVerticesWithout(void * label, set<DyckVertex*>* ret) {
    set<void *>::iterator it = in_lables.begin();
    while (it != in_lables.end()) {
        if (*it != label) {
            set<DyckVertex*>* vertices = this->getInVertices(*it);
            ret->insert(vertices->begin(), vertices->end());
        }
        it++;
    }
}*/

void DyckVertex::getOutVertices(set<DyckVertex*>* ret) {
    set<void *>::iterator it = out_lables.begin();
    while (it != out_lables.end()) {
        set<DyckVertex*>* vertices = this->getOutVertices(*it);
        ret->insert(vertices->begin(), vertices->end());
        it++;
    }
}

void DyckVertex::addProperty(const char * name, void * value) {
    properties.erase(name);
    properties.insert(pair<const char*, void*>(name, value));
}

void * DyckVertex::getProperty(const char * name) {
    if (this->hasProperty(name)) {
        return properties[name];
    } else {
        return NULL;
    }
}

bool DyckVertex::hasProperty(const char * name) {
    return properties.count(name) != 0;
}

void DyckVertex::removeProperty(const char * name) {
    properties.erase(name);
}

map<const char*, void*>& DyckVertex::getAllProperties() {
    return properties;
}

bool DyckVertex::hasLabelsBesides(void* label) {
    set<void*>::iterator it = out_lables.begin();
    while (it != out_lables.end()) {
        if (*it != label) {
            if (out_vers.count(*it)) {
                set<DyckVertex*> * dvs = out_vers[*it];
                if (dvs->size() != 0) {
                    return true;
                } else {
                    out_vers.erase(*it);
                    delete dvs;
                    out_lables.erase(it++);
                    continue;
                }
            } else {
                out_lables.erase(it++);
                continue;
            }
        }

        it++;
    }


    it = in_lables.begin();
    while (it != in_lables.end()) {
        if (*it != label) {
            if (in_vers.count(*it)) {
                set<DyckVertex*> * dvs = in_vers[*it];
                if (dvs->size() != 0) {
                    return true;
                } else {
                    in_vers.erase(*it);
                    delete dvs;
                    in_lables.erase(it++);
                    continue;
                }
            } else {
                in_lables.erase(it++);
                continue;
            }
        }

        it++;
    }

    return false;
}


// the followings are private functions

void DyckVertex::addSource(DyckVertex* ver, void* label) {
    in_lables.insert(label);
    if (!in_vers.count(label)) {
        set<DyckVertex*>* srcs = new set<DyckVertex*>;
        srcs->insert(ver);
        in_vers.insert(pair<void*, set<DyckVertex*>* >(label, srcs));
    } else {
        in_vers[label] ->insert(ver);
    }
}

void DyckVertex::removeSource(DyckVertex* ver, void* label) {
    if (in_vers.count(label)) {
        in_vers[label]->erase(ver);
    }
}

set<DyckVertex*> * DyckVertex::getEquivClass() {
    if (representative == this) {
        return equivclass;
    } else {
        return representative->getEquivClass();
    }
}
