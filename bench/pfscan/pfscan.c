/*
** pfscan.c - Parallell File Scanner
**
** Copyright (c) 2002 Peter Eriksson <pen@lysator.liu.se>
**
** This program is free software; you can redistribute it and/or
** modify it as you wish - as long as you don't claim that you wrote
** it.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ftw.h>
#include <locale.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include <pthread.h>

#include "pqueue.h"
#include "bm.h"



#define version "1.0"
#define argv0  "pfscan"


int max_depth = 64;

//unsigned char *rstr = NULL;


#define debug  0
#define verbose  0



#define line_f   0
#define maxlen   64

int n_matches = 0;
int n_files = 0;
size_t n_bytes = 0;

pthread_mutex_t matches_lock;

PQUEUE pqb;

pthread_mutex_t print_lock;


BM bmb;
    

void
print_version(FILE *fp)
{
    fprintf(fp, "[PFScan, version %s - %s %s]\n",
	    version,
	    __DATE__, __TIME__);
}


int
get_char_code(unsigned char **cp,
	      int base)
{
    int val = 0;
    int len = 0;
    
    
    while (len < (base == 16 ? 2 : 3) &&
	   ((**cp >= '0' && **cp < '0'+(base > 10 ? 10 : base)) ||
	    (base >= 10 && toupper(**cp) >= 'A' && toupper(**cp) < 'A'+base-10)))
    {
	val *= base;

	if (**cp >= '0' && **cp < '0'+(base > 10 ? 10 : base))
	    val += **cp - '0';
	else if (base >= 10 &&
		 toupper(**cp) >= 'A' && toupper(**cp) < 'A'+base-10)
	    val += toupper(**cp) - 'A' + 10;

	++*cp;
	++len;
    }

    return val & 0xFF;
}


int
dehex(unsigned char *str)
{
    unsigned char *wp, *rp;
    int val;


    rp = wp = str;

    while (*rp)
    {
	while (*rp && isspace(* (unsigned char *) rp))
	    ++rp;

	if (*rp == '\0')
	    break;
	
	if (!isxdigit(* (unsigned char *) rp))
	    return -1;
	
	val = get_char_code(&rp, 16);
	*wp++ = val;
    }

    *wp = '\0';
    return wp - str;
}


    
int
deslash(unsigned char *str)
{
    unsigned char *wp, *rp;

    
    rp = wp = str;

    while (*rp)
    {
	if (*rp != '\\')
	    *wp++ = *rp++;
	else
	{
	    switch (*++rp)
	    {
	      case 'n':
		*wp++ = 10;
		++rp;
		break;

	      case 'r':
		*wp++ = 13;
		++rp;
		break;

	      case 't':
		*wp++ = 9;
		++rp;
		break;

	      case 'b':
		*wp++ = 8;
		++rp;
		break;

	      case 'x':
		++rp;
		*wp++ = get_char_code(&rp, 16);
		break;
		
	      case '0':
		*wp++ = get_char_code(&rp, 8);
		break;
		
	      case '1':
	      case '2':
	      case '3':
	      case '4':
	      case '5':
	      case '6':
	      case '7':
	      case '8':
	      case '9':
		*wp++ = get_char_code(&rp, 10);
		break;

	      default:
		*wp++ = *rp++;
		break;
	    }
	}
    }

    *wp = '\0';

    return wp-str;
}




int
is_text(unsigned char *cp, int slen)
{
    while (slen > 0 && (isprint(*cp) || *cp == '\0' || *cp == '\t' || *cp == '\n' || *cp == '\r'))
    {
	--slen;
	++cp;
    }

    return slen == 0;
}


size_t
print_output(unsigned char *str,
	     size_t slen)
{
    size_t len;
    

    len = 0;
    
    if (str == NULL)
    {
	printf("NULL");
	return len;
    }

    if (is_text(str, slen))
    {
	printf("TXT : ");
    
	while (len < slen && len < maxlen)
	{
	    if (isprint(* (unsigned char *) str))
		putchar(*str);
	    else
		switch (*str)
		{
		  case '\0':
		    printf("\\0");
		    break;
		    
		  case '\n':
		    if (line_f)
			return len;
		    printf("\\n");
		    break;
		    
		  case '\r':
		    if (line_f)
			return len;
		    printf("\\r");
		    break;
		    
		  case '\t':
		    printf("\\t");
		    break;
		    
		  default:
		    printf("\\x%02x", * (unsigned char *) str);
		}

	    ++len;
	    ++str;
	}
    }
    
    else
    {
	printf("HEX :");
	while (len < slen && len < maxlen)
	{
	    printf(" %02x", * (unsigned char *) str);
	    ++len;
	    ++str;
	}
    }

    return len;
}


int
matchfun(unsigned char *buf,
	 size_t len,
	 size_t pos,
	 void *misc)
{
    char *pathname = (char *) misc;
    
    pthread_mutex_lock(&matches_lock);
    ++n_matches;
    pthread_mutex_unlock(&matches_lock);

    if (line_f)
	while (pos > 0 &&
	       !(buf[pos-1] == '\n' || buf[pos-1] == '\r'))
	    --pos;
    
    pthread_mutex_lock(&print_lock);
    
    printf("%s : %lu : ", pathname, (unsigned long) pos);
    print_output(buf+pos, len-pos);
    putchar('\n');
    
    pthread_mutex_unlock(&print_lock);
    return 0;
}


int
scan_file(char *pathname)
{
    int fd;
    size_t len;
    unsigned char *buf;
    struct stat sb;


    fd = open(pathname, O_RDONLY);
    if (fd < 0)
    {
	if (verbose)
	{
	    pthread_mutex_lock(&print_lock);
	    
	    fprintf(stderr, "%s : ERR : open() failed: %s\n", pathname, strerror(errno));
	    
	    pthread_mutex_unlock(&print_lock);
	}
	
	return -1;
    }


    if (fstat(fd, &sb) < 0)
    {
	if (verbose)
	{
	    pthread_mutex_lock(&print_lock);
	    
	    fprintf(stderr, "%s : ERR : fstat() failed: %s\n", pathname, strerror(errno));
	    
	    pthread_mutex_unlock(&print_lock);
	}

	close(fd);
	return -1;
    }

    len = sb.st_size;
    
    if (debug > 1)
	fprintf(stderr, "*** Scanning file %s (%u Mbytes)\n",
		pathname, (unsigned int) (len / 1000000));
    
    buf = (unsigned char *) mmap(NULL, len, PROT_READ, MAP_PRIVATE|MAP_NORESERVE, fd, 0);
    if (buf == MAP_FAILED)
    {
	if (verbose)
	{
	    pthread_mutex_lock(&print_lock);
	    
	    fprintf(stderr, "%s : ERR : mmap() failed: %s\n", pathname, strerror(errno));
	    
	    pthread_mutex_unlock(&print_lock);
	}

	close(fd);
	return -1;
    }

    if (/*rstr*/1)
    {
	int code;

	code = bm_search(&bmb, buf, len, matchfun, pathname);
    }
    else
    {
	pthread_mutex_lock(&print_lock);
	
	printf("%s : 0 : ", pathname);
	print_output(buf, len);
	putchar('\n');
	
	pthread_mutex_unlock(&print_lock);
    }

    munmap((char *) buf, len);
    close(fd);
    return 1;
}




