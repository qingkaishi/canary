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

#ifdef NO_TIME_CMD
static struct timeval tpstart, tpend;
#endif

bool main_started = false;

extern "C" {

    void* OnMethodStart(pthread_t self) {
        return NULL;
    }

    void OnStartTimer() {
#ifdef NO_TIME_CMD
        gettimeofday(&tpstart, NULL);
#endif
    }

    void OnInit(unsigned svsNum, unsigned lvsNum) {
        printf("OnInit-Record (dummy record)\n");

        main_started = true;
    }

    void OnExit() {

#ifdef NO_TIME_CMD
        gettimeofday(&tpend, NULL);
        double timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
        timeuse /= 1000;
        printf("processor time is %lf ms\n", timeuse);
#endif
        printf("OnExit-Record (dummy record)\n");
    }

    void OnAddressInit(void* value, size_t size, size_t n, int type, void* st) {
#ifdef DEBUG
        printf("OnAddressInit \n");
#endif
    }

    void OnLoad(int svId, int lvId, long value, void* st, int debug) {
#ifdef DEBUG
        printf("OnLoad %d\n", debug);
#endif
    }

    unsigned OnPreStore(int svId, void* st, int debug) {
#ifdef DEBUG
        printf("OnPreStore %d\n", debug);
#endif
        return 0;
    }

    void OnStore(int svId, unsigned version, void* st, int debug) {
#ifdef DEBUG
        printf("OnStore %d\n", debug);
#endif
    }

    void OnLock(pthread_mutex_t* mutex_ptr, void* st) {
#ifdef DEBUG
        printf("OnLock \n");
#endif
    }

    void OnWait(pthread_cond_t* cond_ptr, pthread_mutex_t* mutex_ptr, void* st) {
#ifdef DEBUG
        printf("OnWait \n");
#endif
    }

    void OnPreMutexInit(bool race, void* st) {
#ifdef DEBUG
        printf("OnPreMutexInit \n");
#endif
    }

    void OnMutexInit(pthread_mutex_t* mutex_ptr, bool race, void* st) {
#ifdef DEBUG
        printf("OnMutexInit \n");
#endif
    }

    void OnPreFork(bool race, void* st) {
#ifdef DEBUG
        printf("OnPreFork \n");
#endif
    }

    void OnFork(pthread_t* forked_tid_ptr, bool race, void* st) {
#ifdef DEBUG
        printf("OnFork \n");
#endif
    }

    void OnPreExternalCall(void* st) {
        if (!main_started)
            return;
#ifdef DEBUG
        printf("OnPreExternalCall \n");
#endif
    }

    void OnExternalCall(long value, int lvid, void* st) {
        if (!main_started)
            return;
#ifdef DEBUG
        printf("OnExternalCall \n");
#endif
    }

}
