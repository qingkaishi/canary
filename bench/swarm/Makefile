##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

all: swarm.c
	@clang -c -emit-llvm -g -O2 -fno-vectorize -fno-slp-vectorize swarm.c

clean:
	@$(RM) *.bc *.o *.ll 

install:
	@echo -n ""
