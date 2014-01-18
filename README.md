Canary
======

Canary is a set of tools built on a unification-based alias analysis.
Relative papers include ``Points-to analysis in almost linear time", 
``Fast algorithms for Dyck-CFL-reachability with applications to alias 
analysis", ``LEAP: lightweight deterministic multi-processor replay of 
concurrent java programs", ``Persuasive prediction of concurrency 
access anomalies", etc. You can read them for detals.



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
opt -load dyckaa.so -dyckaa <bitcode_file> -o <output_file>
```

Some options can be used for different objectives.

1. -dot-may-callgraph
This option is used to print a call graph based on the alias analysis.

2. -leap-transformer
A transformer for LEAP. Please read ``LEAP: lightweight deterministic 
multi-processor replay of concurrent java programs".

You can use the following commands directly to use the function.

```bash
./leap.sh -h
./leap.sh -i <bitcode_file>
```

3. -pecan-transformer
A transformer for Pecan. Please read ``Persuasive prediction of concurrency 
access anomalies"

You can use the following commands directly to use the function.

```bash
./pecan.sh -h
./pecan.sh -i <bitcode_file>
./pecan.sh -a <log_file>
```

4. -inter-aa-eval
This is a modified version of -aa-eval. -aa-eval only can be used to evaluate 
intra-procedure alias analysis. 

5. -count-fp
Count how many functions that a function pointer may point to.


Bugs
------

Please help us look for bugs. Please feel free to contact Qingkai.
Email: qingkaishi@gmail.com


