A small benchmarks
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

TODO
-------------------------------
* More benchmarks
* Make it easier to use
