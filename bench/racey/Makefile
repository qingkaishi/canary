##===- projects/sample/lib/Makefile ------------------------*- Makefile -*-===##

#
# Relative path to the top of the source tree.
#
LEVEL=../..

all: racey.bc

racey.bc: racey.cpp
	@clang -c -emit-llvm -g -O2 -fno-vectorize racey.cpp
	

clean:
	@$(RM) -f *.bc *.o *.ll *.exe canary.zip


