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

    void OnInit(unsigned svsNum, unsigned lvsNum) {
        printf("OnInit-Record (dummy record)\n");
        
        main_started = true;

#ifdef NO_TIME_CMD
        gettimeofday(&tpstart, NULL);
#endif
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

    void OnAddressInit(void* value, size_t size, size_t n, int type) {
#ifdef DEBUG
        printf("OnAddressInit \n");
#endif
    }

    void OnLoad(int svId, int address, long value, int debug) {
#ifdef DEBUG
        printf("OnLoad %d\n", debug);
#endif
    }

    unsigned OnPreStore(int svId, int debug) {
#ifdef DEBUG
        printf("OnPreStore %d\n", debug);
#endif
        return 0;
    }

    void OnStore(int svId, unsigned version, int debug) {
#ifdef DEBUG
        printf("OnStore %d\n", debug);
#endif
    }

    void OnLock(pthread_mutex_t* mutex_ptr) {
#ifdef DEBUG
        printf("OnLock \n");
#endif
    }

    void OnWait(pthread_cond_t* cond_ptr, pthread_mutex_t* mutex_ptr) {
#ifdef DEBUG
        printf("OnWait \n");
#endif
    }

    unsigned OnPreMutexInit(bool race) {
#ifdef DEBUG
        printf("OnPreMutexInit \n");
#endif
        return 0;
    }

    void OnMutexInit(pthread_mutex_t* mutex_ptr, bool race, unsigned) {
#ifdef DEBUG
        printf("OnMutexInit \n");
#endif
    }

    unsigned OnPreFork(bool race) {
#ifdef DEBUG
        printf("OnPreFork \n");
#endif
        return 0;
    }

    void OnFork(pthread_t* forked_tid_ptr, bool race, unsigned) {
#ifdef DEBUG
        printf("OnFork \n");
#endif
    }

    void OnJoin(pthread_t tid, bool race) {
#ifdef DEBUG
        printf("OnJoin \n");
#endif
    }

    void OnLocal(long value, int id) {
#ifdef DEBUG
        printf("OnLoad \n");
#endif
    }

    unsigned OnPreExternalCall() {
        if(!main_started)
            return 0;
#ifdef DEBUG
        printf("OnPreExternalCall \n");
#endif
        return 0;
    }

    void OnExternalCall(unsigned tid, long value, int lvid) {
        if(!main_started)
            return;
#ifdef DEBUG
        printf("OnExternalCall \n");
#endif
    }

}
