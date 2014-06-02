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

        num_shared_vars = svsNum;
        num_local_vars = lvsNum;

        memset(addlogs, 0, sizeof (void*)*CANARY_THREADS_MAX);
        memset(wlogs, 0, sizeof (void*)*CANARY_THREADS_MAX);
        memset(rlogs, 0, sizeof (void*)*CANARY_THREADS_MAX);
        memset(lrlogs, 0, sizeof (void*)*CANARY_THREADS_MAX);

        // unzip
        int ret = system("if [ -f canary.zip ]; then unzip canary.zip; else exit 99; fi");
        if (WEXITSTATUS(ret) == 99) {
            printf("[ERROR] no logs can be detected!\n");
            exit(1);
        }

        {//flog
#ifdef DEBUG
            printf("loading flog...\n");
#endif
            FILE * fin = fopen("fork.dat", "r");
            flog.load(fin);
            fclose(fin);
        }

        {//mlog, llogs
#ifdef DEBUG
            printf("loading mlog, llog...\n");
#endif
            FILE * fin = fopen("mutex.dat", "r");
            mlog.load(fin);

            while (!feof(fin)) {
                g_llog_t * llog = new g_llog_t;
                llog->load(fin);
                llogs.push_back(llog);
            }
            fclose(fin);
        }
        {//rlog
#ifdef DEBUG
            printf("loading rlog...\n");
#endif
            FILE * fin = fopen("read.dat", "r");
            while(!feof(fin)){
                unsigned tid = -1;
                fread(&tid, sizeof (unsigned), 1, fin);
                l_rlog_t* rlog = new l_rlog_t[num_shared_vars];
                rlogs[tid] = rlog;
                
                for (unsigned i = 0; i < num_shared_vars; i++) {
                    rlog[i].VAL_LOG.load(fin);
                    rlog[i].VER_LOG.load(fin);
                }
            }
            
            fclose(fin);
        }
        {//lrlog
#ifdef DEBUG
            printf("loading lrlog...\n");
#endif
            FILE * fin = fopen("lread.dat", "r");
            while(!feof(fin)){
                unsigned tid = -1;
                fread(&tid, sizeof (unsigned), 1, fin);
                l_lrlog_t* rlog = new l_lrlog_t[num_local_vars];
                lrlogs[tid] = rlog;
                
                for (unsigned i = 0; i < num_local_vars; i++) {
                    rlog[i].load(fin);
                }
            }
            
            fclose(fin);
        }
        {//wlog
#ifdef DEBUG
            printf("loading wlog...\n");
#endif
            FILE * fin = fopen("write.dat", "r");
            while(!feof(fin)){
                unsigned tid = -1;
                fread(&tid, sizeof (unsigned), 1, fin);
                l_wlog_t* wlog = new l_wlog_t[num_shared_vars];
                wlogs[tid] = wlog;
                
                for (unsigned i = 0; i < num_shared_vars; i++) {
                    wlog[i].load(fin);
                }
            }
            
            fclose(fin);
        }
        
        {//address birthday map
#ifdef DEBUG
            printf("loading addressmap...\n");
#endif
            FILE * fin = fopen("addressmap.dat", "r");
            while(!feof(fin)){
                unsigned tid = -1;
                unsigned size = -1;
                
                fread(&size, sizeof (unsigned), 1, fin);
                fread(&tid, sizeof (unsigned), 1, fin);
                l_addmap_t * addmap = new l_addmap_t;
                addlogs[tid] = addmap;
                
                for (unsigned i = 0; i < size; i++) {
                    mem_t * m = new mem_t;
                    fread(m, sizeof(mem_t), 1, fin);
                    addmap->ADDRESS_LOG.push_back(m);
                }
                
                addmap->BIRTHDAY_LOG.load(fin);
            }
            
            fclose(fin);
        }
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
