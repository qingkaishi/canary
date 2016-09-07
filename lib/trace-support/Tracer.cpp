/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <list>
#include "trace-support/Event.h"

using namespace std;

#define MAX_EVENT_NUM 100000
#define MAX_MEMORY_ACCESS 100000

int shared(int mem);
void sigroutine(int dunno);

Event events[MAX_EVENT_NUM];
int events_pointer = 0;

void createEvent(int eid,
        pthread_t tid, //unsigned long int %lu
        long mem,
        int type,
        int * locks,
        int lock_num,
        pthread_t synctid,
        pthread_cond_t * cond,
        const char * srcfile,
        long line) {

    events[events_pointer].eid = eid;
    events[events_pointer].tid = tid;
    events[events_pointer].mem = mem;
    events[events_pointer].type = type;
    strcpy(events[events_pointer].srcfile, srcfile);
    events[events_pointer].locks = locks;
    events[events_pointer].lock_num = lock_num;
    events[events_pointer].synctid = synctid;
    events[events_pointer].cond = cond;
    events[events_pointer++].line = line;

    if (events_pointer == MAX_EVENT_NUM) {
        printf(">= %d\n", MAX_EVENT_NUM);
        exit(-1);
    }
}

pthread_mutex_t mutex;
pthread_mutexattr_t Attr;
FILE* fout, *fdebug;

void dump() {
    fprintf(fdebug, "Events Number: %d\n", events_pointer);
    long outputNum = 0;
    for (int i = 0; i < events_pointer; i++) {
        long mem = events[i].mem;
        int type = events[i].type;

        long line = events[i].line;
        pthread_t tid = events[i].tid;
        pthread_t synctid = events[i].synctid;
        long cond = (long) events[i].cond;
        const char * srcfile = events[i].srcfile;
        switch (type) {
            case READ:
                fprintf(fdebug, "READ from %ld at Line %s:%ld in thread %lu\n", mem, srcfile, line, tid);
                break;
            case WRITE:
                fprintf(fdebug, "WRITE to %ld at Line %s:%ld in thread %lu\n", mem, srcfile, line, tid);
                break;
            case ACQUIRE:
                fprintf(fdebug, "ACQUIRE %ld at Line %s:%ld in thread %lu\n", mem, srcfile, line, tid);
                break;
            case RELEASE:
                fprintf(fdebug, "RELEASE %ld at Line %s:%ld in thread %lu\n", mem, srcfile, line, tid);
                break;
            case WAIT:
                fprintf(fdebug, "WAITED (%ld, %ld) at Line %s:%ld in thread %lu\n", cond, mem, srcfile, line, tid);
                break;
            case NOTIFY:
                fprintf(fdebug, "NOTIFY (%ld, %ld) at Line %s:%ld in thread %lu\n", cond, mem, srcfile, line, tid);
                break;
            case FORK:
                fprintf(fdebug, "FORK %lu at Line %s:%ld in thread %lu\n", synctid, srcfile, line, tid);
                break;
            case JOIN:
                fprintf(fdebug, "JOIN %lu at Line %s:%ld in thread %lu\n", synctid, srcfile, line, tid);
                break;
            default:
                break;
        }

        events[i].eid = outputNum;
        outputNum++;
    }
    fprintf(fdebug, "Output Number: %ld\n", outputNum);

    fwrite(&outputNum, sizeof (long), 1, fout);
    fwrite(events, sizeof (struct Event), outputNum, fout);

    fclose(fout);
    fclose(fdebug);
}

/* ************************************************************************
 * test: shared memory?
 * ************************************************************************/
/*struct Memory {
    long mem;
    long tid;
} memories[MAX_MEMORY_ACCESS];
int memory_pointer = 0;

int exist(long mem) {
    for (int i = 0; i < memory_pointer; i++) {
        if (memories[i].mem == mem) return i;
    }
    return -1;
}

void test(long mem, unsigned long int tid) {
    int ttid = (int) tid;
    int pointer = exist(mem); // -1 not exist, others exist

    if (pointer != -1) {
        int ctid = memories[pointer].tid;
        if (ctid != -1 && ttid != ctid) {
            memories[pointer].tid = -1; // -1 means the memory is shared
        }
    } else {
        memories[memory_pointer].mem = mem;
        memories[memory_pointer++].tid = ttid;

        if (memory_pointer == MAX_MEMORY_ACCESS) {
            printf(">= %d\n", MAX_MEMORY_ACCESS);
            exit(-1);
        }
    }
}

int shared(long mem) {
    int pointer = exist(mem); // -1 not exist, others exist

    if (pointer != -1) {
        if (memories[pointer].tid == -1) return 1;
    } else {
        printf("error!\n");
        exit(-1);
    }
    return 0;
}*/

