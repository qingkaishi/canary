#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <map>
#include <vector>
#include <set>

#include "Hbgraph.h"
#include "Event.h"
#include "Anomaly.h"

using namespace std;

/* ************************************************************************
 * Event
 * ************************************************************************/
#define MAX_EVENT_NUM 100000

vector<Anomaly*> drs;
vector<Anomaly*> savs;
vector<Anomaly*> mavs;

Event events[MAX_EVENT_NUM];
long events_pointer = 0;

/* ************************************************************************
 * Thread
 * ************************************************************************/

struct Thread {
    pthread_t tid;
    vector<Event *> events;
};
vector<struct Thread *> threads;
map<pthread_t, struct Thread *> _map;

HBGraph * hbgraph = NULL;

void init_threads() {
    map<pthread_t, set<int>* > _locks_map;
    for (int i = 0; i < events_pointer; i++) {
        // init locks for each event
        set<int>* _locks = NULL;
        if (!_locks_map.count(events[i].tid)) {
            _locks = new set<int>;
            _locks_map.insert(pair<pthread_t, set<int>* >(events[i].tid, _locks));
        } else {
            _locks = _locks_map[events[i].tid];
        }

        if (events[i].type == ACQUIRE) {
            _locks->insert(events[i].mem);
        }

        events[i].lock_num = _locks->size();
        events[i].locks = new int[events[i].lock_num];
        int index = 0;
        set<int>::iterator it = _locks->begin();
        while (it != _locks->end()) {
            events[i].locks[index++] = *it;
            it++;
        }

        if (events[i].type == RELEASE) {
            _locks->erase(events[i].mem);
        }

        // init threads
        if (_map.count(events[i].tid)) {
            _map[events[i].tid]->events.push_back(&events[i]);
        } else {
            struct Thread * thread = new struct Thread;
            thread->tid = events[i].tid;
            thread->events.push_back(&events[i]);

            threads.push_back(thread);
            _map.insert(pair<pthread_t, struct Thread *>(events[i].tid, thread));
        }
    }

    //delete lock_set
    vector<struct Thread *>::iterator it = threads.begin();
    while (it != threads.end()) {
        delete _locks_map[(*it)->tid];
        it++;
    }

    // init happens-before graph, and determine whether it is a planar graph
    hbgraph = new HBGraph(events_pointer);

    vector<int> forks;
    vector<int> joins;
    vector<int> notifies;
    vector<int> waits;

    // program order
    it = threads.begin();
    while (it != threads.end()) {
        vector<Event *> es = (*it)->events;
        for (unsigned i = 0; i < es.size(); i++) {
            if (i + 1 < es.size())
                hbgraph->insert_edge(es[i]->eid, es[i + 1]->eid);

            switch (es[i]->type) {
                case FORK:
                    forks.push_back(es[i]->eid);
                    break;
                case JOIN:
                    joins.push_back(es[i]->eid);
                    break;
                case WAIT:
                    waits.push_back(es[i]->eid);
                    break;
                case NOTIFY:
                    notifies.push_back(es[i]->eid);
                    break;
                default: break;
            }
        }
        it++;
    }
    // fork join order
    for (unsigned i = 0; i < forks.size(); i++) {
        pthread_t forked_tid = events[forks[i]].synctid;
        hbgraph->insert_edge(events[forks[i]].eid, (_map[forked_tid]->events).front()->eid);
    }
    for (unsigned i = 0; i < joins.size(); i++) {
        pthread_t joined_tid = events[joins[i]].synctid;
        hbgraph->insert_edge((_map[joined_tid]->events).back()->eid, events[joins[i]].eid);
    }

    // wait notify order
    for (unsigned i = 0; i < waits.size(); i++) {
        struct Event ei = events[waits[i]];
        pthread_cond_t * eicond = ei.cond;
        int eimem = ei.mem;
        pthread_t eitid = ei.tid;

        for (unsigned j = 0; j < notifies.size(); j++) {
            struct Event ej = events[notifies[j]];
            pthread_cond_t * ejcond = ej.cond;
            int ejmem = ej.mem;
            pthread_t ejtid = ej.tid;
            // FIXME
            if (waits[i] > notifies[j] && eicond == ejcond && eimem == ejmem && eitid != ejtid) {
                hbgraph->insert_edge(ej.eid, ei.eid);
            }
        }
    }
}

