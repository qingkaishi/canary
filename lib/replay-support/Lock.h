#include <pthread.h>
#include <stdio.h>

#define MAXNUMLOCKS 100

inline void initialize(int lock_num);

inline void lock(int idx);

inline void unlock(int idx);

inline void wait(int idx);

inline void forklock(int idx);

inline void forkunlock(int idx);

#ifdef HLE_ENABLED
#include <immintrin.h>

int * LOCKS;
pthread_mutex_t forkmutex;

inline void initialize(int lock_num) {
    if (lock_num > MAXNUMLOCKS) {
        printf("Too many locks!\n");
        exit(1);
    }
    
    LOCKS = new int[lock_num];
    for (int i = 0; i < lock_num; i++) {
        LOCKS[i] = 0;
    }
    pthread_mutex_init(&forkmutex, NULL);
}

inline void lock(int idx) {
    while (__atomic_exchange_n(&LOCKS[idx], 1, __ATOMIC_ACQUIRE | __ATOMIC_HLE_ACQUIRE) != 0) {
        int val = 0;
        /* Wait for lock to become free again before retrying. */
        do {
            _mm_pause(); /* Abort speculation */
            __atomic_load(&LOCKS[idx], &val, __ATOMIC_CONSUME);
        } while (val == 1);
    }
}

inline void unlock(int idx) {
    __atomic_clear(&LOCKS[idx], __ATOMIC_RELEASE | __ATOMIC_HLE_RELEASE);
}

inline void forklock(int idx) {
    pthread_mutex_lock(&forkmutex);
}

inline void forkunlock(int idx) {
    pthread_mutex_unlock(&forkmutex);
}

inline void wait(int idx) {
    printf("inline void wait(int): Not Supported!\n");
    exit(1);
}

#endif

#ifdef EXCH_ENABLED
#include <immintrin.h>

int * LOCKS;
pthread_mutex_t forkmutex;

inline void initialize(int lock_num) {
    if (lock_num > MAXNUMLOCKS) {
        printf("Too many locks!\n");
        exit(1);
    }
    
    LOCKS = new int[lock_num];
    for (int i = 0; i < lock_num; i++) {
        LOCKS[i] = 0;
    }
    pthread_mutex_init(&forkmutex, NULL);
}

inline void lock(int idx) {
    while (__atomic_exchange_n(&LOCKS[idx], 1, __ATOMIC_ACQUIRE) != 0) {
        int val = 0;
        /* Wait for lock to become free again before retrying. */
        do {
            _mm_pause(); /* Abort speculation */
            __atomic_load(&LOCKS[idx], &val, __ATOMIC_CONSUME);
        } while (val == 1);
    }
}

inline void unlock(int idx) {
    __atomic_clear(&LOCKS[idx], __ATOMIC_RELEASE);
}

inline void forklock(int idx) {
    pthread_mutex_lock(&forkmutex);
}

inline void forkunlock(int idx) {
    pthread_mutex_unlock(&forkmutex);
}

inline void wait(int idx) {
    printf("inline void wait(int): Not Supported!\n");
    exit(1);
}

#endif

#ifdef STM_ENABLED
pthread_mutex_t forkmutex;

inline void initialize(int lock_num) {
    if (lock_num > MAXNUMLOCKS) {
        printf("Too many locks!\n");
        exit(1);
    }
    pthread_mutex_init(&forkmutex, NULL);
}

inline void lock(int idx) {
    // gcc stm
    // gcc -fgnu-tm
    // __transaction_atomic{}
    // __transaction_relaxed{}
    printf("inline void lock(int): Not Supported!\n");
    exit(1);
}

inline void unlock(int idx) {
    printf("inline void unlock(int): Not Supported!\n");
    exit(1);
}

inline void wait(int idx) {
    printf("inline void wait(int): Not Supported!\n");
    exit(1);
}

inline void forklock(int idx) {
    pthread_mutex_lock(&forkmutex);
}

inline void forkunlock(int idx) {
    pthread_mutex_unlock(&forkmutex);
}
#endif

#ifdef POSIX_MUTEX
#include <sys/time.h>


pthread_mutex_t LOCKS[MAXNUMLOCKS];
pthread_cond_t conditions[MAXNUMLOCKS];

inline void initialize(int lock_num) {
    if (lock_num > MAXNUMLOCKS) {
        printf("Too many locks!\n");
        exit(1);
    }

    for (int i = 0; i < lock_num; i++) {
        pthread_mutex_init(&LOCKS[i], NULL);
    }
}

