#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

#include <boost/unordered_map.hpp>

#include "SignalRoutine.h"
#include "../Log.h"

/*
 * Define log types
 * l_ means local log types, each thread has a log
 * g_ means global log types, each shared variable has a log
 * r  -> read 
 * w  -> write
 * l  -> lock
 * m  -> mutex init
 * f  -> fork
 */
typedef struct {
    LastOnePredictorLog<long> VAL_LOG; // <void *>
    LastOnePredictorLog<unsigned> VER_LOG; // <unsigned>
} l_rlog_t;

typedef VLastOnePredictorLog l_wlog_t;

typedef LastOnePredictorLog<unsigned> g_llog_t;

typedef LastOnePredictorLog<unsigned> g_mlog_t;

typedef LastOnePredictorLog<unsigned> g_flog_t;

/*
 * Define local log keys
 */
static pthread_key_t rlog_key;
static pthread_key_t wlog_key;

/*
 * Define global log variables, each shared var has one
 */
static std::vector<g_llog_t *> llogs; // <g_llog_t *> size is not fixed, and should be determined at runtime
static g_flog_t flog;
static g_mlog_t mlog;

/*
 * write versions, each shared var has one
 */
static unsigned* write_versions;

/*
 * locks for synchronization, each shared var has one
 */
static pthread_mutex_t* locks;
static pthread_mutex_t fork_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_init_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * mutex_t-> mutex_id hashmap
 * pthread_t -> thread_id hashmap
 */
static boost::unordered_map<pthread_mutex_t *, unsigned> mutex_ht;
static boost::unordered_map<pthread_t, unsigned> thread_ht;
static boost::unordered_map<pthread_t, l_rlog_t*> rlogs;
static boost::unordered_map<pthread_t, l_wlog_t*> wlogs;
static boost::unordered_map<long, unsigned> value_ht;

/*
 * start to record?
 */
static bool start = false;

/*
 * number of shared variables
 */
static unsigned num_shared_vars = 0;

#ifdef NO_TIME_CMD
static struct timeval tpstart, tpend;
#endif

static inline void lock(unsigned svId) {
    pthread_mutex_lock(&locks[svId]);
}

static inline void unlock(unsigned svId) {
    pthread_mutex_unlock(&locks[svId]);
}

static inline void forkLock() {
    pthread_mutex_lock(&fork_lock);
}

static inline void forkUnlock() {
    pthread_mutex_unlock(&fork_lock);
}

static inline void mutexInitLock() {
    pthread_mutex_lock(&mutex_init_lock);
}

static inline void mutexInitUnlock() {
    pthread_mutex_unlock(&mutex_init_lock);
}

/* delete logs
 */
void close_read_log(void* log) {
    pthread_t tid = pthread_self();
    rlogs[tid] = (l_rlog_t*) log;
}

void close_write_log(void* log) {
    pthread_t tid = pthread_self();
    wlogs[tid] = (l_wlog_t*) log;
}

