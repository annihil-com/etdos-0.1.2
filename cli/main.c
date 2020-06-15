/*

	Fake ET client console app
	mainly for testing and debugging or analyses of the
	server <-> client communication stuff
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "etdos.h"
#include "cvar.h"

#define ISOPT(x)  (argv[x][0] == '-')
#define GETOPT(x) (ISOPT(x) ? argv[x][1] : 0)

etdos_t etdos;
int dump_netrate = 0;
clctrl_t ctrl;

void callback_server_cmd(){ write_packet(); }
void callback_server_print(char *a)
{
	emitv("fatal_error: %s\n", a);
	exit(1);
}

extern cl_t cl;
void callback_server_snap()
{
	client_t *client;
	entityState_t *ent;
	clSnapshot_t *snap = &cl.snapshots[cl.snap.messageNum & PACKET_MASK];
	int count = snap->numEntities;
	int i;

	for (i=0; i<MAX_CLIENTS; i++)
		etdos.clients[i].insnap = 0;

	for (i=0; i<count; i++) {
		ent = &cl.parseEntities[(snap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES-1)];
		if (ent->number >= 0 && ent->number < MAX_CLIENTS) {
			client = &etdos.clients[ent->number];

			client->insnap = 1;
			client->flags = ent->eFlags;
			memcpy(&client->origin[0], &ent->pos.trBase[0], sizeof(vec3_t));
			memcpy(&client->angles[0], &ent->apos.trBase[0], sizeof(vec3_t));
			memcpy(&client->velocity[0], &ent->pos.trDelta[0], sizeof(vec3_t));
		}
	}

	write_packet();
}

void callback_server_disconnect(){ }
void callback_server_cs(){}
void callback_server_err(char*q)
{
	emitv("serv error: %s\n", q);
	exit(1);
}

extern void reset_dl();

void callback_server_startwwwdl()
{
	printf("Need PK3's:\n");
	if (etdos.dl_files) {
		int i = 0;
		while(etdos.dl_files[i]) {
			printf("%s\n", etdos.dl_files[i]);
			i++;
		}
	}

	exit(1);
}

void read_paklists()
{
	static char sbuf[32768];
	FILE *fp = fopen("data/pk3list.qvm", "r");

	if (!fp) {
		LOG_WARN { emits("couldn't open pk3 checksum list\n"); }
		return;
	}

	int npk = 0;
	etdos.paklist = 0;
	char *s;

	while(fgets(sbuf, sizeof(sbuf), fp)) {
		s = strchr(sbuf, ' ');
		if (!s)
			continue;

		npk++;
		etdos.paklist = (pure_t*)realloc(etdos.paklist, npk*sizeof(pure_t));

		memset(&etdos.paklist[npk-1], 0, sizeof(pure_t));
		if ((s-sbuf) < 64)
			memcpy(etdos.paklist[npk-1].pk3name, sbuf, s-sbuf);
		etdos.paklist[npk-1].message = decode_mem(s+1, &etdos.paklist[npk-1].blen);
		etdos.paklist[npk-1].qvm = 1;
	}

	fclose(fp);
	fp = fopen("data/pk3list.ref", "r");
	while(fgets(sbuf, sizeof(sbuf), fp)) {
		s = strchr(sbuf, ' ');
		if (!s)
			continue;

		npk++;
		etdos.paklist = (pure_t*)realloc(etdos.paklist, npk*sizeof(pure_t));

		memset(&etdos.paklist[npk-1], 0, sizeof(pure_t));
		if ((s-sbuf) < 64)
			memcpy(etdos.paklist[npk-1].pk3name, sbuf, s-sbuf);
		etdos.paklist[npk-1].message = decode_mem(s+1, &etdos.paklist[npk-1].blen);
		etdos.paklist[npk-1].qvm = 0;
	}

	npk++;
	etdos.paklist = (pure_t*)realloc(etdos.paklist, npk*sizeof(pure_t));
	etdos.paklist[npk-1].message = 0;
	etdos.npaklist = npk-1;

	LOG_WARN { emitv("read %i pk3 headers\n", npk-1); }

}

void cvar_init()
{
	Cvar_Get ("name", "ETBOTTY", CVAR_USERINFO);
	Cvar_Get ("rate", "42000", CVAR_USERINFO);
	Cvar_Get ("snaps", "2", CVAR_USERINFO);
	Cvar_Get ("g_password", "", CVAR_USERINFO);
	Cvar_Get ("cl_guid", "haxi", CVAR_USERINFO);
	Cvar_Get ("cl_anonymous", "0", CVAR_USERINFO);
	Cvar_Get ("cl_wwwDownload", "1", CVAR_USERINFO);
	Cvar_Get ("cg_etVersion", "Enemy Territory, ET 2.60", CVAR_USERINFO);
	Cvar_Get ("cg_uinfo", "13 0 30", CVAR_USERINFO);
	Cvar_Get ("maxucmds", "5", 0);

}

void update_stats()
{
	if (etdos.time - etdos.netstats.time < NETSTATTIME)
		return;

	if (dump_netrate) {
		float rx, tx, t;
		t = 1e-3*((float)etdos.time - (float)etdos.netstats.time);
		rx = (float)etdos.netstats.insize/t;
		tx = (float)etdos.netstats.outsize/t;
		rx /= 1024.;
		tx /= 1024.;

		printf("RX: %.1f kbs    TX: %.1f kbs\n", rx, tx);
	}

	etdos.netstats.rx += etdos.netstats.insize;
	etdos.netstats.tx += etdos.netstats.outsize;

	etdos.netstats.time = etdos.time;
	etdos.netstats.outsize = 0;
	etdos.netstats.insize = 0;
}

void final_cleanup()
{
	printf("rec'vd %u bytes, send %u bytes\n", etdos.netstats.rx, etdos.netstats.tx);
}

void process_ctrl()
{
	if (etdos.connst != CA_ACTIVE)
		return;

	if (ctrl.sv_cmd[0]) {
		sv_reliable_cmd(ctrl.sv_cmd);
		write_packet();
		write_packet();
		ctrl.sv_cmd[0] = 0;
	}
}

int main_loop()
{
	static unsigned int framecnt = 0;

	while (++framecnt) {
		etdos.time = Sys_Milliseconds();

		check_resend();

		send_cmds(0);

		get_packets();

		update_stats();

		// fake level loading
		if (etdos.connst == CA_CONNECTED)
			etdos.connst = CA_PRIMED; // ready to send ucmds

		if (ctrl.sv_quit) {
			final_cleanup();
			return 1;
		} else
			process_ctrl();

		usleep(16000);
	}

	return 0;
}

int setup_socket(char *server, int s_port)
{
	int sockfd;
	struct sockaddr_in s_in;
	struct hostent *he;

	if ((he = gethostbyname(server)) == NULL) {
		LOG_WARN {emitv("gethostbyname(\"%s\") error %i\n", server, errno)}
		return 0;
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bcopy(he->h_addr, (char *)&s_in.sin_addr, he->h_length);
	s_in.sin_family = AF_INET;
	s_in.sin_port = htons(s_port);

	if (connect(sockfd, (struct sockaddr *)&s_in, sizeof(s_in))) {
		LOG_WARN {emitv("cant connect to socket, error %i\n", errno);}
		return 0;
	}

	SockadrToNetadr(&s_in, &etdos.server);

	return 1;
}

void print_help()
{
	printf("usage: etdos <opts> -c 1.2.3.4<:27960>\n");
	printf("-v(v(v(...\t\tincrease verbosity\n");
}

extern void NET_GetLocalAddress( void );

int main(int argc, char **argv)
{
//	callback_server_startwwwdl("http://files.game-redirect.com/et//noquarter/RTCW_spirit_v2.pk3", 0);

	dbg_fp = stdout;
	cvar_init();
	srand(time(NULL));

	char server[24];
	int s_port;

	if (argc == 1) {
		print_help();
		return 0;
	}

	memset(&etdos, 0, sizeof(etdos));
	memset(&cl, 0, sizeof(cl_t));

	int c = 0;
	while (++c < argc) {
		switch (GETOPT(c)) {
			case 'c':
				c++;
				char *sep = strchr(argv[c], ':');
				if (!sep) {
					s_port = 27960;
					strcpy(server, argv[c]);
				} else {
					*sep = '\0';
					s_port = atoi(++sep);
					strcpy(server, argv[c]);
				}
				break;
			case 'v':
				while(argv[c][verbosity+1] == 'v')
					verbosity++;
				break;
			case 's':
				dump_netrate = 1;
				break;
		}
	}

	NET_OpenIP();
	Cvar_Get ("qport", va("%i", htons(etdos.net_port)), CVAR_USERINFO);

	if (!setup_socket(server, s_port)) {
		emits("error setting up socket\n");
		return 1;
	}

	etdos.connst = CA_CONNECTING;
	etdos.conn.connectTime = -9999;

	read_paklists();

	etdos.proto = 82;
	int ret = main_loop();

	printf("main_loop() terminated with code %i\n", ret);
	return 0;
}
