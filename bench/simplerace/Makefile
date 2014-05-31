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


all:: simplerace.c
	@if [ -f "timestamp.o" ]; then l=`cat timestamp.o`; else l="0"; fi; \
	n=`stat -c %Y simplerace.c`; \
	if [ "$$l" != "$$n" ]; then clang -c -emit-llvm -g -O2 -fno-vectorize simplerace.c; stat -c %Y simplerace.c > timestamp.o; fi
	

clean-local::
	@$(RM) -f *.bc *.o *.ll *.exe canary.zip


