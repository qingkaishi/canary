#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <set>

#include <boost/unordered_map.hpp>

#include "SignalRoutine.h"
#include "../Log.h"
#include "../Cache.h"

/*
 * Define log types
 * l_ means local log types, each thread has a log
 * g_ means global log types, each shared variable has a log
 * r  -> read 
 * lr -> local read
 * w  -> write
 * l  -> lock
 * m  -> mutex init
 * f  -> fork
 */
typedef struct {
    // <long/void *> sizeof pointer usually equals to sizeof long
    LastOnePredictorLog<long> VAL_LOG;
    // <int>, -1 is the place holder, means the read operation reads
    // the local value
    LastOnePredictorLog<int> VER_LOG;
} l_rlog_t;

typedef VLastOnePredictorLog l_wlog_t;

typedef LastOnePredictorLog<long> l_lrlog_t;

typedef LastOnePredictorLog<pthread_t> g_llog_t;

typedef LastOnePredictorLog<pthread_t> g_mlog_t;

typedef LastOnePredictorLog<pthread_t> g_flog_t;

typedef struct {
    void* address;
    size_t range;
} mem_t;

typedef struct {
    std::vector<mem_t *> ADDRESS_LOG;
    VLastOnePredictorLog BIRTHDAY_LOG;
} l_addmap_t;

/*
 * Define local log keys
 */
static pthread_key_t lrlog_key;
static pthread_key_t rlog_key;
static pthread_key_t wlog_key;
static pthread_key_t cache_key;
static pthread_key_t birthday_key;
static pthread_key_t address_birthday_map_key;

/*
 * Define global log variables, each shared var has one
 */
static std::vector<g_llog_t *> llogs; // <g_llog_t *> size is not fixed, and should be determined at runtime
static g_flog_t flog;
static g_mlog_t mlog;

/*
 * write versions, each shared var has a version
 */
static unsigned* write_versions;

/*
 * locks for synchronization, each shared var has one
 */
static pthread_mutex_t* locks;
static pthread_mutex_t fork_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_init_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * mutex_t-> mutex_id hashmap
 * pthread_t -> thread_id hashmap
 */
static unsigned mutex_id = 0;
static boost::unordered_map<pthread_mutex_t *, unsigned> mutex_ht;
static boost::unordered_map<pthread_t, unsigned> thread_ht;
static boost::unordered_map<pthread_t, l_rlog_t*> rlogs;
static boost::unordered_map<pthread_t, l_lrlog_t*> lrlogs;
static boost::unordered_map<pthread_t, l_wlog_t*> wlogs;
static boost::unordered_map<pthread_t, l_addmap_t*> addlogs;

/*
 * set of active threads, not the original tid
 */
static std::set<unsigned> active_threads;

/*
 * number of shared variables
 */
static unsigned num_shared_vars = 0;
static unsigned num_local_vars = 0;

#ifdef NO_TIME_CMD
static struct timeval tpstart, tpend;
#endif

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

/* we do not dump logs here, because it is time consuming
 * we store them in a global map, dump them at last
 * 
 * in practice, we can dump them here to avoid memory leak
 */
void close_read_log(void* log) {
    //pthread_t tid = pthread_self();
    //rlogs[tid] = (l_rlog_t*) log;
}

void close_local_read_log(void* log) {
    //pthread_t tid = pthread_self();
    //lrlogs[tid] = (l_lrlog_t*) log;
}

void close_write_log(void* log) {
    //pthread_t tid = pthread_self();
    //wlogs[tid] = (l_wlog_t*) log;
}

void close_map_log(void* log) {
    //pthread_t tid = pthread_self();
    //addlogs[tid] = (l_addmap_t*) log;
}

void close_cache(void* log) {
    delete (Cache*) log;
}

void close_birthday_key(void* key) {
    delete (size_t*) key;
}

