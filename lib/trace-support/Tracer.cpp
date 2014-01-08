#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include "Event.h"

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
        int line) {

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
FILE* fout, *fdebug;

void dump() {
    Event events_temp[MAX_EVENT_NUM]; 
    fprintf(fdebug, "Events Number: %d\n", events_pointer);
    int outputNum = 0;
    for (int i = 0; i < events_pointer; i++) {
        long mem = events[i].mem;
        int type = events[i].type;
        if (type <= 1 && !shared(mem)) {
            continue;
        }// read or write but not shared.
        int line = events[i].line;
        pthread_t tid = events[i].tid;
        pthread_t synctid = events[i].synctid;
        int cond = (int) events[i].cond;
        const char * srcfile = events[i].srcfile;
        switch (type) {
            case READ:
                fprintf(fdebug, "READ from %ld at Line %s:%d in thread %lu\n", mem, srcfile, line, tid);
                break;
            case WRITE:
                fprintf(fdebug, "WRITE to %ld at Line %s:%d in thread %lu\n", mem, srcfile, line, tid);
                break;
            case ACQUIRE:
                fprintf(fdebug, "ACQUIRE %ld at Line %s:%d in thread %lu\n", mem, srcfile, line, tid);
                break;
            case RELEASE:
                fprintf(fdebug, "RELEASE %ld at Line %s:%d in thread %lu\n", mem, srcfile, line, tid);
                break;
            case WAIT:
                fprintf(fdebug, "WAITED (%d, %ld) at Line %s:%d in thread %lu\n", cond, mem, srcfile, line, tid);
                break;
            case NOTIFY:
                fprintf(fdebug, "NOTIFY (%d, %ld) at Line %s:%d in thread %lu\n", cond, mem, srcfile, line, tid);
                break;
            case FORK:
                fprintf(fdebug, "FORK %lu at Line %s:%d in thread %lu\n", synctid, srcfile, line, tid);
                break;
            case JOIN:
                fprintf(fdebug, "JOIN %lu at Line %s:%d in thread %lu\n", synctid, srcfile, line, tid);
                break;
            default:
                break;
        }

        events_temp[outputNum] = events[i];
        events_temp[outputNum].eid = outputNum;
        outputNum++;
    }
    fprintf(fdebug, "Output Number: %d\n", outputNum);

    fwrite(&outputNum, sizeof (int), 1, fout);
    fwrite(events_temp, sizeof (struct Event), outputNum, fout);

    fclose(fout);
    fclose(fdebug);
}

/* ************************************************************************
 * test: shared memory?
 * ************************************************************************/
struct Memory {
    int mem;
    int tid;
} memories[MAX_MEMORY_ACCESS];
int memory_pointer = 0;

int exist(int mem) {
    for (int i = 0; i < memory_pointer; i++) {
        if (memories[i].mem == mem) return i;
    }
    return -1;
}

void test(int mem, unsigned long int tid) {
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

int shared(int mem) {
    int pointer = exist(mem); // -1 not exist, others exist

    if (pointer != -1) {
        if (memories[pointer].tid == -1) return 1;
    } else {
        printf("error!\n");
        exit(-1);
    }
    return 0;
}

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

    void OnPreLoad(int* mem, int line, char * file) {
        if (!start) {
            return;
        }

        //printf("OnPreLoad\n");
        pthread_mutex_lock(&mutex);
    }

    void OnLoad(int* mem, int line, char * file) {
        //printf("OnLoad\n");
        if (!start) {
            return;
        }
        
        createEvent(0, pthread_self(), (long)mem, READ, NULL, 0, 0, NULL, file, line);

        test(events[events_pointer - 1].mem, events[events_pointer - 1].tid);

        pthread_mutex_unlock(&mutex);
    }

    void OnPreStore(int* mem, int line, char * file) {
        if (!start) {
            return;
        }
        //printf("OnPreStore\n");

        pthread_mutex_lock(&mutex);
    }

    void OnStore(int* mem, int line, char * file) {
        if (!start) {
            return;
        }
        
        //printf("OnStore\n");
        createEvent(0, pthread_self(), (long)mem, WRITE, NULL, 0, 0, NULL, file, line);

        test(events[events_pointer - 1].mem, events[events_pointer - 1].tid);

        pthread_mutex_unlock(&mutex);
    }

    void OnPreLock(int* mem, int line, char * file) {
        return;
    }

    void OnLock(int* mem, int line, char * file) {
        //printf("OnLock\n");
        if (!start) {
            return;
        }
        
        createEvent(0, pthread_self(), (long)mem, ACQUIRE, NULL, 0, 0, NULL, file, line);

    }

    void OnPreUnlock(int* mem, int line, char * file) {
        if (!start) {
            return;
        }
        
        createEvent(0, pthread_self(), (long)mem, RELEASE, NULL, 0, 0, NULL, file, line);
    }

    void OnUnlock(int *mem, int line, char * file) {
        //printf("OnUnLock\n");
    }

    void OnPreFork(int* tid, int line, char * file) {
        //printf("OnPreFork\n");
        if (!start) {
            return;
        }

        pthread_mutex_lock(&mutex);
    }

    void OnFork(int* tid, int line, char * file) {
        if (!start) {
            return;
        }
        
        createEvent(0, pthread_self(), 0, FORK, NULL, 0, *((pthread_t*) tid), NULL, file, line);
        //printf("OnFork\n");

        pthread_mutex_unlock(&mutex);
    }

    void OnPreJoin(unsigned long tid, int line, char * file) {
        //printf("OnPreJoin\n");
    }

    void OnJoin(unsigned long tid, int line, char * file) {
        if (!start) {
            return;
        }
        //printf("OnJoin\n");
        createEvent(0, pthread_self(), 0, JOIN, NULL, 0, tid, NULL, file, line);
        
    }

    void OnPreWait(int* cond, int *mem, int line, char * file) {
        //printf("OnPreWait\n");
    }

    void OnWait(int* cond, int *mem, int line, char * file) {
        if (!start) {
            return;
        }
        //printf("OnWait\n");
        
        createEvent(0, pthread_self(), (long) mem, WAIT, NULL, 0, 0, (pthread_cond_t*) cond, file, line);
    }

    void OnPreNotify(int* cond, int *mem, int line, char * file) {
        if (!start) {
            return;
        }
        //printf("OnPreNotify\n");
        pthread_mutex_lock(&mutex);
    }

    void OnNotify(int* cond, int *mem, int line, char * file) {
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
