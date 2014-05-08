#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <netinet/in.h>
#include <pthread.h>

enum {
	GETREQSIZ = 256,
	GETRECVSIZ = 8192,
	MAXBUFSIZ = 8192,
	HEADREQSIZ = 512,
	MAXURLSIZ = 1024,
	MAXHOSTSIZ = 1024,
	MAXIPSIZ = 16,
	MAXTHREADS = 10,
	VALSIZE = 256,
	HTTPPORT = 80,
	UNKNOWNREQ = 2,
	FTPREQ = 21,
	PROTO_HTTP = 0xFF,
	PROTO_FTP = 0x00,
	STAT_OK = 0xFF,		/* Download completed successfully	*/
	STAT_INT = 0x0F,	/* ^C caught, download interrupted	*/
	STAT_ERR = 0x00		/* Download finished with error		*/
};

typedef struct request {
	char host[MAXHOSTSIZ];		/* Remote host	*/
	char url[MAXURLSIZ];		/* URL		*/
	char file[MAXBUFSIZ];		/* file name	*/
	char lfile[MAXBUFSIZ];		/* if local file name is specified */
	char ip[MAXIPSIZ];		/* Remote IP	*/
	char username[MAXBUFSIZ];
	char password[MAXBUFSIZ];
	int port;
	int ftp_server_type;
	int clength;			/* Content-length	*/
	unsigned char proto;		/* Protocol		*/
	time_t time_taken;		/* Useful when job is resumed */
} request;

typedef struct thread_data {
	struct sockaddr_in sin;
	char getstr[GETREQSIZ];
	long soffset;		/* Start offset		*/
	long foffset;		/* Finish offset	*/
	long offset;		/* Current Offset	*/
	long clength;		/* Content Length	*/
	int fd;
	pthread_t tid;		/* Thread ID		*/
	unsigned char status;	/* thread exit status	*/
	char username[MAXBUFSIZ];	/* Used in ftp_get() */
	char password[MAXBUFSIZ];	/* Used in ftp_get() */
	char url[MAXURLSIZ];		/* Used in ftp_get() */
	int head_sd;			/* Used for reusing head connection	*/
} thread_data;

typedef struct hist_data {
	struct request req;
	int nthreads;
	int bwritten;
	struct thread_data wthread[MAXTHREADS];
} hist_data;

unsigned int bwritten;

int read_log(struct hist_data *h)
{
	char *logfile;
	FILE *fp;
        char *user_home;
        int i;
	long total_bwritten;

	logfile = (char *)calloc(255, sizeof(char));

	user_home = getenv("HOME");
	snprintf(logfile, 255, "%s/aget.file-ageth.log", user_home);

	if ((fp = fopen(logfile, "r")) == NULL) {
		printf("cannot open log file\n");
		exit(-1);
	}

	fread(h, sizeof(struct hist_data), 1, fp);
	bwritten = h->bwritten;
	fclose(fp);

	if ((unlink(logfile)) == -1) {
		printf("cannot delete log file\n");
		exit(-1);
	}
		
	free(logfile);

	total_bwritten = 0;
        for (i = 0; i < h->nthreads; i++) {
		total_bwritten += (h->wthread[i].offset - h->wthread[i].soffset);
	}
	assert(total_bwritten == bwritten);

	return 0;
}

int main(int argc, char *argv[]) {
	struct hist_data h;
	read_log(&h);
	printf("results ok!\n");
	return 0;
}

