LEVEL=../..

all:
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  event.c -o event.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  buffer.c -o buffer.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  evbuffer.c -o evbuffer.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  log.c -o log.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  evutil.c -o evutil.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  event_tagging.c -o event_tagging.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  http.c -o http.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  evdns.c -o evdns.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  evrpc.c -o evrpc.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  strlcpy.c -o strlcpy.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  select.c -o select.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  poll.c -o poll.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  epoll.c -o epoll.o
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  signal.c -o signal.o
	@llvm-link  event.o buffer.o evbuffer.o log.o evutil.o event_tagging.o http.o evdns.o evrpc.o strlcpy.o select.o poll.o epoll.o signal.o  -o libevent.bc

clean:
	@$(RM) *.bc *.o *.ll *.exe

install:
	@echo -n ""

