##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

SRC=memcached.c hash.c slabs.c items.c assoc.c thread.c daemon.c stats.c util.c cache.c

all: $(SRC)
	@clang -c -emit-llvm -g -O2 -fno-vectorize  -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-memcached.o ./memcached.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-hash.o ./hash.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-slabs.o ./slabs.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-items.o ./items.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-assoc.o ./assoc.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-thread.o ./thread.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-daemon.o ./daemon.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-stats.o ./stats.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-util.o ./util.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-cache.o ./cache.c
	@llvm-link memcached-memcached.o memcached-hash.o memcached-slabs.o memcached-items.o memcached-assoc.o memcached-thread.o memcached-daemon.o memcached-stats.o memcached-util.o memcached-cache.o -o ./memcached.bc
	@$(MAKE) -C ./client

clean:
	@$(RM) *.bc *.o *.ll ./client/trigger

install:
	@echo -n ""