/* ************************************************************************
 * Main program
 * ************************************************************************/
FILE *fout = NULL;

void init(const char * filename, const char* outfilename) {
    FILE* fin = fopen(filename, "rb");
    fout = fopen(outfilename, "w+");

    if (!fin) {
        printf("[PECAN] Cannot open File %s!\n", filename);
        exit(-1);
    }

    if (!fout) {
        printf("[PECAN] Cannot open File %s!\n", outfilename);
        exit(-1);
    }

    fread(&events_pointer, sizeof (long), 1, fin);
    printf("[PECAN] Read %ld events from log file.\n", events_pointer);
    fread(events, sizeof (struct Event), events_pointer, fin);
    fclose(fin);

    init_threads();
}

void dump() {
    printf("[PECAN] DR: %zd \t SAV: %zd \t MAV: %zd \n", drs.size(), savs.size(), mavs.size());
    fprintf(fout, "[PECAN] DR: %zd \t SAV: %zd \t MAV: %zd \n", drs.size(), savs.size(), mavs.size());
    fclose(fout);
    fclose(stdout);
    fclose(stderr);
}

int hasSameLock(struct Event *e1, struct Event *e2) {
    assert(e1->tid != e2->tid);

    int * locks1 = e1->locks;
    int num1 = e1->lock_num;

    int * locks2 = e2->locks;
    int num2 = e2->lock_num;

    for (int i = 0; i < num1; i++) {
        int locki = locks1[i];
        for (int j = 0; j < num2; j++) {
            int lockj = locks2[j];
            if (locki == lockj) return 1;
        }
    }
    return 0;
}

/* ************************************************************************
 * Predictive process
 * ************************************************************************/
bool check(struct Event* ei, struct Event* ej) {
    if (!hbgraph->is_reachable(ei->eid, ej->eid) && !hbgraph->is_reachable(ej->eid, ei->eid) && !hasSameLock(ei, ej)) {
        return true;
    }

    return false;
}

bool checkDR(struct Event* ei, struct Event* ej) {
    if (hbgraph->is_reachable(ei->eid, ej->eid)
            || hbgraph->is_reachable(ej->eid, ei->eid)) {
        return false;
    }

    if (ei->tid != ej->tid && ei->mem == ej->mem && (ei->type == WRITE || ej->type == WRITE)) {
        return true;
    }
    return false;
}

bool checkSAV(struct Event* ei_1, struct Event* ej, struct Event* ei_2) {
    assert(ei_1->tid == ei_2->tid && ei_1->tid != ej->tid);
    assert(ei_1->mem == ei_2->mem && ei_1->mem == ej->mem);

    if (hbgraph->is_reachable(ei_1->eid, ej->eid)
            || hbgraph->is_reachable(ej->eid, ei_2->eid)
            || hbgraph->is_reachable(ej->eid, ei_1->eid)
            || hbgraph->is_reachable(ej->eid, ei_2->eid)) {
        return false;
    }

    if (ei_1->type == READ && ej->type == WRITE && ei_2->type == READ) return true;
    if (ei_1->type == WRITE && ej->type == READ && ei_2->type == WRITE) return true;
    if (ei_1->type == WRITE && ej->type == WRITE && ei_2->type == READ) return true;
    if (ei_1->type == READ && ej->type == WRITE && ei_2->type == WRITE) return true;
    if (ei_1->type == WRITE && ej->type == WRITE && ei_2->type == WRITE) return true;

    return false;
}

