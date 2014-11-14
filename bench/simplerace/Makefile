##===- projects/sample/lib/Makefile ------------------------*- Makefile -*-===##

#
# Relative path to the top of the source tree.
#
LEVEL=../..

all: simplerace.bc

simplerace.bc: simplerace.c
	@clang -c -emit-llvm -g -O2 -fno-vectorize simplerace.c
	

clean:
	@$(RM) -f *.bc *.o *.ll *.exe canary.zip


