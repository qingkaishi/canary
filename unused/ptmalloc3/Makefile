##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../../..

#
# Give the name of a library.  This will build a dynamic version.
#
#LIBRARYNAME=ptmalloc3


#
# Include Makefile.common so we know what to do.
#
#include $(LEVEL)/Makefile.common



all: libptmalloc3.a

libptmalloc3.a: ptmalloc3.o malloc.o
	ar cr libptmalloc3.a ptmalloc3.o malloc.o
	ranlib libptmalloc3.a

ptmalloc3.o: ptmalloc3.c malloc-2.8.3.h
	gcc -c -D_GNU_SOURCE=1 -g -O2  -Wall -Wstrict-prototypes -DUSE_TSD_DATA_HACK -Isysdeps/pthread -Isysdeps/generic -I. -DTHREAD_STATS=1  -DMSPACES=1 ptmalloc3.c

malloc.o: malloc.c
	gcc -c -D_GNU_SOURCE=1 -g -O2  -Wall -Wstrict-prototypes -DUSE_TSD_DATA_HACK -Isysdeps/pthread -Isysdeps/generic -I. -DTHREAD_STATS=1  -DONLY_MSPACES -DUSE_LOCKS=0 malloc.c

clean:
	rm -f *.o *.a
	
install: all
	cp libptmalloc3.a /usr/local/lib/
