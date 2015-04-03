/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include "DyckVertex.h"
#include <assert.h>

int DyckVertex::global_indx = 0;

DyckVertex::DyckVertex(void * v, const char * itsname) {
	name = itsname;
	index = global_indx++;
	value = v;
	representative = this;
	equivclass = new set<void*>;

	if (value != NULL) {
		equivclass->insert(value);
	}
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

	map<void*, set<DyckVertex*>*>::iterator iit = in_vers.begin();
	while (iit != in_vers.end()) {

		ret = ret + iit->second->size();
		iit++;
	}

	map<void*, set<DyckVertex*>*>::iterator oit = out_vers.begin();
	while (oit != out_vers.end()) {

		ret = ret + oit->second->size();
		oit++;
	}

	return ret;
}

set<void*>* DyckVertex::getEquivalentSet() {
	DyckVertex* rep = this->getRepresentative();
	set<void*>* alias = rep->equivclass;
	assert(alias != NULL && "ERROR in DyckVertex::getAliases(DyckVertex*) \n");
	return alias;
}

bool DyckVertex::inSameEquivalentSet(DyckVertex* v1) {
	return this->getRepresentative() == v1->getRepresentative();
}

void DyckVertex::setRepresentative(DyckVertex* rootRep) {
	rootRep = rootRep->getRepresentative();
	DyckVertex * thisRep = this->getRepresentative();

	if (rootRep == thisRep) {
		return;
	}

	set<void*> * rootecls = rootRep->getEquivalentSet();
	set<void*> * thisecls = this->getEquivalentSet();

	rootecls->insert(thisecls->begin(), thisecls->end());

	thisRep->representative = rootRep;
	delete thisecls;
	thisRep->equivclass = NULL; // set null after delete
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

map<void*, set<DyckVertex*>*>& DyckVertex::getOutVertices() {
	return out_vers;
}

set<void*>& DyckVertex::getInLabels() {
	return in_lables;
}

map<void*, set<DyckVertex*>*>& DyckVertex::getInVertices() {
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
		out_vers.insert(pair<void*, set<DyckVertex*>*>(label, tars));
	} else {
		out_vers[label]->insert(ver);
	}

	ver->addSource(this, label);
}

void DyckVertex::removeTarget(DyckVertex* ver, void* label) {
	if (out_vers.count(label)) {
		out_vers[label]->erase(ver);
	}

	ver->removeSource(this, label);
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

// the followings are private functions

void DyckVertex::addSource(DyckVertex* ver, void* label) {
	in_lables.insert(label);
	if (!in_vers.count(label)) {
		set<DyckVertex*>* srcs = new set<DyckVertex*>;
		srcs->insert(ver);
		in_vers.insert(pair<void*, set<DyckVertex*>*>(label, srcs));
	} else {
		in_vers[label]->insert(ver);
	}
}

void DyckVertex::removeSource(DyckVertex* ver, void* label) {
	if (in_vers.count(label)) {
		in_vers[label]->erase(ver);
	}
}
