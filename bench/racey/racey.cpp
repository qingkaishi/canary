/*
 * RACEY: a program print a result which is very sensitive to the
 * ordering between processors (races).
 *
 * It is important to "align" the short parallel executions in the
 * simulated environment. First, a simple barrier is used to make sure
 * thread on different processors are starting at roughly the same time.
 * Second, each thread is bound to a physical cpu. Third, before the main
 * loop starts, each thread use a tight loop to gain the long time slice
 * from the OS scheduler.
 *
 * NOTE: This program need to be customized for your own OS/Simulator 
 * environment. See TODO places in the code.
 *
 * Author: Min Xu <mxu@cae.wisc.edu>
 * Main idea: Due to Mark Hill
 * Created: 09/20/02
 *
 * Compile (on Solaris for Simics) :
 *   cc -mt -o racey racey.c magic.o
 * (on linux with gcc)
 *   gcc -m32 -lpthread -o racey racey.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

// TODO: replace assert(0) with your own function that marks a program phase.  
// Example being a simic "magic" instruction, or VMware backdoor call. Printf 
// should be avoided since it may cause app/OS interaction, even de-scheduling.
#define PHASE_MARKER assert(1)

#define   MAX_LOOP 5000000
#define   MAX_ELEM 64

#define   PRIME1   103072243
#define   PRIME2   103995407

/* simics checkpoints Proc Ids (not used, for reference only) */
//int               Ids_01p[1]  = { 6};
//int               Ids_02p[2]  = { 6,  7};
//int               Ids_04p[4]  = { 0,  1,  4,  5};
//int               Ids_08p[8]  = { 0,  1,  4,  5,  6,  7,  8,  9};
//int               Ids_16p[16] = { 0,  1,  4,  5,  6,  7,  8,  9,
//                                 10, 11, 12, 13, 14, 15, 16, 17};

/* Processor IDs are found, on a Sun SMP, through psrinfo,mpstat */
//int               *ProcessorIds = NULL;
//int               NumProcs;
int               startCounter;
pthread_mutex_t   threadLock;   /* counter mutex */

/* shared variables */
unsigned sig[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
union {
  /* 64 bytes cache line */
  char b[64];
  int value;
} m[MAX_ELEM];


/* the mix function */
unsigned mix(unsigned i, unsigned j) {
  return (i + j * PRIME2) % PRIME1;
}

/* The function which is called once the thread is created */
void* ThreadBody(void* tid)
{
  int threadId = *(int *) tid;
  int i;

  /*
   * Thread Initialization:
   *
   * Bind the thread to a processor.  This will make sure that each of
   * threads are on a different processor.  ProcessorIds[threadId]
   * specifies the processor ID which the thread is binding to.
   */
  // TODO:
  // Bind this thread to ProcessorIds[threadId]
  // use processor_bind(), for example on solaris.

  /* seize the cpu, roughly 0.5-1 second on ironsides */
  for(i=0; i<20000; i++);

  /* simple barrier, pass only once */
  pthread_mutex_lock(&threadLock);
  startCounter--;
  if(startCounter == 0) {
     PHASE_MARKER; /* start of parallel phase */
  }
  pthread_mutex_unlock(&threadLock);

  while(startCounter) {usleep(5);};

  /*
   * main loop:
   *
   * Repeatedly using function "mix" to obtain two array indices, read two 
   * array elements, mix and store into the 2nd
   *
   * If mix() is good, any race (except read-read, which can tell by software)
   * should change the final value of mix
   */
  for(i = 0 ; i < MAX_LOOP; i++) {
    unsigned num = sig[threadId];
    unsigned index1 = num%MAX_ELEM;
    unsigned index2;
    num = mix(num, m[index1].value);
    index2 = num%MAX_ELEM;
    num = mix(num, m[index2].value);
    m[index2].value = num;
    sig[threadId] = num;
    /* Optionally, yield to other processors (Solaris use sched_yield()) */
    /* pthread_yield(); */
  }
  return NULL;
}

int
main(int argc, char* argv[])
{
  pthread_t*     threads;
  int*           tids;
  pthread_attr_t attr;
  int            ret;
  int            mix_sig, i;
  int               *ProcessorIds;
  int               NumProcs;

  /* Parse arguments */
  if(argc < 3) {
    fprintf(stderr, "%s <numProcesors> <pIds>\n", argv[0]);
    exit(1);
  }
  NumProcs = atoi(argv[1]);
  assert(NumProcs > 0 && NumProcs < 16);
  assert(argc == (NumProcs+2));
  ProcessorIds = (int *) malloc(sizeof(int) * NumProcs);
  assert(ProcessorIds != NULL);
  for(i=0; i < NumProcs; i++) {
    ProcessorIds[i] = atoi(argv[i+2]);
  }

  /* Initialize the mix array */
  for(i = 0; i < MAX_ELEM; i++) {
    m[i].value = mix(i,i);
  }

  /* Initialize barrier counter */
  startCounter = NumProcs;

  /* Initialize array of thread structures */
  threads = (pthread_t *) malloc(sizeof(pthread_t) * NumProcs);
  assert(threads != NULL);
  tids = (int *) malloc(sizeof (int) * NumProcs);
  assert(tids != NULL);

  /* Initialize thread attribute */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  ret = pthread_mutex_init(&threadLock, NULL);
  assert(ret == 0);

  for(i=0; i < NumProcs; i++) {
    /* ************************************************************
     * pthread_create takes 4 parameters
     *  p1: threads(output)
     *  p2: thread attribute
     *  p3: start routine, where new thread begins
     *  p4: arguments to the thread
     * ************************************************************ */
    tids[i] = i;
    ret = pthread_create(&threads[i], &attr, ThreadBody, &tids[i]);
    assert(ret == 0);

  }

  /* Wait for each of the threads to terminate */
  for(i=0; i < NumProcs; i++) {
    ret = pthread_join(threads[i], NULL);
    assert(ret == 0);
  }

  /* compute the result */
  mix_sig = sig[0];
  for(i = 1; i < NumProcs ; i++) {
    mix_sig = mix(sig[i], mix_sig);
  }

  PHASE_MARKER; /* end of parallel phase */

  /* print results */
  printf("\n\nShort signature: %08x @ %p\n\n\n", mix_sig, &mix_sig);
  fflush(stdout);
  usleep(5);

  PHASE_MARKER;

  pthread_mutex_destroy(&threadLock);

  pthread_attr_destroy(&attr);
}