extern "C" {
    
    void OnInit(unsigned svsNum) {
        printf("OnInit-Record\n");
        num_shared_vars = svsNum;
        initializeSigRoutine();

        pthread_key_create(&rlog_key, close_read_log);
        pthread_key_create(&wlog_key, close_write_log);

        write_versions = new unsigned[svsNum];
        locks = new pthread_mutex_t[svsNum];

        for (unsigned i = 0; i < svsNum; i++) {
            pthread_mutex_init(&locks[i], NULL);
        }

        // main thread.
        pthread_t tid = pthread_self();
        thread_ht[tid] = thread_ht.size();

#ifdef NO_TIME_CMD
        gettimeofday(&tpstart, NULL);
#endif
    }

    void OnExit() {
        start = false;

#ifdef NO_TIME_CMD
        gettimeofday(&tpend, NULL);
        double timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
        timeuse /= 1000;
        printf("processor time is %lf ms\n", timeuse);
#endif
        printf("OnExit-Record\n");

        // dump, delete/free is not needed, because program will exit.
        {//flog
            flog.dump("fork.dat", "wb");
        }

        {
            //mlog, llogs
            mlog.dump("mutex.dat", "wb");
            for (unsigned i = 0; i < llogs.size(); i++) {
                g_llog_t * llog = llogs[i];
                llog->dump("mutex.dat", "ab");
            }
        }

        {//rlog
            bool created = false;
            boost::unordered_map<pthread_t, l_rlog_t*>::iterator rit = rlogs.begin();
            while (rit != rlogs.end()) {
                l_rlog_t* rlog = rit->second;
                unsigned _tid = thread_ht[rit->first];
                for (unsigned i = 0; i < num_shared_vars; i++) {
                    if(!created) {
                        rlog[i].VAL_LOG.dumpWithUnsigned("read.dat", "wb", _tid);
                        rlog[i].VER_LOG.dumpWithUnsigned("read.dat", "ab", _tid);
                        
                        created = true;
                    } else {
                        rlog[i].VAL_LOG.dumpWithUnsigned("read.dat", "ab", _tid);
                        rlog[i].VER_LOG.dumpWithUnsigned("read.dat", "ab", _tid);
                    }
                }

                rit++;
            }
        }
        {//wlog
            bool created = false;
            boost::unordered_map<pthread_t, l_wlog_t*>::iterator wit = wlogs.begin();
            while (wit != wlogs.end()) {
                l_wlog_t* wlog = wit->second;
                unsigned _tid = thread_ht[wit->first];
                for (unsigned i = 0; i < num_shared_vars; i++) {
                    if(!created){
                        wlog->dumpWithUnsigned("write.dat", "wb", _tid);
                        
                        created = true;
                    }else{
                        wlog->dumpWithUnsigned("write.dat", "ab", _tid);
                    }
                }

                wit++;
            }
        }
    }

    void OnLoad(int svId, long value, int debug) {
        if (!start) {
            return;
        }

#ifdef DEBUG
        printf("OnLoad\n");
#endif
        // using thread_key to tell whether log has been established
        // if so, use it, otherwise, new one
        l_rlog_t * rlog = (l_rlog_t*) pthread_getspecific(rlog_key);
        if (rlog == NULL) {
            rlog = new l_rlog_t[num_shared_vars];
            pthread_setspecific(rlog_key, rlog);
        }

        if(value_ht.count(value)){
            rlog[svId].VAL_LOG.logValue(value_ht[value]);
        } else {
            rlog[svId].VAL_LOG.logValue(value);
        }
        
        rlog[svId].VER_LOG.logValue(write_versions[svId]);
    }

    unsigned OnPreStore(int svId, long value, int debug) {
        if (!start) {
            return 0;
        }

        lock(svId);
#ifdef DEBUG
        printf("OnPreStore: %d [%d]\n", svId, debug);
#endif
        unsigned version = write_versions[svId];
        write_versions[svId] = version + 1;
        
        if(!value_ht.count(value)){
            value_ht[value] = value_ht.size();
        }

        return version;
    }

    void OnStore(int svId, unsigned version, int debug) {
        if (!start) {
            return;
        }
#ifdef DEBUG
        printf("OnStore\n");
#endif
        unlock(svId);

        l_wlog_t * wlog = (l_wlog_t*) pthread_getspecific(wlog_key);
        if (wlog == NULL) {
            wlog = new l_wlog_t[num_shared_vars];
            pthread_setspecific(wlog_key, wlog);
        }
        wlog[svId].logValue(version);
    }

    void OnLock(pthread_mutex_t* mutex_ptr) {
        if (!start) {
            return;
        }

        pthread_t tid = pthread_self();
        while (!thread_ht.count(tid));
        unsigned _tid = thread_ht[tid];

        if (!mutex_ht.count(mutex_ptr)) {
            fprintf(stderr, "ERROR: program bug!\n");
            OnExit();
            exit(1);
        }

        g_llog_t* llog = llogs[mutex_ht[mutex_ptr]];
        llog->logValue(_tid);

#ifdef DEBUG
        printf("OnLock --> t%d\n", _tid);
#endif
    }

    void OnWait(pthread_cond_t* cond_ptr, pthread_mutex_t* mutex_ptr) {
        if (!start) {
            return;
        }
#ifdef DEBUG
        printf("OnWait\n");
#endif
        pthread_t tid = pthread_self();
        while (!thread_ht.count(tid));
        unsigned _tid = thread_ht[tid];

        if (!mutex_ht.count(mutex_ptr)) {
            fprintf(stderr, "ERROR: program bug!\n");
            OnExit();
            exit(1);
        }
        g_llog_t* llog = llogs[mutex_ht[mutex_ptr]];
        llog->logValue(_tid);
    }

    void OnPreMutexInit(bool race) {
        if (start && race) {
            mutexInitLock();
        }
    }

    void OnMutexInit(pthread_mutex_t* mutex_ptr, bool race) {
        if (!mutex_ht.count(mutex_ptr)) {
            mutex_ht[mutex_ptr] = mutex_ht.size();
            g_llog_t * llog = new g_llog_t;
            llogs.push_back(llog);
        }

        if (start && race) {
            pthread_t tid = pthread_self();
            while (!thread_ht.count(tid));
            unsigned _tid = thread_ht[tid];
            mlog.logValue(_tid);

            mutexInitUnlock();
        }
    }

    void OnPreFork(bool race) {
        if (!start) {
            start = true;
        }
#ifdef DEBUG
        printf("OnPreFork\n");
#endif
        if (race) forkLock();
    }

    void OnFork(pthread_t* forked_tid_ptr, bool race) {
        if (!start) {
            return;
        }

#ifdef DEBUG
        printf("OnFork\n");
#endif

        pthread_t ftid = *(forked_tid_ptr);
        if (!thread_ht.count(ftid)) {
            thread_ht[ftid] = thread_ht.size();
        }

        if (race) {
            pthread_t tid = pthread_self();
            while (!thread_ht.count(tid));
            unsigned _tid = thread_ht[tid];

            flog.logValue(_tid);

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
