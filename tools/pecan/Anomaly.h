/* 
 * File:   anomaly.hpp
 * Author: jack
 *
 * Created on October 23, 2013, 2:45 PM
 */
#include <vector>
#include "Event.h"
#include <string.h>

using namespace std;

#ifndef ANOMALY_HPP
#define	ANOMALY_HPP

#define DATA_RACE 100
#define S_ATOMICITY 101
#define M_ATOMICITY 102

#define MIX 103

class Anomaly {
private:
    vector<Event *> events;
    unsigned max_event_num;
    int type;
    string  typeStr;

    vector<Anomaly*> anomalies;

public:

    Anomaly(int t) {
        type = t;
        anomalies.push_back(this);

        switch (type) {
            case DATA_RACE:
                max_event_num = 2;
                typeStr = "data races";
                break;
            case S_ATOMICITY:
                max_event_num = 3;
                typeStr = "single-variable atomicity violations";
                break;
            case M_ATOMICITY:
                max_event_num = 4;
                typeStr = "multi-variable atomicity violations";
                break;
            case MIX:
                max_event_num = 40;
                typeStr = "mixed violations";
                break;
        }
    }

    ~Anomaly() {
    }

    bool add_event(Event * e) {
        if (events.size() >= max_event_num) return false;
        
        events.push_back(e);
        return true;
    }

    bool equals(Anomaly * a) {
        if (this->type != a->type) return false;

        return this->containsAllEventsIn(a) && a->containsAllEventsIn(this);
    }

    string getTypeStr() {
        return typeStr;
    }

    bool add_anomaly(Anomaly * x) {
        if (this->type != MIX) {
            printf("[CANARY] cannot add an anomaly into a non-mixed anomaly!");
            exit(-1);
        }

	return false;
    }
    
    Event * getEvent(int idx){
        if(idx>=0 && ((unsigned)idx)<events.size()){
            return events[idx];
        }
        return NULL;
    }
    
    bool containsAllEventsIn(Anomaly * a){
        bool ret = true;
        
        for (unsigned j = 0; j < a->events.size(); j++) {
            int aln = a->events[j]->line;
            const char * asrc = a->events[j]->srcfile;
            int amem = a->events[j]->mem;
            int atype = a->events[j]->type;
            bool findOne = true;
            
            for (unsigned i = 0; i < events.size(); i++) {
                int ln = events[i]->line;
                const char * src = events[i]->srcfile;
                int mem = events[i]->mem;
                int type = events[j]->type;
                if (ln != aln || strcmp(asrc, src) != 0 || amem != mem || atype != type) {
                    findOne = false;
                    break;
                }
            }
            
            if (!findOne) {
                ret = false;
                break;
            }
        }

        return ret;
    }

};


#endif	/* ANOMALY_HPP */

