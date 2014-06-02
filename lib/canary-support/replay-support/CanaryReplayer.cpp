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
#include "../Cache.h"

/*
 * Define global log variables, each shared var has one
 */
static std::vector<g_llog_t *> llogs; // <g_llog_t *> size is not fixed, and should be determined at runtime
static g_flog_t flog;
static g_mlog_t mlog;

static l_rlog_t* rlogs[CANARY_THREADS_MAX];
static l_lrlog_t* lrlogs[CANARY_THREADS_MAX];
static l_wlog_t* wlogs[CANARY_THREADS_MAX];
static l_addmap_t* addlogs[CANARY_THREADS_MAX];

/*
 * number of shared variables
 */
static unsigned num_shared_vars = 0;
static unsigned num_local_vars = 0;

/*
 * set of active threads, not the original tid
 */
static std::set<unsigned> active_threads;

/*
 * locks for synchronization, each shared var has one
 */
static pthread_mutex_t* locks;
static pthread_mutex_t fork_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_init_lock = PTHREAD_MUTEX_INITIALIZER;

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

extern "C" {

    void OnInit(unsigned svsNum, unsigned lvsNum) {
        printf("OnInit-Replay (canary replayer)\n");
    }

    void OnExit() {
        printf("OnExit-Replay (canary replayer)\n");
    }

    void OnAddressInit(void* value, size_t size, size_t n) {
    }

    void OnLoad(int svId, long address, long value, int debug) {
    }

    unsigned OnPreStore(int svId, int debug) {
        return 0;
    }

    void OnStore(int svId, unsigned version, long address, long value, int debug) {
    }

    void OnLock(pthread_mutex_t* mutex_ptr) {
    }

    void OnWait(pthread_cond_t* cond_ptr, pthread_mutex_t* mutex_ptr) {
    }

    void OnPreMutexInit(bool race) {
    }

    void OnMutexInit(pthread_mutex_t* mutex_ptr, bool race) {
    }

    void OnPreFork(bool race) {
    }

    void OnFork(pthread_t* forked_tid_ptr, bool race) {
    }

    void OnJoin(pthread_t tid, bool race) {
    }

    void OnLocal(long value, int id) {
    }

}
