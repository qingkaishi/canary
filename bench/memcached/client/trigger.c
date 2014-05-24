#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <libmemcached/memcached.h>

const char* g_host = "127.0.0.1";
unsigned short g_port = 11211;
bool g_binary = false;

const char* g_key = "test";
size_t g_key_len = 4;

unsigned int g_loop = 1;
unsigned int g_threads = 2;
unsigned int g_step = 10;

static memcached_st* create_st()
{
	memcached_st* st = memcached_create(NULL);
	if(!st) {
		perror("memcached_create failed");
		exit(1);
	}

	char* hostbuf = strdup(g_host);
	memcached_server_add(st, hostbuf, g_port);
	free(hostbuf);

	if(g_binary) {
		memcached_behavior_set(st, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
	}

	return st;
}

static void* worker(void* trash)
{
	int i;
	uint64_t value;

	memcached_st* st = create_st();

	for(i=0; i < g_loop; ++i) {
		memcached_increment(st, g_key, g_key_len, g_step, &value);
	}

	memcached_free(st);
	return NULL;
}

int main(int argc, char* argv[])
{
	{
		memcached_return ret;
		memcached_st* st = create_st();

		ret = memcached_set(st, g_key, g_key_len, "0",1, 0, 0);
		if(ret != MEMCACHED_SUCCESS) {
			fprintf(stderr, "set failed: %s\n", memcached_strerror(st, ret));
		}

		memcached_free(st);
	}

	{
		int i;
		pthread_t threads[g_threads];

		for(i=0; i < g_threads; ++i) {
			int err = pthread_create(&threads[i], NULL, worker, NULL);
			if(err != 0) {
				fprintf(stderr, "failed to create thread: %s\n", strerror(err));
				exit(1);
			}
		}

		for(i=0; i < g_threads; ++i) {
			void* ret;
			int err = pthread_join(threads[i], &ret);
			if(err != 0) {
				fprintf(stderr, "failed to join thread: %s\n", strerror(err));
			}
		}
	}

	{
		memcached_return ret;
		char* value;
		size_t vallen;
		uint32_t flags;
		memcached_st* st = create_st();

		value = memcached_get(st, g_key, g_key_len,
				&vallen, &flags, &ret);
		if(ret != MEMCACHED_SUCCESS) {
			fprintf(stderr, "get failed: %s\n", memcached_strerror(st, ret));
		}

		printf("expected: %d\n", g_threads * g_loop * g_step);
		printf("result:   %s\n", value);

		memcached_free(st);

                if (g_threads * g_loop * g_step != atoi(value))
			assert(0);
	}

	return 0;
}

