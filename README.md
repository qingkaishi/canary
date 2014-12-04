Canary
======

Canary is a set of tools built on a unification-based alias analysis.
Relative papers include "Points-to analysis in almost linear time", 
"Fast algorithms for Dyck-CFL-reachability with applications to alias 
analysis", "LEAP: lightweight deterministic multi-processor replay of 
concurrent java programs", "Persuasive prediction of concurrency 
access anomalies", etc. You can read them for details.

Please use canary with llvm-3.6.0svn (If you cannot built it successfully,
plase check out the latest version and try again.).

We have built and tested it on *32-bit* x86 linux architectures using
gcc 4.8.2.

Building Canary
------

```bash
git clone https://github.com/qingkaishi/canary.git
cd canary
scons # >= 2.3.0
sudo scons install
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

* -no-function-type-check
If the option is set, we do not check the function type when resolving pointer
calls, otherwise, only type compatible function can be aliased with a function
pointer.

NOTE: currently, f1 and f2 are two type compatible functions iff.

> Both or netheir of them are var arg function;

> Both or netheir of them have a non-void return value;

> Same number of parameters;

> Same type store sizes of each pair of parameters;

> There is an explicit cast operation between type(f1) and type(f2).

* -dot-dyck-callgraph
This option is used to print a call graph based on the alias analysis.
You can use it with -with-labels option, which will add lables (call insts)
to the edges in call graphs.

* -preserve-dyck-callgraph
Preserve the call graph for later usage. Only using  -dot-dyck-callgraph
will not preserve the call graph.

* -leap-transformer
A transformer for LEAP. Please read ``LEAP: lightweight deterministic 
multi-processor replay of concurrent java programs". Here is an example.

```bash
# transform
canary -preserve-dyck-callgraph -leap-transformer <bitcode_file> -o <output_file>
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
canary -preserve-dyck-callgraph -trace-transformer <bitcode_file> -o <output_file>
# link a record version 
clang++ <ouput_file> -o <executable> -ltrace
# a log file will be produced after executing it; using the following command
# to analyze it.  
pecan <log_file> <result_file>
```

NOTE
-----
Transformers and corresponding supports are not updated in time.

Bugs
------

Please help us look for bugs. Please feel free to contact Qingkai.
Email: qingkaishi@gmail.com

TODO
------
* Upgrade canary:

> 1. support more presice aa of constant array

> 2. performance optimization

* Recover canary record/replay transform pass.
