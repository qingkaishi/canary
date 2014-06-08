##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

SRC=bm.c pfscan.c pqueue.c version.c
BIT=bm.bc pfscan.bc pqueue.bc version.bc

all: pfscan.bc

pfscan.bc: bm.bc pfscan_orig.bc pqueue.bc version.bc
	@llvm-link bm.bc pfscan_orig.bc pqueue.bc version.bc -o pfscan.bc

bm.bc: bm.c bm.h
	@clang -c -emit-llvm -g -O2 -fno-vectorize bm.c -o bm.bc

pfscan_orig.bc: pfscan.c
	@clang -c -emit-llvm -g -O2 -fno-vectorize pfscan.c -o pfscan_orig.bc

pqueue.bc: pqueue.c pqueue.h
	@clang -c -emit-llvm -g -O2 -fno-vectorize pqueue.c -o pqueue.bc

version.bc:
	@clang -c -emit-llvm -g -O2 -fno-vectorize version.c -o version.bc

clean:
	@$(RM) *.bc *.o *.ll 

install:
	@echo -n ""
