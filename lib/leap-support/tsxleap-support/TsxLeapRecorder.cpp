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

#define RTM_ENABLED

#include "Lock.h"

typedef struct c_thread {
    pthread_t real_tid;
    int pseudo_tid;
    unsigned * LIDX;
    unsigned ** LLOG;
} c_thread_t;

#define MAX_LOG_LEN 50000 // the events of each sv in each thread
#define MAX_THREAD_NUM 50

#define LOCAL_THREAD_INIT
#undef LOCAL_THREAD_INIT

static c_thread_t** threads = NULL;
static bool start = false;
static int thread_idx = 0;

static unsigned *GIDX = NULL;

static int num_shared_vars = 0;

static struct timeval tpstart, tpend;

int static inline threadid(pthread_t tid) {
    for (int i = 0; i < thread_idx; i++) {
        if (threads[i]->real_tid == tid) {
            return threads[i]->pseudo_tid;
        }
    }

    return -1;
}

void static inline threadcreate(pthread_t tid) {
#ifdef LOCAL_THREAD_INIT
    c_thread_t* newthread = new c_thread_t;
    threads[thread_idx] = newthread;
    thread_idx++;

    newthread->LIDX = new unsigned[num_shared_vars];
    newthread->LLOG = new unsigned*[num_shared_vars];

    for (int i = 0; i < num_shared_vars; i++) {
        newthread->LLOG[i] = new unsigned[MAX_LOG_LEN];
        newthread->LIDX[i] = 0;
    }
#else
    c_thread_t* newthread = threads[thread_idx++];
#endif

    if (thread_idx > MAX_THREAD_NUM) {
        printf("Too many threads!\n");
        exit(0);
    }

    newthread->pseudo_tid = thread_idx; // start from 1
    newthread->real_tid = tid; // must be last, which indicate the c_thread_t is completely initialized
}

void static inline store(int svId, int tid, unsigned currentGIdx) {
    c_thread_t * currentT = threads[tid - 1];
    unsigned currentLIdx = currentT->LIDX[svId];
    unsigned * LLOG = currentT->LLOG[svId];

    if (currentLIdx > 0 && LLOG[currentLIdx - 1] + LLOG[currentLIdx - 2] == currentGIdx) {
        LLOG[currentLIdx - 1]++;
    } else {
        LLOG[currentLIdx] = currentGIdx;
        LLOG[currentLIdx + 1] = 1;

        currentT->LIDX[svId] = currentLIdx + 2;
    }

    //GIDX[svId]++; move out; should be in critical sections
}

extern "C" {

    void OnInit(int svsNum) {
        printf("OnInit-Record\n");
        initializeSigRoutine();

        //start = true;
        num_shared_vars = svsNum + 2; // one for fork, and the other is for synchronizations

        threads = new c_thread_t*[MAX_THREAD_NUM];
#ifndef LOCAL_THREAD_INIT
        for (int j = 0; j < MAX_THREAD_NUM; j++) {
            c_thread_t* newthread = new c_thread_t;
            threads[j] = newthread;

            newthread->LIDX = new unsigned[num_shared_vars];
            newthread->LLOG = new unsigned*[num_shared_vars];

            for (int i = 0; i < num_shared_vars; i++) {
                newthread->LLOG[i] = new unsigned[MAX_LOG_LEN];
                newthread->LIDX[i] = 0;
            }
        }
#endif
        initialize(num_shared_vars); // initialize locks

        GIDX = new unsigned[num_shared_vars];
        for (int i = 0; i < num_shared_vars; i++) {
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
        strcpy(sig->recorder, "tsxleap");

        FILE * fout = fopen("log.replay.dat", "wb");
        printf("OnExit-Record\n");

        fwrite(sig, sizeof (Sig), 1, fout);
        for (int i = 0; i < thread_idx; i++) {
            c_thread_t* current = threads[i];
            //fwrite(current, sizeof (c_thread_t), 1, fout);
            fwrite(current->LIDX, sizeof (unsigned), num_shared_vars, fout);
            for (int j = 0; j < num_shared_vars; j++) {
                fwrite(current->LLOG[j], sizeof (unsigned), current->LIDX[j], fout);
            }
        }

        fclose(fout);

#ifdef DEBUG
        // debug out
        FILE* fdebug = fopen("log.debug", "w+");

        for (int i = 0; i < thread_idx; i++) {
            c_thread_t* current = threads[i];
            fprintf(fdebug, "T%d: \n", current->pseudo_tid);
            fprintf(fdebug, "LIDX: \n");
            for (int j = 0; j < num_shared_vars; j++) {
                fprintf(fdebug, "%d, ", current->LIDX[j]);
            }
            fprintf(fdebug, "\nLLOG: \n");

            for (int j = 0; j < num_shared_vars; j++) {
                for (int k = 0; k < current->LIDX[j]; k++) {
                    fprintf(fdebug, "%d, ", current->LLOG[j][k]);
                }
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

        lock(svId);

#ifdef DEBUG
        printf("OnPreLoad\n");
#endif

    }

    void OnLoad(int svId, int debug) {
        if (!start) {
            return;
        }
        unsigned tmp = GIDX[svId];
        GIDX[svId]++;
        unlock(svId);

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(svId, _tid, tmp);

#ifdef DEBUG
        printf("OnLoad: %d at t%d [%d]\n", svId, _tid, debug);
#endif
    }

    void OnPreStore(int svId, int debug) {
        if (!start) {
            return;
        }

        lock(svId);
#ifdef DEBUG        
        printf("OnPreStore\n");
#endif
    }

    void OnStore(int svId, int debug) {
        if (!start) {
            return;
        }
        unsigned tmp = GIDX[svId];
        GIDX[svId]++;
        unlock(svId);

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(svId, _tid, tmp);

#ifdef DEBUG        
        printf("OnStore: %d at t%d [%d]\n", svId, _tid, debug);
#endif
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

        unsigned tmp = GIDX[num_shared_vars - 2];
        GIDX[num_shared_vars - 2]++;

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(num_shared_vars - 2, _tid, tmp);
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
        unsigned tmp = GIDX[num_shared_vars - 1];
        GIDX[num_shared_vars - 1]++;

        forkunlock(num_shared_vars - 1);

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(num_shared_vars - 1, _tid, tmp);
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
        unsigned tmp = GIDX[num_shared_vars - 2];
        GIDX[num_shared_vars - 2]++;

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(num_shared_vars - 2, _tid, tmp);
    }

    void OnWait(int condId, long cond_ptr, long mutex_ptr) {
        if (!start) {
            return;
        }
#ifdef DEBUG
        printf("OnWait\n");
#endif
        unsigned tmp = GIDX[num_shared_vars - 2];
        GIDX[num_shared_vars - 2]++;

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(num_shared_vars - 2, _tid, tmp);
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
        unsigned tmp = GIDX[num_shared_vars - 2];
        GIDX[num_shared_vars - 2]++;

        unlock(num_shared_vars - 2);

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        store(num_shared_vars - 2, _tid, tmp);
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
