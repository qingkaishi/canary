Canary
======

Canary features a unification-based alias analysis (/lib) and related tools (/tools).
We have built and tested them on x86 linux architectures using llvm-12.


Building 
------

```bash
git clone https://github.com/qingkaishi/canary.git
cd canary
mkdir build
cd build
cmake ..
make
```


Using the alias analysis
------

Build and link the lib files to your project and use `DyckAliasAnalysis` as a common Mod pass. 
You may use the following options.

* -print-alias-set-info

This will print the evaluation of alias sets and outputs all alias sets, and their 
relations (dot style).

* -count-fp

Count how many functions that a function pointer may point to.

* -no-function-type-check

If the option is set, we do not check the function FuncTy when resolving pointer
calls, otherwise, only FuncTy compatible function can be aliased with a function
pointer. We say f1 and f2 are two FuncTy-compatible functions iff.

    - Both or netheir of them are var arg function;

    - Both or netheir of them have a non-void return value;

    - Same number of parameters;

    - Same FuncTy store sizes of each pair of parameters;

    - There is an explicit cast operation between FuncTy(f1) and FuncTy(f2) (it works with option -with-function-cast-comb).

* -dot-dyck-callgraph

This option is used to print a call graph based on the alias analysis.
You can use it with -with-labels option, which will add lables (call insts)
to the edges in call graphs.