extern "C" {

    void OnInit(unsigned svsNum, unsigned lvsNum) {
        printf("[INFO] OnInit-Record (canary record)\n");
        num_shared_vars = svsNum;
        num_local_vars = lvsNum;
        initializeSigRoutine();

        pthread_key_create(&rlog_key, close_read_log);
        pthread_key_create(&lrlog_key, close_local_read_log);
        pthread_key_create(&wlog_key, close_write_log);
        pthread_key_create(&address_birthday_map_key, close_map_log);
        pthread_key_create(&birthday_key, close_birthday_key);
        pthread_key_create(&cache_key, close_cache);

        write_versions = new unsigned[svsNum];
        memset(write_versions, 0, sizeof (unsigned) * svsNum);
        locks = new pthread_mutex_t[svsNum];

        for (unsigned i = 0; i < svsNum; i++) {
            pthread_mutex_init(&locks[i], NULL);
        }

        // main thread.
        pthread_t tid = pthread_self();
        active_threads.insert(thread_ht.size());
        thread_ht[tid] = thread_ht.size();

#ifdef NO_TIME_CMD
        gettimeofday(&tpstart, NULL);
#endif
    }

    void OnExit() {
#ifdef NO_TIME_CMD
        gettimeofday(&tpend, NULL);
        double timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
        timeuse /= 1000;
        printf("[INFO] Wall time is %lf ms\n", timeuse);
#endif
        active_threads.clear();

        printf("[INFO] OnExit-Record (canary record)\n");

        // dump, delete/free is not needed, because program will exit.
        {//flog
#ifdef DEBUG
            printf("dumping flog...\n");
#endif
            FILE * fout = fopen("fork.dat", "w");
            flog.dumpWithValueUnsignedMap(fout, thread_ht);
            fclose(fout);
        }

        {//mlog, llogs
#ifdef DEBUG
            printf("dumping mlog, llog...\n");
#endif
            FILE * fout = fopen("mutex.dat", "w");
            mlog.dumpWithValueUnsignedMap(fout, thread_ht);
            for (unsigned i = 0; i < llogs.size(); i++) {
                g_llog_t * llog = llogs[i];
                llog->dumpWithValueUnsignedMap(fout, thread_ht);
            }
            fclose(fout);
        }

        {//rlog
#ifdef DEBUG
            printf("dumping rlog...\n");
#endif
            FILE * fout = fopen("read.dat", "w");
            boost::unordered_map<pthread_t, l_rlog_t*>::iterator rit = rlogs.begin();
            while (rit != rlogs.end()) {
                l_rlog_t* rlog = rit->second;
                unsigned _tid = thread_ht[rit->first];
                for (unsigned i = 0; i < num_shared_vars; i++) {
                    rlog[i].VAL_LOG.dumpWithUnsigned(fout, _tid);
                    rlog[i].VER_LOG.dumpWithUnsigned(fout, _tid);
                }

                rit++;
            }
            fclose(fout);
        }
        {//lrlog
#ifdef DEBUG
            printf("dumping lrlog...\n");
#endif
            FILE * fout = fopen("lread.dat", "w");
            boost::unordered_map<pthread_t, l_lrlog_t*>::iterator rit = lrlogs.begin();
            while (rit != lrlogs.end()) {
                l_lrlog_t* rlog = rit->second;
                unsigned _tid = thread_ht[rit->first];
                for (unsigned i = 0; i < num_local_vars; i++) {
                    rlog[i].dumpWithUnsigned(fout, _tid);
                }

                rit++;
            }
            fclose(fout);
        }
        {//wlog
#ifdef DEBUG
            printf("dumping wlog...\n");
#endif
            FILE * fout = fopen("write.dat", "w");
            boost::unordered_map<pthread_t, l_wlog_t*>::iterator wit = wlogs.begin();
            while (wit != wlogs.end()) {
                l_wlog_t* wlog = wit->second;
                unsigned _tid = thread_ht[wit->first];
                for (unsigned i = 0; i < num_shared_vars; i++) {
                    wlog[i].dumpWithUnsigned(fout, _tid);
                }

                wit++;
            }
            fclose(fout);
        }
        {//address birthday map
#ifdef DEBUG
            printf("dumping addressmap...\n");
#endif
            FILE * fout = fopen("addressmap.dat", "w");
            boost::unordered_map<pthread_t, l_addmap_t*>::iterator it = addlogs.begin();
            while (it != addlogs.end()) {
                unsigned _tid = thread_ht[it->first];
                l_addmap_t * addmap = it->second;

                unsigned size = addmap->ADDRESS_LOG.size();
#ifdef LDEBUG
                fprintf(fout, "Size = %u, Thread = %u\n", size, _tid);
#else
                fwrite(&size, sizeof (unsigned), 1, fout);
                fwrite(&_tid, sizeof (unsigned), 1, fout);
#endif
                for (unsigned i = 0; i < size; i++) {
                    mem_t * m = addmap->ADDRESS_LOG[i];
#ifdef LDEBUG
                    fprintf(fout, "(%p, %u); ", m->address, m->range);
#else
                    fwrite(&m->address, sizeof (void*), 1, fout);
                    fwrite(&m->range, sizeof (size_t), 1, fout);
#endif
                }
#ifdef LDEBUG
                fprintf(fout, "\n");
#endif

                addmap->BIRTHDAY_LOG.dump(fout);

#ifdef LDEBUG
                fprintf(fout, "\n");
#endif

                it++;
            }
            fclose(fout);
        }

        printf("[INFO] Threads num: %d\n", thread_ht.size());

        // zip
        system("if [ -f canary.zip ]; then rm canary.zip; fi");
        system("zip -9 canary.zip ./fork.dat ./mutex.dat ./addressmap.dat ./write.dat ./read.dat ./lread.dat");
        system("rm -f ./fork.dat ./mutex.dat ./addressmap.dat ./write.dat ./read.dat ./lread.dat");
        system("echo -n \"Log size (Byte): \"; echo -n `du -sb canary.zip` | cut -d\" \" -f 1;");
    }

    void OnAddressInit(void* value, size_t size, size_t n) {
        l_addmap_t * mlog = (l_addmap_t*) pthread_getspecific(address_birthday_map_key);
        if (mlog == NULL) {
            mlog = new l_addmap_t;
            addlogs[pthread_self()] = mlog;
            pthread_setspecific(address_birthday_map_key, mlog);
        }

        size_t * counter = (size_t *) pthread_getspecific(birthday_key);
        if (counter == NULL) {
            counter = new size_t;
            (*counter) = 0;

            pthread_setspecific(birthday_key, counter);
        }

        size_t c = *counter;
        mem_t * m = new mem_t;
        m->address = value;
        m->range = size*n;
        mlog->ADDRESS_LOG.push_back(m);
        mlog->BIRTHDAY_LOG.logValue(c++);
        (*counter) = c;
    }

    void OnLocal(long value, int id) {
#ifdef DEBUG
        printf("OnLocal %d\n", id);
#endif
        l_lrlog_t * rlog = (l_lrlog_t*) pthread_getspecific(lrlog_key);
        if (rlog == NULL) {
            rlog = new l_lrlog_t[num_local_vars];
            lrlogs[pthread_self()] = rlog;
            pthread_setspecific(lrlog_key, rlog);
        }

        rlog[id].logValue(value);
    }

    void OnLoad(int svId, long address, long value, int debug) {
        if (active_threads.size() <= 1) {
            return;
        }

#ifdef DEBUG
        printf("OnLoad === 1\n");
#endif
        // using thread_key to tell whether log has been established
        // if so, use it, otherwise, new one
        l_rlog_t * rlog = (l_rlog_t*) pthread_getspecific(rlog_key);
        if (rlog == NULL) {
            rlog = new l_rlog_t[num_shared_vars];
            rlogs[pthread_self()] = rlog;
            pthread_setspecific(rlog_key, rlog);
        }

#ifdef DEBUG
        printf("OnLoad === 2\n");
#endif

        Cache* cache = (Cache*) pthread_getspecific(cache_key);
        if (cache != NULL && cache->query(address, value)) {
            rlog[svId].VER_LOG.logValue(-1);
        } else {
            rlog[svId].VAL_LOG.logValue(value);
            rlog[svId].VER_LOG.logValue(write_versions[svId]);
        }
#ifdef DEBUG
        printf("OnLoad === 3\n");
#endif
    }

    unsigned OnPreStore(int svId, int debug) {
        if (active_threads.size() <= 1) {
            return 0;
        }

        lock(svId);
#ifdef DEBUG
        printf("OnPreStore: %d [%d]\n", svId, debug);
#endif
        unsigned version = write_versions[svId];
        write_versions[svId] = version + 1;

        return version;
    }

    void OnStore(int svId, unsigned version, long address, long value, int debug) {
        if (active_threads.size() <= 1) {
            return;
        }
#ifdef DEBUG
        printf("OnStore\n");
#endif
        unlock(svId);

        l_wlog_t * wlog = (l_wlog_t*) pthread_getspecific(wlog_key);
        if (wlog == NULL) {
            wlog = new l_wlog_t[num_shared_vars];
            wlogs[pthread_self()] = wlog;
            pthread_setspecific(wlog_key, wlog);
        }
        wlog[svId].logValue(version);

        Cache* cache = (Cache*) pthread_getspecific(cache_key);
        if (cache == NULL) {
            cache = new Cache;
            pthread_setspecific(cache_key, cache);
        }

        cache->add(address, value);
    }

    void OnLock(pthread_mutex_t* mutex_ptr) {
        if (active_threads.size() <= 1) {
            return;
        }

        // if map does not contain it, it must be a bug
        // because lock and init have a race.
        if (!mutex_ht.count(mutex_ptr)) {
            fprintf(stderr, "ERROR: program bug- mutex is not initialized before lock()!\n");
            OnExit();
            exit(1);
        }

        // here, the same address mutex_ptr is impossible to init a new lock
        // because the lock has been locked
        // therefore mutex_ht[mutex_ptr] is safe
        g_llog_t* llog = llogs[mutex_ht[mutex_ptr]];
        llog->logValue(pthread_self()); // exchange to _tid at last

#ifdef DEBUG
        printf("OnLock --> t%d\n", thread_ht[pthread_self()]);
#endif
    }

    void OnWait(pthread_cond_t* cond_ptr, pthread_mutex_t* mutex_ptr) {
        if (active_threads.size() <= 1) {
            return;
        }
#ifdef DEBUG
        printf("OnWait\n");
#endif

        if (!mutex_ht.count(mutex_ptr)) {
            fprintf(stderr, "ERROR: program bug- mutex is not initialized before wait()!\n");
            OnExit();
            exit(1);
        }
        g_llog_t* llog = llogs[mutex_ht[mutex_ptr]];
        llog->logValue(pthread_self());
    }

    void OnPreMutexInit(bool race) {
        if (active_threads.size() > 1 && race) {
            mutexInitLock();
        }
    }

    void OnMutexInit(pthread_mutex_t* mutex_ptr, bool race) {
        // if mutex_ht contains mutex_ptr, it means that the original one may be
        // destroyed; No matther mutex_ht contains it or not, it is a new lock
        mutex_ht[mutex_ptr] = mutex_id++;
        g_llog_t * llog = new g_llog_t;
        llogs.push_back(llog);


        if (active_threads.size() > 1 && race) {
            pthread_t tid = pthread_self();
            mlog.logValue(tid);

            mutexInitUnlock();
        }
    }

    void OnPreFork(bool race) {
#ifdef DEBUG
        printf("OnPreFork\n");
#endif
        if (race) forkLock();
        active_threads.insert(thread_ht.size());
    }

    void OnFork(pthread_t* forked_tid_ptr, bool race) {
#ifdef DEBUG
        printf("OnFork\n");
#endif

        pthread_t ftid = *(forked_tid_ptr);
        if (!thread_ht.count(ftid)) {
            thread_ht[ftid] = thread_ht.size();
        }

        if (race) {
            pthread_t tid = pthread_self();
            flog.logValue(tid);

            forkUnlock();
        }
    }

    void OnJoin(pthread_t tid, bool race) {
        if (race) {
            forkLock();
            active_threads.erase(thread_ht[tid]);
            forkUnlock();
        } else {
            active_threads.erase(thread_ht[tid]);
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
