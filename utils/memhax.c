/*
	logs mem alloc/free calls. Most free()/malloc() crashes
  	occur due to regions being overwritten beyond intended boundary
	often hard to trace, this enables speedy lookup of offending region

	gcc -shared -s -fPIC -fvisibility=hidden grab_entbitfields.c -o libhax.so

	Copyright kobject_ @ 2008
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#define CONTAINS(x,s,z) if(  (uint)x <= z && ((uint)x+s) >= z )

typedef unsigned int uint;

uint trace_mem = 0x820d438;

__attribute__ ((visibility ("default")))
void *malloc(size_t size)
{
	uint ret = __builtin_return_address(0);

	static void *(*orig_malloc)(size_t) = NULL;
	if (!orig_malloc)
		orig_malloc = dlsym(RTLD_NEXT, "malloc");

	void *p = orig_malloc(size);

	CONTAINS(p, size, trace_mem) printf("COLLISION! -> ");
	printf("malloc (0x%x): %u 0x%x\n", ret, size, (unsigned int)p);

	return p;
}

__attribute__ ((visibility ("default")))
void *realloc(void *o, size_t size)
{
	uint ret = __builtin_return_address(0);
	static void *(*orig_realloc)(void *,size_t) = NULL;
	if (!orig_realloc)
		orig_realloc = dlsym(RTLD_NEXT, "realloc");

	void *p = orig_realloc(o, size);
	CONTAINS(p, size, trace_mem) printf("COLLISION! -> ");
	printf("realloc (0x%x): %u 0x%x->0x%x\n", ret, size, (unsigned int)o, (unsigned int)p);

	return p;
}

__attribute__ ((visibility ("default")))
void free(void *o)
{
		uint ret = __builtin_return_address(0);
	static void (*orig_free)(void *) = NULL;
	if (!orig_free)
		orig_free = dlsym(RTLD_NEXT, "free");

	printf("free (0x%x): 0x%x\n", ret, (unsigned int)o);
	orig_free(o);
	return;
}
