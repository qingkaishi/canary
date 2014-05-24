Benchmarks
============================
All benchmarks belong to their respective owners.  If you are the creator
and/or rights holder of one of the benchmarks and would like it taken down,
please contact qingkaishi@gmail.com.

If you want to report a bug or want to join us to contribute to it, please 
do not hasitate to contact qingkaishi@gmail.com.

Usage
--------------------------------
```bash
make # a bit code file will be generated for each app
cd $APP # e.g. aget
./bench -c $TRANSFORMER # optional, e.g. canary-record-transformer
./bench -l$LIB -L$LIB_PATH # e.g. LIB=canaryrecord LIB_PATH=/usr/local/lib
./bench -d # run it
```

Description
--------------------
* aget

AGET is a multithreaded HTTP download accelerator.

* bbuf

BBUF is a shared bounded buff implementation.

* canneal

CANNEAL uses cache-aware simulated annealing (SA) to minimize the routing 
cost of a chip design. Canneal uses fine-grained parallelism with a 
lock-free algorithm and a very aggressive synchronization strategy that is 
based on data race recovery instead of avoidance.

* pbzip2

PBZIP2 is a parallel implementation of the bzip2 block-sorting file compressor 
that uses pthreads.

* pfscan

PFSCAN is a parallel file scanner.

* racey

RACEY is a benchmark for deterministic replay systems firstly used in "A 'Flight 
Data Recorder' for Enabling Full-system Multiprocessor Deterministic Replay; 
in ISCA 2003"

* simplerace

SIMPLERACE is a simple racy program firstly used in "A trace simplification 
technique for effective debugging of concurrent programs; in FSE 2010".

* swarm

SWARM is a parallel sort implementation.

* memcached

MEMCACHED is a free & open source, high-performance, distributed memory object 
caching system, generic in nature, but intended for use in speeding up dynamic 
web applications by alleviating database load.

It is an in-memory key-value store for small chunks of arbitrary data (strings, 
objects) from results of database calls, API calls, or page rendering


TODO
-------------------------------
* More benchmarks
* Make it easier to use
