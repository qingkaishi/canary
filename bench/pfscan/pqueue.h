/*
** pqueue.h - FIFO queue management routines.
**
** Copyright (c) 1997-2000 Peter Eriksson <pen@lysator.liu.se>
**
** This program is free software; you can redistribute it and/or
** modify it as you wish - as long as you don't claim that you wrote
** it.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef PLIB_PQUEUE_H
#define PLIB_PQUEUE_H

typedef struct
{
    void **buf;
    int qsize;
    int occupied;
    int nextin;
    int nextout;
    int closed;
    pthread_mutex_t mtx;
    pthread_cond_t more;
    pthread_cond_t less;
} PQUEUE;


extern int
pqueue_init(PQUEUE *bp,
	   int qsize);

extern void
pqueue_destroy(PQUEUE *bp);

extern int
pqueue_put(PQUEUE *bp,
	  void *item);

extern int
pqueue_get(PQUEUE *bp,
	   void **item);

extern void
pqueue_close(PQUEUE *bp);

#endif
    
