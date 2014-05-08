
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 *
 */

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "Head.h"
#include "Data.h"
#include "Defs.h"
#include "Misc.h"
#include "Download.h"


/* Hacked by qingkai, so that it is easy to replay */
const char * HEAD4REPLAY="HTTP/1.0 206 Partial Content\r\nDate: Mon, 16 Dec 2013 11:38:02 GMT\r\nServer: Apache/2.2.15 (CentOS)\r\nLast-Modified: Tue, 02 Jul 2013 22:42:24 GMT\r\nETag: \"160059b-a7d8c7-4e08f0f087400\"\r\nAccept-Ranges: bytes\r\nContent-Length: 5500004\r\nContent-Range: bytes 5500003-11000006/11000007\r\nContent-Type: application/zip\r\nX-Cache: MISS from ustlnx52.ust.hk\r\nX-Cache-Lookup: MISS from ustlnx52.ust.hk:8080\r\nVia: 1.0 ustlnx52.ust.hk (squid/3.1.20-20120626-r10456)\r\nConnection: close\r\n\r\n";

extern sigset_t signal_set;

unsigned int bwritten = 0;
pthread_mutex_t bwritten_mutex = PTHREAD_MUTEX_INITIALIZER;

void *http_get(void *arg) {
	struct thread_data *td;
	int sd;
	char *rbuf, *s;
	int dr, dw, i; 
	long foffset;
	pthread_t tid;
	sigset_t set;

	tid = pthread_self();
	/* Block out all signals */
	sigfillset(&set);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	/* Set Cancellation Type to Asynchronous */
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	td = (struct thread_data *)arg;
	foffset = td->foffset;
	
	if(td->soffset < 0 || td->soffset >= td->foffset) { /* If download complete */
		td->status = STAT_OK;		/* Tell that download is OK. */
		pthread_exit((void *)1);
		return NULL;
	}

	rbuf = (char *)calloc(MAXBUFSIZ, sizeof(char));
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		Log("<THREAD #%ld> socket creation failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
	}

	if ((connect(sd, (const struct sockaddr *)&td->sin, sizeof(struct sockaddr))) == -1) {
		Log("<THREAD #%ld> connection failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
	}

	if ((send(sd, td->getstr, strlen(td->getstr), 0)) == -1) {
		Log("<THREAD #%ld> send failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
	}

        if ((dr = recv(sd, rbuf, MAXBUFSIZ-1, 0)) == -1) {
		Log("<THREAD #%ld> recv failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
        }
	rbuf[dr] = '\0';
	
	/* Hacked by qingkai, so that it is easy to replay */
	strcpy(rbuf, HEAD4REPLAY);
	dr = strlen(rbuf);

	handleHttpRetcode(rbuf);
        if ((strstr(rbuf, "HTTP/1.0 206")) == NULL) {
		fprintf(stderr, "Something unhandled happened, shutting down...\n");
		exit(1);
	}
	s = rbuf;
	i = 0;
	while(1 && (s - rbuf) < dr) {
		if (*s == '\n' && *(s - 1) == '\r' && *(s - 2) == '\n' && *(s - 3) == '\r' && (s - rbuf) < dr) {
			s++;
			i++;
			break;
		}
		s++;
		i++;
	}

	if ((dr - i ) > foffset)
		dw = pwrite(td->fd, s, (foffset - i), td->soffset);
	else
		dw = pwrite(td->fd, s, (dr - i), td->soffset);
	td->offset = td->soffset + dw;
	pthread_mutex_lock(&bwritten_mutex);
	bwritten += dw;
	pthread_mutex_unlock(&bwritten_mutex);

	pthread_testcancel();	/* Check for pending cancel requests */
	
	int idx = 0;
	while (td->offset < foffset) {
		fd_set set;
		/* Set Cancellation Type to Asynchronous
		 * so that a blocking recv() doesn't cause problems
		 */
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

		/* patch by Gurer Ozen	
		 * in slow downloads, avoids excessive CPU consuming...
		 */
		FD_ZERO(&set);
		FD_SET(sd,&set);
		select(FD_SETSIZE,&set,NULL,NULL,NULL);
		/* --end of patch	*/

		memset(rbuf, 0, MAXBUFSIZ);

		/* Hacked by qingkai, so that it is easy to replay */
		dr = MAXBUFSIZ/2;//recv(sd, rbuf, MAXBUFSIZ, 0);
		
		/* The following stmt shows that dr is not a fixed data; haked by qingkai */
		//printf("[%d]====> %d/%d [Thread %d]\n\n", idx++, dr, MAXBUFSIZ, tid);

		/* Set Cancellation Type back to Deferred */
		pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

		/* Qingkai: We can add a signal method here to send SIGINT to the program to expose the crug TODO */
		//if(idx++ == 500)
		    //kill(getpid(), SIGINT);

		if ((td->offset + dr) > foffset)
			dw = pwrite(td->fd, rbuf, foffset - td->offset, td->offset);
		else
			dw = pwrite(td->fd, rbuf, dr, td->offset);
		td->offset += dw;

		pthread_mutex_lock(&bwritten_mutex);
		bwritten += dw;
		pthread_mutex_unlock(&bwritten_mutex);
		pthread_testcancel();	/* Check for pending cancel requests */
	}

	if (td->offset == td->foffset)
		td->status = STAT_OK;		/* Tell thet download is OK. */
	close(sd);
	pthread_exit(NULL);
	return NULL;
}
