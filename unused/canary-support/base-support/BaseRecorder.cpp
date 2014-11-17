/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <set>

#include <boost/unordered_map.hpp>

//#define DEBUG
//#define NO_TIME_CMD
//#define LDEBUG
//#define CACHE_ENABLED

#include "SignalRoutine.h"
#include "../CanaryThread.h"

/*
 * number of shared/local variables
 */
static unsigned num_local_vars = 0;

/*
 * locks for synchronization, each shared var has one
 */
static pthread_mutex_t fork_lock = PTHREAD_MUTEX_INITIALIZER;

static boost::unordered_map<pthread_t, canary_thread_t*> thread_ht;

/*
 * set of active threads, not the original tid
 * 
 * static std::set<unsigned> active_threads;
 * 
 * cannot use it!!
 */

static bool start = false;
static bool forking = false;
static bool main_started = false;

#ifdef NO_TIME_CMD
static struct timeval tpstart, tpend;
#endif

static inline void forkLock() {
    pthread_mutex_lock(&fork_lock);
}

static inline void forkUnlock() {
    pthread_mutex_unlock(&fork_lock);
}

extern "C" {

    void* OnMethodStart() {
        if (!main_started)
            return NULL;

        pthread_t self = pthread_self();
        bool contains = thread_ht.count(self);
        while (!contains && forking) {
            sched_yield();
            contains = thread_ht.count(self);
        }

        if (contains) {
            canary_thread_t* st = thread_ht[self];
            if (st->onexternal == 0)
                return st;
        }

        return NULL;
    }

    void OnStartTimer() {
#ifdef NO_TIME_CMD
        gettimeofday(&tpstart, NULL);
#endif
    }

    void OnInit(unsigned svsNum, unsigned lvsNum) {
        printf("[INFO] OnInit-Record (canary record)\n");
        num_local_vars = lvsNum;
        initializeSigRoutine();

        // main thread.
        canary_thread_t* st = new canary_thread_t(0, num_local_vars, thread_ht.size());
        thread_ht[pthread_self()] = st;

        main_started = true;
        start = false;
    }

    void OnExit() {
#ifdef NO_TIME_CMD
        gettimeofday(&tpend, NULL);
        double timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
        timeuse /= 1000;
        printf("[INFO] Wall time is %lf ms\n", timeuse);
#endif
        start = false;

        printf("[INFO] OnExit-Record (canary record)\n");

        // dump, delete/free is not needed, because program will exit.
        {
#ifdef DEBUG
            printf("dumping thread local log...\n");
#endif      
            FILE * file_lread = fopen("lread.dat", "w");
            FILE * file_add = fopen("addressmap.dat", "w");

            boost::unordered_map<pthread_t, canary_thread_t*>::iterator lit = thread_ht.begin();
            while (lit != thread_ht.end()) {
                canary_thread_t* st = lit->second;
                st->dump(NULL, file_lread, NULL, file_add);

                delete st;

                lit++;
            }

            fclose(file_lread);
            fclose(file_add);
        }

        printf("[INFO] Threads num: %d\n", thread_ht.size());
        thread_ht.clear();

        // zip
        system("if [ -f canary.zip ]; then rm canary.zip; fi");
        system("zip -9 canary.zip ./addressmap.dat ./lread.dat");
        system("rm -f ./addressmap.dat ./lread.dat");
        system("echo -n \"[INFO] Log size (Byte): \"; echo -n `du -sb canary.zip` | cut -d\" \" -f 1;");
    }

    void OnPreExternalCall(void* st) {
        if (st != NULL)
            ((canary_thread_t*) st)->onexternal++;
    }

    void OnExternalCall(long value, int lvid, void* st) {
        if (st == NULL)
            return;
        unsigned oe = ((canary_thread_t*) st)->onexternal;
        ((canary_thread_t*) st)->onexternal = oe - 1;

        if (oe == 1 && lvid != -1) {
            ((canary_thread_t*) st)->lrlog[lvid].logValue(value);
        }
    }

    void OnAddressInit(void* value, size_t size, size_t n, int type, void* st) {
        if (st == NULL) {
            return;
        }

#ifdef DEBUG
        printf("OnAddressInit\n");
#endif

        l_addmap_t * mlog = ((canary_thread_t*) st)->addlog;

        if (type == 1000) { // a stack address
            if (mlog->stack_tag) {
                return;
            } else {
                mlog->stack_tag = true;
            }
        }

        mem_t m;
        m.address = value;
        m.range = size*n;
        m.type = type;
        mlog->adds.push_back(m);
    }

    void OnLoad(int svId, int lvId, long value, void* address, void* st, int debug) {
        if (st == NULL)
            return;

        bool success = false;

#ifdef CACHE_ENABLED
        success = ((canary_thread_t*) st)->cache->query(address, value);
        if (!success)
            ((canary_thread_t*) st)->cache->add(address, value);
#endif

        if ((!start && lvId != -1) || (start && svId == -1)) {
            if (!success)
                ((canary_thread_t*) st)->lrlog[lvId].logValue(value);
        } else {
            // shared are not recorded
        }

#ifdef DEBUG
        printf("OnLoad === 1\n");
#endif
    }

    unsigned OnPreStore(int svId, void* st, int debug) {
#ifdef DEBUG
        printf("OnPreStore: %d [%d]\n", svId, debug);
#endif
        return 0;
    }

    void OnStore(int svId, unsigned version, long value, void* add, void* st, int debug) {
#ifdef DEBUG
        printf("OnStore\n");
#endif
    }

    void OnLock(pthread_mutex_t* mutex_ptr, void* st) {
#ifdef DEBUG
        printf("OnLock --> t%d\n", thread_ht[pthread_self()]);
#endif
    }

    void OnWait(pthread_cond_t* cond_ptr, pthread_mutex_t* mutex_ptr, void* st) {
    }

    void OnPreMutexInit(bool race, void* st) {
    }

    void OnMutexInit(pthread_mutex_t* mutex_ptr, bool race, void* st) {
    }

    void OnPreFork(bool race, void* st) {
        if (st == NULL) {
            return;
        }

        if (!start)
            start = true;

#ifdef DEBUG
        printf("OnPreFork\n");
#endif
        if (race) forkLock();
        forking = true; // I am forking new thread.
    }

    void OnFork(pthread_t* forked_tid_ptr, bool race, void* st) {
        if (st == NULL) {
            return;
        }
#ifdef DEBUG
        printf("OnFork\n");
#endif

        pthread_t ftid = *(forked_tid_ptr);
        if (!thread_ht.count(ftid)) {
            canary_thread_t* st = new canary_thread_t(0, num_local_vars, thread_ht.size());
            thread_ht[ftid] = st;
        }

        forking = false; // Forking finishes.
        if (race) {
            forkUnlock();
        }
    }
}

/* ************************************************************************
 * Signal Process
 * ************************************************************************/

void sigroutine(int dunno) {
    printSigInformation(dunno);

    OnExit();
    exit(dunno);
}