int
foreach_path(const char *path,
	     const struct stat *sp,
	     int f)
{
    ++n_files;

    n_bytes += sp->st_size;
    
    switch (f)
    {
      case FTW_F:
	pqueue_put(&pqb, (void *) strdup(path));
	return 0;

      case FTW_D:
	return 0;

      case FTW_DNR:
	fprintf(stderr, "%s: %s: Can't read directory.\n",
		argv0, path);
	return 1;

      case FTW_NS:
	fprintf(stderr, "%s: %s: Can't stat object.\n",
		argv0, path);
	return 1;

      default:
	fprintf(stderr, "%s: %s: Internal error (invalid ftw code)\n",
		argv0, path);
    }
    
    return 1;
}


int
do_ftw(char *path)
{
    int code = ftw(path, foreach_path, max_depth);

    
    if (code < 0)
    {
	fprintf(stderr, "%s: ftw: %s\n", argv0, strerror(errno));
	return 1;
    }

    return code;
}


void *
worker(void *arg)
{
    char *path;
    
    
    while (pqueue_get(&pqb, (void **) &path) == 1)
    {
	scan_file(path);
	free(path);
    }

    fflush(stdout);

    //pthread_mutex_lock(&aworker_lock);
    //--aworkers;
    //pthread_mutex_unlock(&aworker_lock);
    //pthread_cond_signal(&aworker_cv);

    return NULL;
}



