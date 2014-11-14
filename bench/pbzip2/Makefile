##===- projects/sample/lib/Makefile ------------------------*- Makefile -*-===##

#
# Relative path to the top of the source tree.
#
LEVEL=../..

all: pbzip.bc

pbzip.bc: pbzip2.cpp
	@clang -c -emit-llvm -g -O2 -fno-vectorize pbzip2.cpp
	

clean:
	@$(RM) -f *.bc *.o *.ll *.exe canary.zip

