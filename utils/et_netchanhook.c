/*
	hooq ET netchan_process() to log some interesting data
	uses malloc as a quick hook()

	yasm -f elf32 netchan.asm -o netchan.o
	gcc -shared -s -fPIC -fvisibility=hidden <this & that>

	Copyright kobject_ @ 2008
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>

typedef unsigned int uint;

#define GET_PAGE(addr) ((void *)(((uint)addr) & ~((uint)(getpagesize() - 1))))
#define unprotect(addr) (mprotect(GET_PAGE(addr), getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC))
#define reprotect(addr) (mprotect(GET_PAGE(addr), getpagesize(), PROT_READ | PROT_EXEC))


extern void lognetchan();

__attribute__ ((visibility ("default")))
void *malloc(size_t size)
{
	static int hooqed = 0;
	static void *(*orig_malloc)(size_t) = NULL;
	if (!orig_malloc)
		orig_malloc = dlsym(RTLD_NEXT, "malloc");

	if (!hooqed) {
		uint targ;
		void *hooq = 0x807e9b7;
		unprotect(hooq);
		memset(hooq, 0x90, 11);
		*(char*)hooq = '\xe9';
		targ = (uint)lognetchan - 5 - (uint)hooq;
		*(uint*)(hooq+1) = targ;
		*(char*)0x807e9c2 = '\xeb';
		reprotect(hooq);
		hooqed=1;
	}

	return orig_malloc(size);
}

