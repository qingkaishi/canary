LEVEL=../..

all: libevent.bc

libevent.bc: event.o buffer.o evbuffer.o log.o evutil.o event_tagging.o http.o evdns.o evrpc.o strlcpy.o select.o poll.o epoll.o signal.o
	@llvm-link  event.o buffer.o evbuffer.o log.o evutil.o event_tagging.o http.o evdns.o evrpc.o strlcpy.o select.o poll.o epoll.o signal.o  -o libevent.bc

event.o: event.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  event.c -o event.o

buffer.o: buffer.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  buffer.c -o buffer.o

evbuffer.o: evbuffer.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  evbuffer.c -o evbuffer.o

log.o: log.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  log.c -o log.o

evutil.o: evutil.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  evutil.c -o evutil.o

event_tagging.o: event_tagging.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  event_tagging.c -o event_tagging.o

http.o: http.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  http.c -o http.o

evdns.o: evdns.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  evdns.c -o evdns.o

evrpc.o: evrpc.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  evrpc.c -o evrpc.o

strlcpy.o: strlcpy.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  strlcpy.c -o strlcpy.o

select.o: select.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  select.c -o select.o

poll.o: poll.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  poll.c -o poll.o

epoll.o: epoll.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  epoll.c -o epoll.o

signal.o: signal.c
	@clang -c -emit-llvm -O2 -fno-vectorize -fno-slp-vectorize -DHAVE_CONFIG_H -I. -I./compat  signal.c -o signal.o

clean:
	@$(RM) *.bc *.o *.ll *.exe

install:
	@echo -n ""

