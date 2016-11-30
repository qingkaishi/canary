/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "DyckGraph/DyckVertex.h"
#include <assert.h>

int DyckVertex::global_indx = 0;

DyckVertex::DyckVertex(void * v, const char * itsname) {
	name = itsname;
	index = global_indx++;

	if (v != NULL) {
		equivclass.insert(v);
	}
}

DyckVertex::~DyckVertex() {
}

const char * DyckVertex::getName() {
	return name;
}

unsigned int DyckVertex::outNumVertices(void* label) {
    auto it = out_vers.find(label);
    if (it != out_vers.end()) {
        return it->second.size();
    }
    return 0;
}

unsigned int DyckVertex::inNumVertices(void* label) {
    auto it = in_vers.find(label);
    if (it != in_vers.end()) {
        return it->second.size();
    }
    return 0;
}

unsigned int DyckVertex::degree() {
	unsigned int ret = 0;

	map<void*, set<DyckVertex*>>::iterator iit = in_vers.begin();
	while (iit != in_vers.end()) {
		ret = ret + iit->second.size();
		iit++;
	}

	map<void*, set<DyckVertex*>>::iterator oit = out_vers.begin();
	while (oit != out_vers.end()) {
		ret = ret + oit->second.size();
		oit++;
	}

	return ret;
}

set<void*>* DyckVertex::getEquivalentSet() {
	return &this->equivclass;
}

void DyckVertex::mvEquivalentSetTo(DyckVertex* rootRep) {
	if (rootRep == this) {
		return;
	}

	set<void*> * rootecls = rootRep->getEquivalentSet();
	set<void*> * thisecls = this->getEquivalentSet();

	rootecls->insert(thisecls->begin(), thisecls->end());
}

set<void*>& DyckVertex::getOutLabels() {
	return out_lables;
}

map<void*, set<DyckVertex*>>& DyckVertex::getOutVertices() {
	return out_vers;
}

set<void*>& DyckVertex::getInLabels() {
	return in_lables;
}

map<void*, set<DyckVertex*>>& DyckVertex::getInVertices() {
	return in_vers;
}

int DyckVertex::getIndex() {
	return index;
}

void DyckVertex::addTarget(DyckVertex* ver, void* label) {
	out_lables.insert(label);
	out_vers[label].insert(ver);

	ver->addSource(this, label);
}

void DyckVertex::removeTarget(DyckVertex* ver, void* label) {
    auto it = out_vers.find(label);
    if (it != out_vers.end()) {
        it->second.erase(ver);
    }

	ver->removeSource(this, label);
}

bool DyckVertex::containsTarget(DyckVertex* tar, void* label) {
    auto it = out_vers.find(label);
    if (it != out_vers.end()) {
        return out_vers[label].count(tar);
    }

    return false;
}

set<DyckVertex*>* DyckVertex::getInVertices(void * label) {
    auto it = in_vers.find(label);
    if (it != in_vers.end()) {
        return &it->second;
    }
    return nullptr;
}

set<DyckVertex*>* DyckVertex::getOutVertices(void * label) {
    auto it = out_vers.find(label);
    if (it != out_vers.end()) {
        return &it->second;
    }
    return nullptr;
}

// the followings are private functions

void DyckVertex::addSource(DyckVertex* ver, void* label) {
	in_lables.insert(label);
	in_vers[label].insert(ver);
}

void DyckVertex::removeSource(DyckVertex* ver, void* label) {
    auto it = in_vers.find(label);
    if (it != in_vers.end()) {
        it->second.erase(ver);
    }
}