inline void lock(int idx) {
    pthread_mutex_lock(&LOCKS[idx]);
}

inline void unlock(int idx) {
    pthread_mutex_unlock(&LOCKS[idx]);
}

inline void wait(int idx) {
    struct timespec tv;
    tv.tv_sec = time(0);
    tv.tv_nsec = 100000000; //100ms
    pthread_cond_timedwait(&conditions[idx], &LOCKS[idx], &tv);
}

inline void forklock(int idx) {
    lock(idx);
}

inline void forkunlock(int idx) {
    unlock(idx);
}
#endif

#ifdef RTM_ENABLED
#include <immintrin.h>

#define _XBEGIN_STARTED		(~0u)
#define _XABORT_EXPLICIT	(1 << 0)
#define _XABORT_RETRY		(1 << 1)
#define _XABORT_CONFLICT	(1 << 2)
#define _XABORT_CAPACITY	(1 << 3)
#define _XABORT_DEBUG		(1 << 4)
#define _XABORT_NESTED		(1 << 5)
#ifndef _XABORT_CODE
#define _XABORT_CODE(x)		(((x) >> 24) & 0xff)
#endif

#define _ABORT_LOCK_BUSY 	0xff
#define _ABORT_LOCK_IS_LOCKED	0xfe
#define _ABORT_NESTED_TRYLOCK	0xfd

typedef struct {
    pthread_mutex_t posix_mutex;
    short elision;
} rtm_enabled_mutex;

struct elision_config {
    int skip_lock_busy;
    int skip_lock_internal_abort;
    int retry_try_xbegin;
    int skip_trylock_internal_abort;
} aconf;

rtm_enabled_mutex LOCKS[MAXNUMLOCKS];

inline void initialize(int lock_num) {
    if (lock_num > MAXNUMLOCKS) {
        printf("Too many locks!\n");
        exit(1);
    }

    aconf.skip_lock_busy = 3;
    aconf.skip_lock_internal_abort = 3;
    aconf.retry_try_xbegin = 3;
    aconf.skip_trylock_internal_abort = 3;

    for (int i = 0; i < lock_num; i++) {
        pthread_mutex_init(&(LOCKS[i].posix_mutex), NULL);
        LOCKS[i].elision = 0;
    }
}

inline void lock(int idx) {
    if (LOCKS[idx].elision <= 0) {
        unsigned status;
        int try_xbegin;

        for (try_xbegin = aconf.retry_try_xbegin;
                try_xbegin > 0;
                try_xbegin--) {
            if ((status = _xbegin()) == _XBEGIN_STARTED) {
                if (LOCKS[idx].posix_mutex.__data.__lock == 0)
                    return ;

                _xabort(_ABORT_LOCK_BUSY);
            }

            if (!(status & _XABORT_RETRY)) {
                if ((status & _XABORT_EXPLICIT)
                        && _XABORT_CODE(status) == _ABORT_LOCK_BUSY) {
                    /* Right now we skip here.  Better would be to wait a bit
                       and retry.  This likely needs some spinning.  */
                    if (LOCKS[idx].elision != aconf.skip_lock_busy)
                        LOCKS[idx].elision = aconf.skip_lock_busy;
                }
                    /* Internal abort.  There is no chance for retry.
                     Use the normal locking and next time use lock.
                     Be careful to avoid writing to the lock.  */
                else if (LOCKS[idx].elision != aconf.skip_lock_internal_abort)
                    LOCKS[idx].elision = aconf.skip_lock_internal_abort;
                break;
            }
        }
    } else {
        LOCKS[idx].elision--;
    }

    pthread_mutex_lock(&(LOCKS[idx].posix_mutex));
}

inline void unlock(int idx) {
    /* If lock is free, assume that the lock was elided */
    if (LOCKS[idx].posix_mutex.__data.__lock == 0)
        _xend(); /* commit */
    else {
        pthread_mutex_unlock(&(LOCKS[idx].posix_mutex));
    }
}

inline void wait(int idx) {
    printf("inline void wait(int): Not Supported!\n");
    exit(1);
}

inline void forklock(int idx) {
    pthread_mutex_lock(&(LOCKS[idx].posix_mutex));
}

inline void forkunlock(int idx) {
    pthread_mutex_unlock(&(LOCKS[idx].posix_mutex));
}
#endif

