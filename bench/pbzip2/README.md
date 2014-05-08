pbzip2
======
An order violation concurrency bug in pbzip2-0.9.4.

In pbzip2, the program will spawn consumer threads to do
the compress and spawn a output thread to write data to the
file. However, the main thread only join the output thread
and don't join the consumer threads (line 1862). When the
main thread try to free all the resources, it is possible
that some consumer threads still haven't exited yet (line
896).

The main() will free fifo->mut in queueDelete() function
(line 1912). However, the consumer threads might still need
fifo->mut (line 897). Therefore, a segmentation fault will
be thrown.

The buggy interleaving is like the following:

        Thread 1                                  Thread 2

        void main(...) {                          void *consumer(void *q) {
          ...                                       ...
          for (i=0; i < numCPU; i++) {              for (;;) {
            ret = pthread_create(&con, NULL,          pthread_mutex_lock(fifo->mut);
                      consumer, fifo);                while (fifo->empty) {
            ...                                         if (allDone == 1)
          }
          ret = pthread_create(&output, NULL,
                    fileWriter, OutFilename);
          ...
          // start reading in data
           producer(..., fifo);
          ...
          // wait until exit of thread
          pthread_join(output, NULL);
          ...
          fifo->empty = 1;
          ...
          // reclaim memory
          queueDelete(fifo);
          fifo = NULL;
        
                                                          pthread_mutex_unlock(fifo->mut);
                                                          return NULL;
                                                        }
                                                        ...
                                                      }
                                                      ...
                                                    }
                                                    ...
        }                                         }

1. Run the program
-------------------------------------------------
```bash
./pbzip2 -k -f -p3 test.tar
```


2. Check result
-------------------------------------------------

When the bug is triggered, you will found the program
has a segmentation fault.

