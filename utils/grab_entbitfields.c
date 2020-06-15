/*
	Automatically generate entitystate bitfields
	order specific to ET (2.60)

	gcc -shared -s -fPIC -fvisibility=hidden grab_entbitfields.c -o libhax.so

	Copyright kobject_ @ 2008
*/

#include <stdio.h>
#include <stdlib.h>

// entitystate bitfields
//static void *offset = 0x81b53a0;	//260
static void *offset = 0x8219A40;	//255

// playerstate bitfields
//static void *offset = 0x81b5820;

// 16byte aligned for 260, 12byte aligned for 255
typedef struct
{
	char *type;
	int	offset;
	int fields;
	//int dummy;
} tbl;

void grabit()
{
	tbl *table = (tbl*)offset;
	int i = 0;
	while (1) {
		i++;
		if (table->type == NULL)
			break;
		//printf("%x: %x %x %x\n", (unsigned int)table, (unsigned int)table->type, table->offset, table->fields);
		//printf("{ %s, %i, %i },\n", table->type, table->offset, table->fields);
		printf("{ NETF(%s), %i },\n", table->type, table->fields);

		table = (tbl*)(offset+i*sizeof(tbl));

	}
	exit(1);
}

// using fopen as a quick dirty hook... no intention
// of actually running the game. Just grab it and exit
__attribute__ ((visibility ("default")))
FILE *fopen(const char *path, const char *mode)
{
	grabit();
	exit(1);
}