bool checkMAV(struct Event* ei_1, struct Event* ei_2, struct Event* ej_1, struct Event* ej_2) {
    assert(ei_1->tid == ei_2->tid && ei_1->tid != ej_1->tid && ej_1->tid == ej_2->tid);
    assert((ei_1->mem == ej_1->mem || ei_1->mem == ej_2->mem) && (ei_2->mem == ej_1->mem || ei_2->mem == ej_2->mem));

    if (hbgraph->is_reachable(ei_1->eid, ej_1->eid)
            || hbgraph->is_reachable(ej_1->eid, ei_1->eid)
            || hbgraph->is_reachable(ej_2->eid, ei_2->eid)
            || hbgraph->is_reachable(ei_2->eid, ej_2->eid)) {
        return false;
    }

    if (ei_1->type == WRITE && ei_2->type == WRITE && ej_1->type == WRITE && ej_2->type == WRITE) return true;
    if (ei_1->type == WRITE && ei_2->type == WRITE && ej_1->type == READ && ej_2->type == READ) return true;
    if (ei_1->type == READ && ei_2->type == READ && ej_1->type == WRITE && ej_2->type == WRITE) return true;

    return false;
}

void predictDataRaces() {

    for (int i = 0; i < events_pointer - 1; i++) {
        struct Event ei = events[i];
        if (ei.type != READ && ei.type != WRITE) {
            continue;
        }

        for (int j = i + 1; j < events_pointer; j++) {
            struct Event ej = events[j];
            if (ej.type != READ && ej.type != WRITE) {
                continue;
            }

            if (checkDR(&ei, &ej)) {
                if (check(&ei, &ej)) {
                    //printf("[PECAN] [Data Races] %s at Line %d (Thread %lu)\t%s at Line %d (Thread %lu).\n", ei.type == READ ? "READ" : "WRITE", ei.line, ei.tid, ej.type == READ ? "READ" : "WRITE", ej.line, ej.tid);
                    fprintf(fout, "[Data Races] %s at Line %ld (Thread %lu)\t%s at Line %ld (Thread %lu).\n", ei.type == READ ? "READ" : "WRITE", ei.line, ei.tid, ej.type == READ ? "READ" : "WRITE", ej.line, ej.tid);

                    Anomaly* dr = new Anomaly(DATA_RACE);
                    dr->add_event(&ei);
                    dr->add_event(&ej);

                    int existed = 0;
                    for (unsigned o = 0; o < drs.size(); o++) {
                        if (drs[o]->equals(dr)) {
                            existed = 1;
                            break;
                        }
                    }
                    if (!existed) {
                        drs.push_back(dr);
                    }
                }
            }
        }
    }

}

bool canFindOne(int from, int to, vector<struct Event *>& vec, struct Event* ej) {
    for (int q = from; q <= to; q++) {
        struct Event * eq = vec[q];
        if (check(eq, ej)) {
            return true;
        }
    }

    return false;
}

