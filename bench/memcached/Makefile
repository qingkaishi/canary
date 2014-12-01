##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

OBJ=memcached-memcached.o memcached-hash.o memcached-slabs.o memcached-items.o memcached-assoc.o memcached-thread.o memcached-daemon.o memcached-stats.o memcached-util.o memcached-cache.o

all: clients memcached.bc

clients:
	@$(MAKE) -C client

memcached.bc: $(OBJ)
	@llvm-link memcached-memcached.o memcached-hash.o memcached-slabs.o memcached-items.o memcached-assoc.o memcached-thread.o memcached-daemon.o memcached-stats.o memcached-util.o memcached-cache.o -o ./memcached.bc

memcached-memcached.o: memcached.c
	@clang -c -emit-llvm -g  -DHAVE_CONFIG_H -I.  -DNDEBUG -fno-vectorize -fno-slp-vectorize -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-memcached.o ./memcached.c

memcached-hash.o: hash.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -fno-vectorize -fno-slp-vectorize -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-hash.o ./hash.c

memcached-slabs.o: slabs.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -fno-vectorize -fno-slp-vectorize -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-slabs.o ./slabs.c

memcached-items.o: items.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -fno-vectorize -fno-slp-vectorize -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-items.o ./items.c

memcached-assoc.o: assoc.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -fno-vectorize -fno-slp-vectorize -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-assoc.o ./assoc.c

memcached-thread.o: thread.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -fno-vectorize -fno-slp-vectorize -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-thread.o ./thread.c

memcached-daemon.o: daemon.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -fno-vectorize -fno-slp-vectorize -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-daemon.o ./daemon.c

memcached-stats.o: stats.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -fno-vectorize -fno-slp-vectorize -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-stats.o ./stats.c

memcached-util.o: util.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -fno-vectorize -fno-slp-vectorize -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-util.o ./util.c

memcached-cache.o: cache.c
	@clang -c -emit-llvm   -DHAVE_CONFIG_H -I.  -DNDEBUG   -g -O2 -fno-vectorize -fno-slp-vectorize -pthread -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -o memcached-cache.o ./cache.c


clean:
	@$(RM) *.bc *.o *.ll  *.exe
	@$(MAKE) clean -C client

install:
	@echo -n ""
