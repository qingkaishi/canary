# To make the trigger

#LIBMEMCACHED_HOME=/usr/local/

CC = gcc
CFLAGS = -g -Wall
#INCS = -I$(LIBMEMCACHED_HOME)/include
#LDFLAGS = -L$(LIBMEMCACHED_HOME)/lib -Wl,-rpath,$(LIBMEMCACHED_HOME)/lib

all: trigger.exe

trigger.exe: trigger.o
	@gcc $(CFLAGS) -o trigger.exe trigger.o -lpthread -lmemcached

trigger.o: trigger.c
	@gcc $(CFLAGS) -c -o trigger.o trigger.c

clean:
	@$(RM) -f trigger.exe