void predictSAVs() {
    for (unsigned i = 0; i < threads.size(); i++) {
        struct Thread * thread_i = threads[i];
        vector<Event *> events_i = thread_i->events;
        for (unsigned j = 0; j < threads.size(); j++) {
            if (i == j) continue;

            struct Thread * thread_j = threads[j];
            vector<Event *> events_j = thread_j->events;

            for (unsigned m = 0; m < events_i.size(); m++) {
                for (unsigned n = m + 1; n < events_i.size(); n++) {

                    if (events_i[m]->mem != events_i[n]->mem) continue;

                    for (unsigned p = 0; p < events_j.size(); p++) {
                        struct Event * ej = events_j[p];

                        if (events_i[m]->mem != ej->mem) continue;

                        if (checkSAV(events_i[m], events_j[p], events_i[n])) {
                            if (canFindOne(m, n, events_i, ej)) {
                                //printf("[PECAN] [sAtomicity] Line %d\tLine %d\t Line %d.\n", events_i[m]->line, events_j[p]->line, events_i[n]->line);
                                fprintf(fout, "[sAtomicity] Line %ld\tLine %ld\t Line %ld.\n", events_i[m]->line, events_j[p]->line, events_i[n]->line);

                                Anomaly* sav = new Anomaly(S_ATOMICITY);
                                sav->add_event(events_i[m]);
                                sav->add_event(ej);
                                sav->add_event(events_i[n]);

                                bool existed = false;
                                for (unsigned o = 0; o < savs.size(); o++) {
                                    if (savs[o]->equals(sav)) {
                                        existed = true;
                                        break;
                                    }
                                }
                                if (!existed) {
                                    savs.push_back(sav);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void predictMAVs() {

    for (unsigned i = 0; i < threads.size(); i++) {
        struct Thread * thread_i = threads[i];
        vector<Event *> events_i = thread_i->events;
        for (unsigned j = i + 1; j < threads.size(); j++) {
            struct Thread * thread_j = threads[j];
            vector<Event *> events_j = thread_j->events;

            // find two from events_i 
            for (unsigned m = 0; m < events_i.size(); m++) {
                Event * em = events_i[m];
                for (unsigned n = m + 1; n < events_i.size(); n++) {
                    Event * en = events_i[n];
                    if (em->mem == en->mem || em->type != en->type) continue;

                    // find two from events_j
                    for (unsigned p = 0; p < events_j.size(); p++) {
                        Event * ep = events_j[p];
                        if (ep->mem != em->mem && ep->mem != en->mem) continue;
                        for (unsigned q = p + 1; q < events_j.size(); q++) {
                            Event * eq = events_j[q];
                            bool same_order = false;
                            bool potential = false;

                            if (ep->mem == eq->mem || ep->type != eq->type) continue;
                            if (ep->mem == em->mem && eq->mem != en->mem) continue;
                            if (ep->mem == en->mem && eq->mem != em->mem) continue;

                            if (ep->mem == em->mem) same_order = true;

                            if (checkMAV(em, en, ep, eq)) {
                                if (same_order) {
                                    if ((canFindOne(m, n, events_i, ep) && canFindOne(m, n, events_i, eq)) || (canFindOne(p, q, events_j, em) && canFindOne(p, q, events_j, en))) {
                                        potential = true;
                                    }
                                } else {
                                    if (canFindOne(m, n, events_i, ep) || canFindOne(p, q, events_j, em)) {
                                        potential = true;
                                    }
                                }
                            }

                            if (potential) {
                                //printf("[PECAN] [mAtomicity] Line %d\tLine %d\t Line %d\t Line %d.\n", em->line, en->line, ep->line, eq->line);
                                fprintf(fout, "[mAtomicity] Line %ld\tLine %ld\t Line %ld\t Line %ld.\n", em->line, en->line, ep->line, eq->line);
                                
                                Anomaly* mav = new Anomaly(M_ATOMICITY);
                                mav->add_event(em);
                                mav->add_event(en);
                                mav->add_event(ep);
                                mav->add_event(eq);

                                bool existed = false;
                                for (unsigned o = 0; o < mavs.size(); o++) {
                                    if (mavs[o]->equals(mav)) {
                                        existed = true;
                                        break;
                                    }
                                }
                                if (!existed) {
                                    mavs.push_back(mav);
                                }
                            }
                        }
                    }
                }
            }

        }
    }
}

int main(int argc, char * argv[]) {
    printf("WARNING: current version may only work for some specific programs, because some hard codes exit.\n\n");

    if (argc != 3) {
        printf("Please provide two arguments, which indicate the input log file and the output result file, respectively.\n");
    } else {
        init(argv[1], argv[2]);
    }

    /* test boost graph
    hbgraph = new HBGraph(8);
    hbgraph->insert_edge(0, 2);
    hbgraph->insert_edge(1, 3);
    hbgraph->insert_edge(3, 4);
    hbgraph->insert_edge(4, 5);
   
    if(hbgraph->is_reachable(1, 5)){
        printf("1->5\n");
    }
    
     std::ofstream out("tc-out.dot");
     boost::write_graphviz(out, *(hbgraph->self()));
     */

    predictDataRaces();
    predictSAVs();
    predictMAVs();

    dump();
}
