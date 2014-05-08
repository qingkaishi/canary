
+--------------------+
|                    |
| SUMMARY            |
|                    |
+--------------------+

A data race and atomicity violation in Aget.

+---------------------------------------------------------+
|                                                         |
| DETAILS                                                 |
|                                                         |
+---------------------------------------------------------+

The bug is triggered when user stop the download process by
typing ctrl-c on the console. The aget program will process
the SIGINT signal, and call function 'save_log' to save
downloaded blocks so that the download can be resumed.

Resume.c (line 30)

void save_log()
{
  ...
  memcpy(&(h.wthread), wthread, sizeof(struct thread_data) * nthreads);
  ...
  h.bwritten = bwritten;
  ...
}

The 'memcpy' function call saves the bytes downloaded by
each thread, and 'h.bwritten = bwritten' saves the total
number of bytes downloaded by all thread. In normal
situation, the total number bytes downloaded by each thread
should be the same as 'bwritten'. However, if the following
interleaving happens, the results will mismatch:

The buggy interleaving is like the following:

Thread 1                                  Thread 2

void http_get(...)                        void save_log(...)
{                                         {
                                            ...
                                            memcpy(&(h.wthread), wthread, ...);
                                            ...
  td->offset += dw;

  pthread_mutex_lock(&bwritten_mutex);
  bwritten += dw;
  pthread_mutex_unlock(&bwritten_mutex);

                                            h.bwritten = bwritten;  
  ...                                       ...
}                                         }          



+--------------------+
|                    |
| SUMMARY            |
|                    |
+--------------------+

A data race and atomicity violation bug in Aget.

+---------------------------------------------------------+
|                                                         |
| HOW TO INSTALL                                          |
|                                                         |
+---------------------------------------------------------+

1. Download aget-devel source
-------------------------------------------------

# tar jxvf aget-devel.tar.gz


2. Apply the patch
-------------------------------------------------

# cd <aget_src_dir>
# patch -p1 -i aget-bug2.patch


3. Compile
-------------------------------------------------

# make


+---------------------------------------------------------+
|                                                         |
| REPRODUCE THE BUG                                       |
|                                                         |
+---------------------------------------------------------+

1. Downloading a big file and stop in the middle
-------------------------------------------------

# aget -n2 http://mirror.candidhosting.com/pub/apache/httpd/httpd-2.2.17.tar.gz -l aget.file

Type ctrl+c to stop the download in the middle.


2. Check result (run verify script)
-------------------------------------------------

# reproduce-pkg/verify

If the bug is triggered, the verify program will have an
assertion failure:

verifier: verifier.c:101: read_log: Assertion `total_bwritten == bwritten' failed.
Aborted
