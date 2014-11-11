Canary
======

Canary is a set of tools built on a unification-based alias analysis.
Relative papers include "Points-to analysis in almost linear time", 
"Fast algorithms for Dyck-CFL-reachability with applications to alias 
analysis", "LEAP: lightweight deterministic multi-processor replay of 
concurrent java programs", "Persuasive prediction of concurrency 
access anomalies", etc. You can read them for details.

Please use canary with llvm-3.4.

We have builted and tested it on *32-bit* x86 linux architectures using
gcc 4.8.2.

Building Canary
------

```bash
cd <llvm_src_code_dir>/<projects>/
git clone https://github.com/qingkaishi/canary.git
cd canary
./configure
sudo make install
```


Using Canary
------

**Using Alias Analysis**

```bash
clang -c -emit-llvm -O2 -g -fno-vectorize -fno-slp-vectorize <src_file> -o <bitcode_file>
canary <bitcode_file> -o <output_file>
```

Or you can build a shared library (you need to modify the Makefile yourself), 
and use the following equivalent commands.

```bash
clang -c -emit-llvm -O2 -g -fno-vectorize -fno-slp-vectorize <src_file> -o <bitcode_file>
opt -load dyckaa.so -lowerinvoke  -dyckaa -basicaa  <bitcode_file> -o <output_file>
```

* -print-alias-set-info
This will print the evaluation of alias sets and outputs all alias sets, and their 
relations (dot style).

* -count-fp
Count how many functions that a function pointer may point to.

* -dot-may-callgraph
This option is used to print a call graph based on the alias analysis.
You can use it with -with-labels option, which will add lables (call insts)
to the edges in call graphs.

* -leap-transformer
A transformer for LEAP. Please read ``LEAP: lightweight deterministic 
multi-processor replay of concurrent java programs". Here is an example.

```bash
# transform
canary -leap-transformer <bitcode_file> -o <output_file>
# link a record version
clang++ <ouput_file> -o <executable> -lleaprecord
# execute it
# link a replay version
clang++ <ouput_file> -o <executable> -lleapreplay
# now you can replay
```

* -trace-transformer
A transformer for Pecan. Please read "Persuasive prediction of concurrency 
access anomalies". Here is an example.

```bash
canary -trace-transformer <bitcode_file> -o <output_file>
# link a record version 
clang++ <ouput_file> -o <executable> -ltrace
# a log file will be produced after executing it; using the following command
# to analyze it.  
pecan <log_file> <result_file>
```


Bugs
------

Please help us look for bugs. Please feel free to contact Qingkai.
Email: qingkaishi@gmail.com


