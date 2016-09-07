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

#define POSIX_MUTEX
#define DEBUG
#include "LeapSupport/Lock.h"
#include "LeapSupport/Signature.h"


#define MAX_THREAD_NUM 200

static bool start = false;
static pthread_t thread_idx_map[MAX_THREAD_NUM];
static int thread_idx = 1; //start from 1, make 0 be a terminal

static unsigned **GLOG = NULL;
static unsigned *GIDX = NULL;

static int num_shared_vars = 0;

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
}

void static inline load(int svId, int tid) {
#ifdef DEBUG
    int count = 0;
#endif
    int currentIdx = GIDX[svId];
    while (GLOG[svId][currentIdx] != (unsigned) tid) {
#ifdef DEBUG
        if (count++ < 2) {
            fprintf(stdout, "#### %d, t%d is waiting for t%d\n", svId, tid, GLOG[svId][currentIdx]);
            fflush(stdout);
        }
#endif
        wait(svId);
        currentIdx = GIDX[svId];
    }

    if (GLOG[svId][currentIdx] == (unsigned) tid && GLOG[svId][currentIdx + 1] > 0) {
        // do the event
        GLOG[svId][currentIdx + 1]--;
        if (GLOG[svId][currentIdx + 1] == 0) {
            GIDX[svId] += 2;
        }
    } else {
        //error
        printf("ERROR when replay \n\n");
        exit(-1);
    }
}

static void sigroutine(int dunno);