void
usage(FILE *out)
{
    fprintf(out, "Usage: %s [<options>] <search-string> <pathname> [... <pathname-N>]\n", argv0);

    fputs("\n\
This program implements a multithreaded file scanner.\n\
More information may be found at:\n\
\thttp://www.lysator.liu.se/~pen/pfscan\n\
\n\
Command line options:\n", out);
    
    fprintf(out, "\t-h             Display this information.\n");
    fprintf(out, "\t-V             Print version.\n");
    fprintf(out, "\t-v             Be verbose.\n");
    fprintf(out, "\t-d             Print debugging info.\n");
    fprintf(out, "\t-i             Ignore case when scanning.\n");
    fprintf(out, "\t-l             Line oriented output.\n");
    fprintf(out, "\t-n<workers>    Concurrent worker threads limit.\n");
    fprintf(out, "\t-L<length>     Max length of bytes to print.\n");
}
    


int
main(int argc,
     char *argv[])
{
    int i, j;
    struct rlimit rlb;
    char *arg;
    //pthread_t tid;
    pthread_attr_t pab;

    int ignore_case = 0;
    unsigned char *rstr = NULL;
    
    setlocale(LC_CTYPE, "");

    getrlimit(RLIMIT_NOFILE, &rlb);
    rlb.rlim_cur = rlb.rlim_max;
    setrlimit(RLIMIT_NOFILE, &rlb);

    signal(SIGPIPE, SIG_IGN);

    int nworkers = 2;
    int rlen = 0;

    pthread_mutex_init(&print_lock, NULL);
    //pthread_mutex_init(&aworker_lock, NULL);
    pthread_mutex_init(&matches_lock, NULL);

    for (i = 1; i < argc && argv[i][0] == '-'; i++)
	for (j = 1; j > 0 && argv[i][j]; ++j)
	    switch (argv[i][j])
	    {
	      case '-':
		++i;
		goto EndOptions;
		
	      case 'V':
		print_version(stdout);
		break;

	      case 'd':
		//++debug;
		break;

	      case 'i':
		ignore_case = 1;
		break;
		
	      case 'v':
		//++verbose;
		break;
		
	      case 'h':
		usage(stdout);
		exit(0);
		
	      case 'l':
		//++line_f;
		break;
		
	      case 'L':
		if (argv[i][2])
		    arg = argv[i]+2;
		else
		    arg = argv[++i];
		
		/*if (!arg || sscanf(arg, "%u", &maxlen) != 1)
		{
		    fprintf(stderr, "%s: Invalid length specification: %s\n",
			    argv[0], arg ? arg : "<null>");
		    exit(1);
		}*/
		j = -2;
		break;
		
	      case 'n':
		if (argv[i][2])
		    arg = argv[i]+2;
		else
		    arg = argv[++i];
		
		if (!arg || sscanf(arg, "%u", &nworkers) != 1)
		{
		    fprintf(stderr,
			    "%s: Invalid workers specification: %s\n",
			    argv[0], arg ? arg : "<null>");
		    exit(1);
		}
		j = -2;
		break;
		
	      default:
		fprintf(stderr, "%s: unknown command line switch: -%c\n",
			argv[0], argv[i][j]);
		exit(1);
	    }

  EndOptions:

    rstr = (unsigned char *) strdup(argv[i++]);
    rlen = deslash(rstr);
    
    if (bm_init(&bmb, rstr, rlen, ignore_case) < 0)
    {
	fprintf(stderr, "%s: Failed search string setup: %s\n",
		argv[0], rstr);
	exit(1);
    }
    
    max_depth = rlb.rlim_max - nworkers - 16;


    if (debug)
	fprintf(stderr, "max_depth = %d, nworkers = %d\n", max_depth,
		nworkers);
    
    pqueue_init(&pqb, nworkers + 8);

    pthread_attr_init(&pab);
    pthread_attr_setscope(&pab, PTHREAD_SCOPE_SYSTEM);

    //aworkers = nworkers;
    pthread_t tid[nworkers];
    for (j = 0; j < nworkers; ++j)
	if (pthread_create(&(tid[j]), &pab, worker, NULL) != 0)
	{
	    fprintf(stderr, "%s: pthread_create: failed to create worker thread\n",
		    argv[0]);
	    exit(1);
	}

    while (i < argc && do_ftw(argv[i++]) == 0)
	;

    pqueue_close(&pqb);

    if (debug)
	fprintf(stderr, "Waiting for workers to finish...\n");

    for (j = 0; j < nworkers; ++j){
        pthread_join(tid[j], NULL);
    }
    //pthread_mutex_lock(&aworker_lock);
    //while (aworkers > 0)
    //   pthread_cond_wait(&aworker_cv, &aworker_lock);
    //pthread_mutex_unlock(&aworker_lock);

    if (debug)
	fprintf(stderr, "n_files = %d, n_matches = %d, n_workers = %d, n_Mbytes = %d\n",
		n_files, n_matches, nworkers,
		(int) (n_bytes / 1000000));

    return n_matches;
}
