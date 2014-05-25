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

extern "C" {

    void OnInit(unsigned svsNum) {
        printf("OnInit-Record (dummy record)\n");

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
