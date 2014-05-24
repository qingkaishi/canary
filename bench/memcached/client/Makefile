# To make the trigger

LIBMEMCACHED_HOME=/usr/local/

CC = gcc
CFLAGS = -g -Wall
INCS = -I$(LIBMEMCACHED_HOME)/include
LDFLAGS = -L$(LIBMEMCACHED_HOME)/lib -Wl,-rpath,$(LIBMEMCACHED_HOME)/lib

all: trigger

trigger: trigger.c
	@$(CC) $(CFLAGS) $(INCS) $(LDFLAGS) -o $@ $< -lpthread -lmemcached

clean:
	@$(RM) -f trigger