extern "C" {

    void OnInit(int svsNum) {
        printf("OnInit-Replayer\n");
        signal(SIGHUP, sigroutine);
        signal(SIGINT, sigroutine);
        signal(SIGQUIT, sigroutine);
        signal(SIGKILL, sigroutine);

        //start = true;
        num_shared_vars = svsNum + 2;

        for (int i = 0; i < MAX_THREAD_NUM; i++) {
            thread_idx_map[i] = 0;
        }

        initialize(num_shared_vars); // init locks

        GIDX = new unsigned[num_shared_vars];
        GLOG = new unsigned*[num_shared_vars];

        for (int i = 0; i < num_shared_vars; i++) {
            GIDX[i] = 0;
        }

        FILE* fin = fopen("log.replay.dat", "rb");
        if (fin == NULL) {
            printf("Cannot find log file: log.replay.dat!\n");
            kill(getpid(), SIGINT);
        }

        Sig* sig = new Sig;
        fread(sig, sizeof (Sig), 1, fin);
        if (strcmp(sig->recorder, "leap") == 0) {
            fread(GIDX, sizeof (unsigned), num_shared_vars, fin);
            for (int i = 0; i < num_shared_vars; i++) {
                GLOG[i] = new unsigned[GIDX[i]];
                fread(GLOG[i], sizeof (unsigned), GIDX[i], fin);
            }
        } else if (strcmp(sig->recorder, "tsxleap") == 0) {
            // read data
            int TIDX = 0;
            unsigned ** LIDX = new unsigned*[MAX_THREAD_NUM];
            unsigned *** LLOG = new unsigned**[MAX_THREAD_NUM];
            while (!feof(fin)) {
                LIDX[TIDX] = new unsigned[num_shared_vars];
                int count = fread(LIDX[TIDX], sizeof (unsigned), num_shared_vars, fin);
                if(count != num_shared_vars){
                    delete [] LIDX[TIDX];
                    break;
                }

                LLOG[TIDX] = new unsigned*[num_shared_vars];
                for (int i = 0; i < num_shared_vars; i++) {
                    LLOG[TIDX][i] = new unsigned[LIDX[TIDX][i]];
                    fread(LLOG[TIDX][i], sizeof (unsigned), LIDX[TIDX][i], fin);
                    GIDX[i] = GIDX[i] + LIDX[TIDX][i];
                }

                TIDX++;
            }
#ifdef DEBUG
            FILE* fdebug = fopen("log3.debug", "w+");

            for (int i = 0; i < TIDX; i++) {
                fprintf(fdebug, "T%d: \n", i+1);
                fprintf(fdebug, "LIDX: \n");
                for (int j = 0; j < num_shared_vars; j++) {
                    fprintf(fdebug, "%d, ", LIDX[i][j]);
                }
                fprintf(fdebug, "\nLLOG: \n");

                for (int j = 0; j < num_shared_vars; j++) {
                    for (unsigned k = 0; k < LIDX[i][j]; k++) {
                        fprintf(fdebug, "%d, ", LLOG[i][j][k]);
                    }
                }
                fprintf(fdebug, "\n");
            }

            fclose(fdebug);
#endif

            // construct GLOG
            for (int i = 0; i < num_shared_vars; i++) {
                if(GIDX[i] == 0) continue;
                
                unsigned * glog = new unsigned[GIDX[i]];
                GLOG[i] = glog;
                int glogIdx = 0;
                for (int t = 0; t < TIDX; t++) {
                    unsigned * llog = LLOG[t][i];
                    int num = LIDX[t][i]/2;
                    for(int j = 0; j < num; j++){
                        // insert llog[2*j] and llog[2*j+1] into GLOG[i]
                        // find the pos
                        int pos = 0;
                        for(; pos<glogIdx; pos+=2){
                            if(glog[pos] > llog[2*j]){
                                break;
                            }
                        }
                        
                        // elmts from pos to GIDX[i]
                        for(int k = glogIdx - 1; k>=pos; k-- ){
                            glog[k+2] = glog[k];
                        }
                        
                        // insert
                        glog[pos] = llog[2*j];
                        glog[pos+1] = llog[2*j+1];
                        
                        // increase the length
                        glogIdx+=2;
                        
                        if(((unsigned) glogIdx) > GIDX[i]){
                            printf("ERROR when construct GLOG - ERR1\n");
                            exit(1);
                        }
                    }
                }
                // replace each lidx with a thread id;
                for(int k = 0; k<glogIdx/2; k++){
                    // lidx = glog[k*2];
                    bool found = false;
                    for(int t = 0; t< TIDX; t++){
                        unsigned * llog = LLOG[t][i];
                        int num = LIDX[t][i]/2;
                        for(int j = 0; j < num; j++){
                            if(llog[j*2] == glog[k*2]){
                                glog[k*2] = t + 1; // tid from 1
                                found = true;
                                break;
                            }
                        }
                        if(found)
                            break;
                    }
                    if(!found){
                        printf("ERROR when construct GLOG - ERR2\n");
                        exit(1);
                    }
                }
            }
            // release memory
            for (int i = 0; i < TIDX; i++) {
                for (int j = 0; j < num_shared_vars; j++) {
                    delete[] LLOG[i][j];
                }
                delete[] LLOG[i];
            }
            delete [] LLOG;

            for (int i = 0; i < TIDX; i++) {
                delete [] LIDX[i];
            }
            delete [] LIDX;
        } else {
            printf("Bad signature!\n");
            fclose(fin);
            kill(getpid(), SIGINT);
        }
        fclose(fin);

#ifdef DEBUG
            // debug out
            FILE* fdebug = fopen("log2.debug", "w+");
            for (int i = 0; i < num_shared_vars; i++) {
                fprintf(fdebug, "%d, ", GIDX[i]);
            }
            fprintf(fdebug, "\n\n");
            fflush(fdebug);
            for (int i = 0; i < num_shared_vars; i++) {
                for (unsigned j = 0; j < GIDX[i]; j++) {
                    fprintf(fdebug, "%d, ", GLOG[i][j]);
                }
                fprintf(fdebug, "\n");
            }
            fclose(fdebug);
#endif        

        for (int i = 0; i < num_shared_vars; i++) {
            GIDX[i] = 0;
        }
        // main thread.
        pthread_t tid = pthread_self();
        if (threadid(tid) == -1) {
            threadcreate(tid);
        }
    }

    void OnExit(int nouse) {
        printf("OnExit-Replayer\n");
        start = false;
        exit(0);
    }

    void OnPreLoad(int svId, int debug) {
        if (!start) {
            return;
        }
        lock(svId);
        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);

        load(svId, _tid);
#ifdef DEBUG
        printf("OnPreLoad: %d at t%d [%d]\n", svId, _tid, debug);
        printf("OnLoad\n");
#endif
    }

    void OnLoad(int svId, int debug) {
        if (!start) {
            return;
        }

        unlock(svId);
    }

    void OnPreStore(int svId, int debug) {
        if (!start) {
            return;
        }
        lock(svId);
        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        load(svId, _tid);

#ifdef DEBUG        
        printf("OnPreStore: %d at t%d [%d]\n", svId, _tid, debug);
        printf("OnStore\n");
#endif

    }

    void OnStore(int svId, int debug) {
        if (!start) {
            return;
        }
        unlock(svId);
    }

    void OnPreLock(int nouse) {
        if (!start) {
            return;
        }

        lock(num_shared_vars - 2);

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);

        load(num_shared_vars - 2, _tid);
