Canary
======

Canary is a set of tools built on a unification-based alias analysis.
Relative papers include "Points-to analysis in almost linear time", 
"Fast algorithms for Dyck-CFL-reachability with applications to alias 
analysis", "LEAP: lightweight deterministic multi-processor replay of 
concurrent java programs", "Persuasive prediction of concurrency 
access anomalies", etc. You can read them for detals.

Please use canary with llvm-3.4.



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

Note that the current version does not compile it to .so file. Please DIY 
by changing Makefiles. Some options can be used for different objectives.

* -inter-aa-eval
This is a modified version of -aa-eval. -aa-eval only can be used to evaluate 
intra-procedure alias analysis. 

* -count-fp
Count how many functions that a function pointer may point to.

* -dot-may-callgraph
This option is used to print a call graph based on the alias analysis.

* -leap-transformer
A transformer for LEAP. Please read ``LEAP: lightweight deterministic 
multi-processor replay of concurrent java programs". You can use the following 
commands directly to use the function.

```bash
leap -help
leap <bitcode_file> -o <output_file>
```

* -pecan-transformer
A transformer for Pecan. Please read "Persuasive prediction of concurrency 
access anomalies". You can use the following commands directly to use the 
function.

```bash
pecan -help
pecan <bitcode_file> -o <output_file>
# compile bitcode file to be an executable file; a log file will be produced 
# after executing it; using the following command to analyze it.  
pecan_log_analyzer <log_file> <result_file>
```


Bugs
------

Please help us look for bugs. Please feel free to contact Qingkai.
Email: qingkaishi@gmail.com


