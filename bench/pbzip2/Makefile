##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

all: pbzip2.cpp
	@clang -c -emit-llvm -g -O2 -fno-vectorize pbzip2.cpp

clean:
	@$(RM) *.bc *.o *.ll 

install:
	@echo -n ""