/* ************************************************************************
 * Init?
 * ************************************************************************/
int start = 0;

extern "C" {

    /* ************************************************************************
     * Instrumented function
     * ************************************************************************/
    void OnInit() {
        //printf("OnInit\n");

        pthread_t tid = pthread_self();
        printf("*** Program started (Thread %lu) ***\n", tid);

        signal(SIGHUP, sigroutine);
        signal(SIGINT, sigroutine);
        signal(SIGQUIT, sigroutine);
        signal(SIGKILL, sigroutine);

        fout = fopen("trace.out", "wb");
        if (!fout) {
            printf("init fail!\n");
            exit(-1);
        }

        fdebug = fopen("trace.debug", "w+");
        if (!fdebug) {
            printf("init fail!\n");
            exit(-1);
        }

	pthread_mutexattr_init(&Attr);
	pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &Attr);

        start = 1;
    }

    void OnExit() {
        if (!start)
            return;
        //printf("OnExit\n");

        start = 0;
        pthread_t tid = pthread_self();
        printf("*** Program ended (Thread %lu) ***\n", tid);
        dump();
        return;
    }

    void OnPreLoad(long* mem, long line, char * file) {
        if (!start) {
            return;
        }

        //printf("OnPreLoad\n");
        pthread_mutex_lock(&mutex);
    }

    void OnLoad(long* mem, long line, char * file) {
        //printf("OnLoad\n");
        if (!start) {
            return;
        }
        
        createEvent(0, pthread_self(), (long)mem, READ, NULL, 0, 0, NULL, file, line);

        //test(events[events_pointer - 1].mem, events[events_pointer - 1].tid);

        pthread_mutex_unlock(&mutex);
    }

    void OnPreStore(long* mem, long line, char * file) {
        if (!start) {
            return;
        }
        //printf("OnPreStore\n");

        pthread_mutex_lock(&mutex);
    }

    void OnStore(long* mem, long line, char * file) {
        if (!start) {
            return;
        }
        
        //printf("OnStore\n");
        createEvent(0, pthread_self(), (long)mem, WRITE, NULL, 0, 0, NULL, file, line);

        //test(events[events_pointer - 1].mem, events[events_pointer - 1].tid);

        pthread_mutex_unlock(&mutex);
    }

    void OnPreLock(long* mem, long line, char * file) {
        return;
    }

    void OnLock(long* mem, long line, char * file) {
        //printf("OnLock\n");
        if (!start) {
            return;
        }
        
        createEvent(0, pthread_self(), (long)mem, ACQUIRE, NULL, 0, 0, NULL, file, line);

    }

    void OnPreUnlock(long* mem, long line, char * file) {
        if (!start) {
            return;
        }
        
        createEvent(0, pthread_self(), (long)mem, RELEASE, NULL, 0, 0, NULL, file, line);
    }

    void OnUnlock(long *mem, long line, char * file) {
        //printf("OnUnLock\n");
    }

    void OnPreFork(long* tid, long line, char * file) {
        //printf("OnPreFork\n");
        if (!start) {
            return;
        }

        pthread_mutex_lock(&mutex);
    }

    void OnFork(long* tid, long line, char * file) {
        if (!start) {
            return;
        }
        
        createEvent(0, pthread_self(), 0, FORK, NULL, 0, *((pthread_t*) tid), NULL, file, line);
        //printf("OnFork\n");

        pthread_mutex_unlock(&mutex);
    }

    void OnPreJoin(unsigned long tid, long line, char * file) {
        //printf("OnPreJoin\n");
    }

    void OnJoin(unsigned long tid, long line, char * file) {
        if (!start) {
            return;
        }
        //printf("OnJoin\n");
        createEvent(0, pthread_self(), 0, JOIN, NULL, 0, tid, NULL, file, line);
        
    }

    void OnPreWait(long* cond, long *mem, long line, char * file) {
        //printf("OnPreWait\n");
    }

    void OnWait(long* cond, long *mem, long line, char * file) {
        if (!start) {
            return;
        }
        //printf("OnWait\n");
        
        createEvent(0, pthread_self(), (long) mem, WAIT, NULL, 0, 0, (pthread_cond_t*) cond, file, line);
    }

    void OnPreNotify(long* cond, long *mem, long line, char * file) {
        if (!start) {
            return;
        }
        //printf("OnPreNotify\n");
        pthread_mutex_lock(&mutex);
    }

    void OnNotify(long* cond, long *mem, long line, char * file) {
        if (!start) {
            return;
        }
        //printf("OnNotify\n");
        
        createEvent(0, pthread_self(), (long) mem, NOTIFY, NULL, 0, 0, (pthread_cond_t*) cond, file, line);

        pthread_mutex_unlock(&mutex);
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

    OnExit();
    exit(dunno);
}
