#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

#include "SignalRoutine.h"

#ifdef NO_TIME_CMD
static struct timeval tpstart, tpend;
#endif

bool start = false;

extern "C" {

    void OnInit(unsigned svsNum) {
        printf("OnInit-Record\n");

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
    }

    void OnAddressInit(void* value, size_t size, size_t n) {
        if (!start) {
            return;
        }

    }

    void OnLoad(int svId, long address, long value, int debug) {
        if (!start) {
            return;
        }
    }

    unsigned OnPreStore(int svId, int debug) {
        if (!start) {
            return 0;
        }

        return 0;
    }

    void OnStore(int svId, unsigned version, long address, long value, int debug) {
        if (!start) {
            return;
        }
    }

    void OnLock(pthread_mutex_t* mutex_ptr) {
        if (!start) {
            return;
        }
    }

    void OnWait(pthread_cond_t* cond_ptr, pthread_mutex_t* mutex_ptr) {
        if (!start) {
            return;
        }
    }

    void OnPreMutexInit(bool race) {
        if (start && race) {
        }
    }

    void OnMutexInit(pthread_mutex_t* mutex_ptr, bool race) {
    }

    void OnPreFork(bool race) {
        if (!start) {
            start = true;
        }
    }

    void OnFork(pthread_t* forked_tid_ptr, bool race) {
        if (!start) {
            return;
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
