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

#define POSIX_MUTEX
#define DEBUG
#include "Lock.h"

#define MAX_LOG_LEN 50000
#define MAX_THREAD_NUM 50

static bool start = false;
static pthread_t thread_idx_map[MAX_THREAD_NUM];
static int thread_idx = 1; //start from 1, make 0 be a terminal

static unsigned **GLOG = NULL;
static unsigned *GIDX = NULL;

static int num_shared_vars = 0;

static struct timeval tpstart, tpend;

int static inline threadid(pthread_t tid) {
    for (int i = 1; i < thread_idx; i++) {
        if (thread_idx_map[i] == tid) {
            return i;
        }
    }

    return -1;
}

void static inline threadcreate(pthread_t tid) {
    thread_idx_map[thread_idx++] = tid;
    
    if (thread_idx > MAX_THREAD_NUM) {
        printf("Too many threads!\n");
        exit(0);
    }
}

void static inline store(int svId, int tid) {
    int currentIdx = GIDX[svId];
    if (currentIdx > 0 && (int) (GLOG[svId][currentIdx - 2]) == tid) {
        GLOG[svId][currentIdx - 1]++;
    } else {
        GLOG[svId][currentIdx] = tid;
        GLOG[svId][currentIdx + 1] = 1;
        GIDX[svId] += 2;
    }
}

extern "C" {

    void OnInit(int svsNum) {
        printf("OnInit-Record\n");
        initializeSigRoutine();

        //start = true;
        num_shared_vars = svsNum + 2; // one for fork, and the other is for synchronizations

        for (int i = 0; i < MAX_THREAD_NUM; i++) {
            thread_idx_map[i] = 0;
        }

        initialize(num_shared_vars); // initialize locks

        GLOG = new unsigned*[num_shared_vars];
        GIDX = new unsigned[num_shared_vars];
        for (int i = 0; i < num_shared_vars; i++) {
            GLOG[i] = new unsigned[MAX_LOG_LEN];
            GIDX[i] = 0;
        }

        // main thread.
        pthread_t tid = pthread_self();
        if (threadid(tid) == -1) {
            threadcreate(tid);
        }

        gettimeofday(&tpstart, NULL);
    }

    void OnExit(int nouse) {
        start = false;

        gettimeofday(&tpend, NULL);
        double timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
        timeuse /= 1000;
        printf("processor time is %lf ms\n", timeuse);

        Sig* sig = new Sig;
        strcpy(sig->recorder, "leap");

        FILE * fout = fopen("log.replay.dat", "wb");
        printf("OnExit-Record\n");

        fwrite(sig, sizeof (Sig), 1, fout);
        fwrite(GIDX, sizeof (unsigned), num_shared_vars, fout);
        for (int i = 0; i < num_shared_vars; i++) {
            fwrite(GLOG[i], sizeof (unsigned), GIDX[i], fout);
        }
        fclose(fout);

#ifdef DEBUG
        // debug out
        FILE* fdebug = fopen("log.debug", "w+");
        for (int i = 0; i < num_shared_vars; i++) {
            fprintf(fdebug, "%d, ", GIDX[i]);
        }
        fprintf(fdebug, "\n\n");
        for (int i = 0; i < num_shared_vars; i++) {
            for (unsigned j = 0; j < GIDX[i]; j++) {
                fprintf(fdebug, "%d, ", GLOG[i][j]);
            }
            fprintf(fdebug, "\n");
        }
        fclose(fdebug);
#endif
    }

    void OnPreLoad(int svId, int debug) {
        if (!start) {
            return;
        }

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);

        lock(svId);

#ifdef DEBUG
        printf("OnPreLoad: %d at t%d [%d]\n", svId, _tid, debug);
#endif
        store(svId, _tid);
    }

    void OnLoad(int svId, int debug) {
        if (!start) {
            return;
        }
#ifdef DEBUG
        printf("OnLoad\n");
#endif
        unlock(svId);
    }

    void OnPreStore(int svId, int debug) {
        if (!start) {
            return;
        }

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);

        lock(svId);
#ifdef DEBUG        
        printf("OnPreStore: %d at t%d [%d]\n", svId, _tid, debug);
#endif
        store(svId, _tid);
    }

    void OnStore(int svId, int debug) {
        if (!start) {
            return;
        }
#ifdef DEBUG
        printf("OnStore\n");
#endif
        unlock(svId);
    }

    void OnPreLock(int nouse) {
#ifdef DEBUG
        if (!start) {
            return;
        }
        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        printf("OnPreLock[%d]\n", _tid);
#endif
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

    void OnPreUnlock(int nouse) {
#ifdef DEBUG
        if (!start) {
            return;
        }
        printf("OnpreunLock\n");
#endif
    }

    void OnUnlock(int nouse) {
#ifdef DEBUG
        if (!start) {
            return;
        }
        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        printf("OnunLock <-- t%d\n", _tid);
#endif
    }

    void OnPreFork(int nouse) {
        if (!start) {
            start = true;
            //return;
        }
#ifdef DEBUG
        printf("OnPreFork\n");
#endif
        forklock(num_shared_vars - 1);
    }

    void OnFork(long forked_tid_ptr) {
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

    void OnPreJoin(int id) {
    }

    void OnJoin(int id) {
    }

    void OnPreWait(int condId) {
        if (!start) {
            return;
        }
#ifdef DEBUG        
        printf("OnPrewait\n");
#endif
        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(num_shared_vars - 2, _tid);
    }

    void OnWait(int condId, long cond_ptr, long mutex_ptr) {
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

    void OnPreNotify(int condId) {
        if (!start) {
            return;
        }
#ifdef DEBUG
        printf("OnPreNotify\n");
#endif
        lock(num_shared_vars - 2);
    }

    void OnNotify(int condId) {
        if (!start) {
            return;
        }
#ifdef DEBUG
        printf("OnNotify\n");
#endif
        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(num_shared_vars - 2, _tid);
        unlock(num_shared_vars - 2);
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
