##===- projects/sample/lib/Makefile ------------------------*- Makefile -*-===##

#
# Relative path to the top of the source tree.
#
LEVEL=../..


all: boundedBuffer.bc

boundedBuffer.bc: boundedBuffer.c
	@clang -c -emit-llvm -g -O2 -fno-vectorize boundedBuffer.c
	

clean:
	@$(RM) -f *.bc *.o *.ll *.exe canary.zip

