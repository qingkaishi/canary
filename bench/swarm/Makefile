##===- projects/sample/lib/Makefile ------------------------*- Makefile -*-===##

#
# Relative path to the top of the source tree.
#
LEVEL=../..


all: swarm.bc

swarm.bc: swarm.c
	@clang -c -emit-llvm -g -O2 -fno-vectorize -fno-slp-vectorize swarm.c
	

clean-local::
	@$(RM) -f *.bc *.o *.ll *.exe canary.zip


