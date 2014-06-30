/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef CANARYTHREAD_H
#define	CANARYTHREAD_H

#include "Log.h"
#ifdef CACHE_ENABLED
#include "../Cache.h"
#endif

class canary_thread_t {
public:
    l_rlog_t* rlog;
    l_lrlog_t* lrlog;
    l_wlog_t* wlog;
    l_addmap_t* addlog;
    unsigned onexternal;
    unsigned tid;

    const unsigned num_shared;
    const unsigned num_local;

#ifdef CACHE_ENABLED
    Cache* cache;
#endif

    canary_thread_t(unsigned ns, unsigned nl, unsigned id) : onexternal(0), tid(id), num_shared(ns), num_local(nl) {
        addlog = new l_addmap_t;
        lrlog = new l_lrlog_t[nl];
        rlog = new l_rlog_t[ns];
        wlog = new l_wlog_t[ns];

#ifdef CACHE_ENABLED       
        cache = new Cache;
#endif
    }

    ~canary_thread_t() {
        delete[] rlog;
        delete[] lrlog;
        delete[] wlog;
        delete addlog;
#ifdef CACHE_ENABLED
        delete cache;
#endif
    }

    void dump(FILE* file_read, FILE * file_lread, FILE * file_write, FILE* file_add) {
        // ==============================================================
#ifdef CACHE_ENABLED
        cache->info();
#endif       

        // ==============================================================
        if (file_read != NULL) {
#ifdef LDEBUG
            fprintf(file_read, "Thread %u \n", tid);
#else
            fwrite(&tid, sizeof (unsigned), 1, file_read);
#endif
            for (unsigned i = 0; i < num_shared; i++) {
                rlog[i].VAL_LOG.dump(file_read);
                rlog[i].VER_LOG.dump(file_read);
            }
        }

        // ==============================================================
        if (file_lread != NULL) {
#ifdef LDEBUG
            fprintf(file_lread, "Thread %u \n", tid);
#else
            fwrite(&tid, sizeof (unsigned), 1, file_lread);
#endif
            for (unsigned i = 0; i < num_local; i++) {
                lrlog[i].dump(file_lread);
            }
        }

        // ==============================================================
        if (file_write != NULL) {
#ifdef LDEBUG
            fprintf(file_write, "Thread %u \n", tid);
#else
            fwrite(&tid, sizeof (unsigned), 1, file_write);
#endif
            for (unsigned i = 0; i < num_shared; i++) {
                wlog[i].dump(file_write);
            }
        }

        // ==============================================================
        if (file_add != NULL) {
            unsigned size = addlog->adds.size();
#ifdef LDEBUG
            fprintf(file_add, "Size = %u, Thread = %u\n", size, tid);
#else
            fwrite(&size, sizeof (unsigned), 1, file_add);
            fwrite(&tid, sizeof (unsigned), 1, file_add);
#endif
            for (unsigned i = 0; i < size; i++) {
                mem_t& m = addlog->adds.at(i);
#ifdef LDEBUG
                fprintf(file_add, "(%p, %u, %d); ", m.address, m.range, m.type);
#else
                fwrite(&m, sizeof (mem_t), 1, file_add);
#endif
            }
#ifdef LDEBUG
            fprintf(file_add, "\n");
#endif
        }
    }
};



#endif	/* CANARYTHREAD_H */

