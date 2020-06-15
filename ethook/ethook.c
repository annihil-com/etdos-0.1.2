/*
	ET ucmd hookup (all versions)
	relays ucmds to gui master

	gcc -shared -fPIC -fvisibility=hidden ethook.c -o libethook.so
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>

#define SOCK_PATH "/tmp/etslave_socket"
typedef unsigned int uint;
#define GET_PAGE(addr) ((void *)(((uint)addr) & ~((uint)(getpagesize() - 1))))
#define unprotect(addr) (mprotect(GET_PAGE(addr), getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC))
#define reprotect(addr) (mprotect(GET_PAGE(addr), getpagesize(), PROT_READ | PROT_EXEC))
#define UCMD_SIZE 28

struct sockaddr_un local, remote;

struct {
	char *pattern;
	void *cmd;
	void *print;

	uint *currentcmd;
	char *ucmds;

	void *hooq_offset;
	void *orig_drawscreen;
} et_hooq [] =
{
	// 260
	{"\x89\x14\x24\xe8\x68\xe7\xff\xff", (void*)0x8069240, (void*)0x806a110,
		(uint*)0x906C000, (char*)0x906B900,
		 (void*)0x8141433, (void*)0x8149ec0},

	// 255
	{"\xa1\x30\xc5\x38\x09\xff\xd0\xa1", (void*)0x80b561c, (void*)0x80b5fa4,
		(uint*)0x8ffbea0, (char*)0x8ffb7a0,
		 (void*)0x809fb05, (void*)0x80a7658},

	{ 0, 0, 0 },
};

void (*Com_Printf)(const char *msg, ...);
void (*add_ET_cmd)(const char *cmd_name, void *function) = 0;
void (*orig_screen)();
int connected = 0;
int sock = 0;
uint *last_cmd;
char *ucmds;

void cmd_start_listen()
{
	static int has_init = 0;
	int s, t, len;

	if (has_init)
		return;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, SOCK_PATH);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);
	if (bind(s, (struct sockaddr *)&local, len) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(s, 1) == -1) {
		perror("listen");
		exit(1);
	}

	Com_Printf("Waiting for a connection...(freezes until connected. ^1dont panic^7)\n");
	Com_Printf("Run with in_nograb 1, in windowed mode. While waiting connect via GuiMaster\n");
	Com_Printf("If you did something wrong kill X with ^2CTRL+ALT+BACKSPACE\n");
	orig_screen();
	t = sizeof(remote);
	if ((sock = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
		perror("accept");
		exit(1);
	}

	Com_Printf("Connected.\n");
	connected = 1;
}

void cmd_disconnect()
{
	close(sock);
	connected = 0;
	Com_Printf("Disconnected.\n");
}

void frame()
{
	static int added_funcs = 0;
	if (!added_funcs) {
		add_ET_cmd("hooq_listen", cmd_start_listen);
		add_ET_cmd("hooq_disconnect", cmd_disconnect);
		added_funcs = 1;
	}

	/* pipe the ucmds */
	if (connected) {
		char ucmd[UCMD_SIZE];
		int cur = ((*last_cmd) & 63)*UCMD_SIZE;
		memcpy(ucmd, ucmds+cur, UCMD_SIZE);

		if (send(sock, ucmd, UCMD_SIZE, 0) < 0) {
			Com_Printf("Connection died unexpectedly.\n");
			perror("send");
			connected = 0;
			close(sock);
		}
	}
	orig_screen();
}

void __attribute__ ((constructor)) init()
{
	char link[PATH_MAX];
	memset(link, 0, sizeof(link));
	if (readlink("/proc/self/exe", link, sizeof(link)) <= 0)
		exit(1);
	if (strcmp(basename(link), "et.x86"))
		return;

	int i = 0;
	while (et_hooq[i++].pattern) {
		if (!memcmp((void*)0x80526d0, et_hooq[i-1].pattern, 8)) {
			add_ET_cmd = et_hooq[i-1].cmd;
			Com_Printf = et_hooq[i-1].print;
			orig_screen = et_hooq[i-1].orig_drawscreen;
			last_cmd = et_hooq[i-1].currentcmd;
			ucmds = et_hooq[i-1].ucmds;
			break;
		}
	}

	if (!add_ET_cmd) {
		fprintf(stderr, "not hooqued, unfound :{\n");
		exit(1);
	}

	/* hook draw_screen in com_frame() the lazy way */
	unprotect(et_hooq[i-1].hooq_offset);
	uint targ = (uint)frame - 5 - (uint)et_hooq[i-1].hooq_offset;
	*(uint*)(et_hooq[i-1].hooq_offset+1) = targ;
	reprotect(et_hooq[i-1].hooq_offset);
}
