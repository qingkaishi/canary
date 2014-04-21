#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include "Signature.h"
#include "SignalRoutine.h"
#include "util/cvector.h"
#include <boost/unordered_map.hpp>

#define POSIX_MUTEX

#include "Lock.h"

#define MAX_LOG_LEN 50000
#define MAX_THREAD_NUM 50

typedef struct {
    cvector VAL_LOG; // <void *>
    cvector VER_LOG; // <unsigned>
} RLOG;

typedef struct {
    cvector VER_LOG; // <unsigned>
} WLOG;

cvector W_VERSION;

typedef struct {
    cvector SYN_LOG; // <unsigned> record ti
} LLOG;

static LLOG llog;

static pthread_key_t rlog_key;
static pthread_key_t wlog_key;

static boost::unordered_map<pthread_t, unsigned> thread_ht;
static boost::unordered_map<void *, unsigned> mutex_ht;

static bool start = false;
static int num_shared_vars = 0;

// set when log is too long
static bool stop_recording = false;

//static struct timeval tpstart, tpend;

/* delete logs
 */
void close_read_log(void* log) {
    /// @TODO dump

    for (unsigned i = 0; i < num_shared_vars; i++) {
        cvector_destroy(((RLOG*) log)[i]->VAL_LOG);
        cvector_destroy(((RLOG*) log)[i]->VER_LOG);
    }
    delete [] (RLOG*) log;
}

void close_write_log(void* log) {
    /// @TODO dump

    for (unsigned i = 0; i < num_shared_vars; i++) {
        cvector_destroy(((WLOG*) log)[i]->VER_LOG);
    }
    delete [] (WLOG*) log;
}

void static inline storeRead(unsigned version, void* value, RLOG* rlog) {
}

void static inline storeWrite(unsigned version, WLOG* wlog) {
    if (stop_recording) {
        return;
    }

    int log_len = cvector_length(wlog->VER_LOG);
    if (log_len >= MAX_LOG_LEN) {
        printf("Log is too long to record! Stop recording!\n");
        stop_recording = true;
        return;
    }

    unsigned x = (unsigned) cvector_at(wlog->VER_LOG, log_len - 1);
    unsigned y = (unsigned) cvector_at(wlog->VER_LOG, log_len - 2);
    if (log_len > 0 && x + y == version) {
        cvector_insert_at(wlog->VER_LOG, log_len - 1, (void *) (x + 1));
    } else {
        cvector_insert_at(wlog->VER_LOG, log_len, (void *) (version));
        cvector_insert_at(wlog->VER_LOG, log_len + 1, (void *) (1));
    }
}

void static inline storeSync(unsigned syncId, unsigned tId){
}

extern "C" {

    void OnInit(int svsNum) {
        printf("OnInit-Record\n");
        num_shared_vars = svsNum;
        initializeSigRoutine();

        pthread_key_create(&rlog_key, close_read_log);
        pthread_key_create(&wlog_key, close_write_log);

        W_VERSION = cvector_create(sizeof (unsigned));
        cvector_resize(W_VERSION, svsNum);
        
        llog.SYN_LOG = cvector_create(sizeof (unsigned));
        
        initialize(svsNum + 2); // initialize locks, add one for thread_ht, the other for mutex_ht

        // main thread.
        pthread_t tid = pthread_self();
        thread_ht[tid] = thread_ht.size();

        //gettimeofday(&tpstart, NULL);
    }

    void OnExit(int nouse) {
        start = false;

        //gettimeofday(&tpend, NULL);
        //double timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
        //timeuse /= 1000;
        //printf("processor time is %lf ms\n", timeuse);
        
        ///@TODO dump llog
    }

    void OnLoad(int svId, void* value, int debug) {
        if (!start) {
            return;
        }

#ifdef DEBUG
        printf("OnLoad\n");
#endif
        // using thread_key to tell whether log has been established
        // if so, use it, otherwise, new one
        RLOG * rlog = (RLOG*) pthread_getspecific(rlog_key);
        if (rlog == NULL) {
            rlog = new RLOG[num_shared_vars];
            pthread_setspecific(rlog_key, rlog);
        }
        if (rlog[svId] == NULL)
            rlog[svId]->VAL_LOG = cvector_create(sizeof (void*));
        rlog[svId]->VER_LOG = cvector_create(sizeof (unsigned));
        storeRead((unsigned) cvector_at(W_VERSION, svId), value, rlog[svId]); /// @FIXME version, value, rlog[svid]
    }

    unsigned OnPreStore(int svId, int debug) {
        if (!start) {
            return;
        }

        lock(svId);
#ifdef DEBUG
        printf("OnPreStore: %d at t%d [%d]\n", svId, _tid, debug);
#endif
        unsigned version = W_VERSION[svId];
        W_VERSION[svId]++;

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

        WLOG * wlog = (WLOG*) pthread_getspecific(wlog_key);
        if (wlog == NULL) {
            wlog = new WLOG[num_shared_vars];
        }
        if (wlog[svId] == NULL) {
            wlog[svId]->VER_LOG = cvector_create(sizeof (unsigned));
        }
        storeWrite(version, wlog[svId]); // version, wlog
    }

    void OnLock(int nouse) {
        if (!start) {
            return;
        }

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(num_shared_vars - 2, _tid);
#ifdef DEBUG
        printf("OnLock --> t%d\n", _tid);
#endif

    }
    
    void OnPreMutexInit(pthread_mutex_t* mptr, bool race){
    
    }
    
    void OnMutexInit(pthread_mutex_t* mptr, bool race){
    
    }

    void OnPreFork(bool race) {
        if (!start) {
            start = true;
        }
#ifdef DEBUG
        printf("OnPreFork\n");
#endif
        forklock(num_shared_vars - 1);
    }

    void OnFork(long forked_tid_ptr, bool race) {
        if (!start) {
            return;
        }

        pthread_t ftid = *((pthread_t*) forked_tid_ptr);
        if (threadid(ftid) == -1) {
            threadcreate(ftid);
        }
#ifdef DEBUG
        printf("OnFork\n");
#endif
        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(num_shared_vars - 1, _tid);
        forkunlock(num_shared_vars - 1);
    }

    void OnWait(int condId, long cond_ptr, pthread_mutex_t* mutex_ptr) {
        if (!start) {
            return;
        }
#ifdef DEBUG
        printf("OnWait\n");
#endif
        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(num_shared_vars - 2, _tid);
    }
}

/* ************************************************************************
 * Signal Process
 * ************************************************************************/

void sigroutine(int dunno) {
    printSigInformation(dunno);

    OnExit(num_shared_vars);
    exit(dunno);
}