#ifdef DEBUG
        printf("OnPreLock[%d]\n", _tid);
#endif
    }

    void OnLock(int nouse) {
        if (!start) {
            return;
        }

#ifdef DEBUG
        int _tid = threadid(pthread_self());
        printf("OnLock --> t%d\n", _tid);
#endif

        unlock(num_shared_vars - 2);
    }

    void OnPreUnlock(int svId) {
#ifdef DEBUG
        if (!start) {
            return;
        }
        printf("OnpreunLock\n");
#endif
    }

    void OnUnlock(int svId) {
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
        lock(num_shared_vars - 1);

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);
        load(num_shared_vars - 1, _tid);
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

        unlock(num_shared_vars - 1);
    }

    void OnPreJoin(int id) {
    }

    void OnJoin(int id) {
    }

    void OnPreNotify(int condId) {
        if (!start) {
            return;
        }

        lock(num_shared_vars - 2);
        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);

        load(num_shared_vars - 2, _tid);
#ifdef DEBUG
        printf("OnPreNotify\n");
        printf("OnNotify\n");
#endif
    }

    void OnNotify(int condId) {
        if (!start) {
            return;
        }

        unlock(num_shared_vars - 2);
    }

    /// need mutex and condition.
    /// if it is waken early, need to wait on the mutex and condition again.
    /// meanwhile a notify should be invoked so that another thread can be waken up.

    void OnPreWait(int condId) {
        //printf("OnPrewait\n");
        if (!start) {
            return;
        }

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);

#ifdef DEBUG        
        printf("OnPrewait\n");
#endif

        lock(num_shared_vars - 2);
        load(num_shared_vars - 2, _tid);
        unlock(num_shared_vars - 2);


    }

    void OnWait(int condId, long cond_ptr, long mutex_ptr) {
        if (!start) {
            return;
        }

        pthread_t tid = pthread_self();
        int _tid = -1;
        do {
            _tid = threadid(tid);
        } while (_tid < 0);

        int currentIdx = GIDX[num_shared_vars - 2];
        while (GLOG[num_shared_vars - 2][currentIdx] != (unsigned)_tid) {
            //wait;
            //printf("#### %d\n", num_shared_vars - 2);
            struct timespec tv;
            tv.tv_sec = time(0);
            tv.tv_nsec = 50000000; //50ms
            pthread_cond_signal((pthread_cond_t*) cond_ptr);
            pthread_cond_wait((pthread_cond_t*) cond_ptr, (pthread_mutex_t*) mutex_ptr);
            currentIdx = GIDX[num_shared_vars - 2];
        }

        if (GLOG[num_shared_vars - 2][currentIdx] == (unsigned)_tid && GLOG[num_shared_vars - 2][currentIdx + 1] > 0) {
            // do the event
            GLOG[num_shared_vars - 2][currentIdx + 1]--;
            if (GLOG[num_shared_vars - 2][currentIdx + 1] == 0) {
                GIDX[num_shared_vars - 2] += 2;
            }
        } else {
            //error
            printf("ERROR when replay \n\n");
            exit(-1);
        }

#ifdef DEBUG
        printf("OnWait\n");
#endif
    }
}

/* ************************************************************************
 * Signal Process
 * ************************************************************************/

void sigroutine(int dunno) {
    switch (dunno) {
        case SIGHUP:
            printf("\nGet a signal -- SIGHUP\n");
            break;
        case SIGINT:
            printf("\nGet a signal -- SIGINT\n");
            break;
        case SIGQUIT:
            printf("\nGet a signal -- SIGQUIT\n");
            break;
        case SIGKILL:
            printf("\nGet a signal -- SIGKILL\n");
            break;
    }

    OnExit(num_shared_vars);
    exit(dunno);
}
