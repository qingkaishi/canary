##===- projects/sample/lib/Makefile ------------------------*- Makefile -*-===##

#
# Relative path to the top of the source tree.
#
LEVEL=../..

#
# List all of the subdirectories that we will compile.
#
NO_INSTALL=1

include $(LEVEL)/Makefile.common


all:: pbzip2.cpp
	@if [ -f "pbzip2.o" ]; then l=`cat pbzip2.o`; else l="0"; fi; \
	n=`stat -c %Y pbzip2.cpp`; \
	if [ "$$l" != "$$n" ]; then clang -c -emit-llvm -g -O2 -fno-vectorize pbzip2.cpp; stat -c %Y pbzip2.cpp > pbzip2.o; fi
	

clean-local::
	@$(RM) -f *.bc *.o *.ll *.exe canary.zip

