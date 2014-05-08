##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

SRC=bm.c pfscan.c pqueue.c version.c
BIT=bm.bc pfscan.bc pqueue.bc version.bc

all: $(SRC)
	@clang -c -emit-llvm -g -O2 -fno-vectorize $(SRC)
	@llvm-link $(BIT) -o Pfscan.bc
	@$(RM) pfscan.bc
	@mv Pfscan.bc pfscan.bc

clean:
	@$(RM) *.bc *.o *.ll 

install:
	@echo -n ""
