/*
	slave ET-client 0xdaem0n

	call args:
	<port>

	Master will contact & tell slave what to do on the supplied port.
	Daemon is programmed very defensively. If not receiving keep-alives
	from master caller it will quit to avoid creating heaps of defuncts.

	CopyRight kobject_
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#include <etdos/protocol.h>
#include <etdos/common.h>
#include <etdos/etdos.h>
#include <etdos/cvar.h>
#include <etdos/plugin.h>

#define SHH (struct sockaddr*)

#define DO_DISCONNECT		if (etdos.connst == CA_ACTIVE) {\
								sv_reliable_cmd("disconnect");\
								send_cmds(1);\
								send_cmds(1);\
								send_cmds(1); }

#define DO_DIE 		{ final_cleanup(); exit(1); }

typedef unsigned short uint16;

extern void send_cmds();
extern void get_packets();
extern void check_resend();

uint16 slave_state = ST_LIMBO;
uint16 slave_role = ROLE_NORMAL;

etdos_t etdos;
clctrl_t ctrl;
pid_t dpid;
struct sockaddr_in si_slave;
struct sockaddr_in si_master;
struct sockaddr_in *sv = NULL;
int sl_sock;
int frametime = 16000;
int req_frametime;

void send_to_master(void *data, int len)
{
	if (slave_state == ST_LIMBO)
		return;

	int ret = sendto(sl_sock, data, len, 0, SHH&si_master, sizeof(si_master));
    if (ret == -1)
		LOG_WARN {emitv("send_to_master ERROR %i\n", errno);}
}

void notify_master(const char *fmt, ...)
{
	if (slave_state == ST_LIMBO)
		return;

	static char msg[2048];
	va_list argptr;

	va_start (argptr, fmt);
	vsnprintf (msg+4, sizeof(msg)-4, fmt, argptr);
	va_end (argptr);

	*(uint16*)msg = SL_NOTIFY;
	*(uint16*)(msg+2) = slave_state + (slave_role << 8);
	LOG_FLOW { emitv("%i %i -> %s\n", slave_state, slave_role, msg+4); }
	send_to_master(msg, strlen(msg+4)+4);
}

void final_cleanup()
{
	if (slave_state != ST_LIMBO) {
		uint16 cmd = SL_CRASH;
		send_to_master(&cmd, 2);
	}

	if (sv)
		free(sv);

	int i;
	for(i=0; i<etdos.npaklist; i++)
		free(etdos.paklist[i].message);
	free(etdos.paklist);

	if (etdos.plugins){
		int plg = 0;
		plugin_t *plugin = &etdos.plugins[plg];
		while (plugin->handle) {
			dlclose(plugin->handle);
			plugin = &etdos.plugins[++plg];
		}
		free(etdos.plugins);
	}
}

/*
	priv init commands included via <...>
	plugins/myplugin.so<cmd=0|cmd1=2>
*/
void load_plugin(char *plugin)
{
	static int nplugins = 0;
	char *initcmd, *a, *b;
	if (!plugin)
		return;

	a = strchr(plugin, '<');
	if (!a)
		initcmd = NULL;
	else {
		b = strchr(plugin, '>');
		if (!b)
			initcmd = NULL;
		else {
			*a = '\0';
			*b = '\0';
			initcmd = strdup(a+1);
		}
	}

	nplugins++;
	etdos.plugins = realloc(etdos.plugins, sizeof(plugin_t)*(nplugins+1));
	plugin_t *p = &etdos.plugins[nplugins-1];

	p->handle = dlopen(plugin, RTLD_LAZY);

	if (!etdos.plugins[nplugins-1].handle)
		DO_DIE

	p->p_preconnect = dlsym(p->handle, "p_preconnect");
	p->p_connected = dlsym(p->handle, "p_connected");
	p->p_gamestate = dlsym(p->handle, "p_gamestate");
	p->p_snapshot = dlsym(p->handle, "p_snapshot");
	p->p_extern_cmd = dlsym(p->handle, "p_extern_cmd");
	p->p_ucmd = dlsym(p->handle, "p_ucmd");
	p->p_init = dlsym(p->handle, "p_init");

	p->phooks.Cvar_Get = Cvar_Get;
	p->phooks.sv_reliable_cmd = sv_reliable_cmd;
	p->phooks.write_packet = write_packet;
	p->phooks.Cvar_InfoString = Cvar_InfoString;
	p->phooks.va = va;
	p->phooks.clients = &etdos.clients[0];
	p->phooks.ps = &etdos.current_ps;
	p->phooks.time = &etdos.time;
	p->phooks.clnum = &etdos.conn.clientNum;

	p->p_init(&p->phooks);
	p->p_extern_cmd(initcmd);

	if (initcmd)
		free(initcmd);

	etdos.plugins[nplugins].handle = 0;
}

void fill_client_infos()
{
	char *s;
	int i;
	for (i=0; i<MAX_CLIENTS; i++)
		etdos.clients[i].valid = 0;

	for (i=0; i<MAX_CLIENTS; i++) {
		s = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS+i];
		if (*s) {
			etdos.clients[i].valid = 1;
			etdos.clients[i].team = atoi(Info_ValueForKey(s, "t"));
		}
	}
}

//vchat 0 0 50 Affirmative
//cpm "^00xdeadbeef^7 disconnected
//print "[lof]^0^1X^0fire^1.^7ShAx^1'^7^7*^1'^7 [lon]entered the game
void callback_server_cmd()
{
	static int last = -1;

	int idx = etdos.conn.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1);
	if (idx != last) {

		if (!strncmp(etdos.conn.serverCommands[idx], "cs ", 3)) {
			int i = atoi(&etdos.conn.serverCommands[idx][3]);
			if (i >= CS_PLAYERS && i < (CS_PLAYERS+MAX_CLIENTS)) {
				fill_client_infos();
				i -= CS_PLAYERS;
				char *s = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS+i];

				if (i == etdos.conn.clientNum) {
					if (*s) notify_master("[ci]%s", s);
				} else if (slave_role == ROLE_SCOUT) {
					if (*s) notify_master("[c][%i]%s", i, s);
				}
			}
			last = etdos.conn.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1);
		}

		if (slave_role == ROLE_SCOUT) {
			if (!strncmp(etdos.conn.serverCommands[idx], "chat ", 5)) {
				notify_master("[chat]%s", (char*)&etdos.conn.serverCommands[idx][5]);
				last = etdos.conn.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1);
			} else if (!strncmp(etdos.conn.serverCommands[idx], "vchat ", 6)) {

				last = etdos.conn.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1);
			} else if (!strncmp(etdos.conn.serverCommands[idx], "cpm ", 4)) {

			} else if (!strncmp(etdos.conn.serverCommands[idx], "cp ", 3)) {

				last = etdos.conn.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1);
			} else if (!strncmp(etdos.conn.serverCommands[idx], "print ", 6)) {

				last = etdos.conn.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1);
			}
		}

		if (!strncmp(etdos.conn.serverCommands[idx], "disconnect", 10)) {
			notify_master("[disconnect]%s", (char*)&etdos.conn.serverCommands[idx][11]);
			last = etdos.conn.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1);
		}
	}

	// immediately fire off an ucmd to ack message - ping hack
	write_packet();
}

void callback_server_print(char*s)
{
	if (!strncmp(s, "[err_dialog]", 12)) {
		notify_master("[error]%s", s+12);
		DO_DIE
	} else if (!strncmp(s, "[err_prot]", 10)) {
		if (ET_protocols[etdos.prot] == 0) {
			notify_master("[error]unknown server protocol", ET_protocols[etdos.prot]);
			DO_DIE
		}
		if (slave_role == ROLE_SCOUT)
			notify_master("[status]switching to protocol %i", ET_protocols[etdos.prot]);

		etdos.proto = ET_protocols[etdos.prot];
		etdos.connst = CA_CONNECTING;
	} else
		notify_master("sv print: %s\n", s);
}

void callback_server_err(char *err)
{
	if (err && *err) {
		notify_master("[error]%s", err);
		DO_DIE
	}
}

void callback_server_disconnect()
{

}

void callback_server_snap()
{
	static int first = 1;
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

		// add self data
		client = &etdos.clients[etdos.conn.clientNum];
		memcpy(&client->origin[0], &cl.snap.ps.origin[0], sizeof(vec3_t));
	}

	// scouts must send back entity data to master for spycam
	if (slave_role == ROLE_SCOUT) {
		char *buf = malloc(4+sizeof(client_t)*MAX_CLIENTS);
  		char *p = buf;

		*(uint16*)p = SL_CLIENTS;
		*(uint16*)(p+2) = slave_state + (slave_role << 8);

		p += 4;

		for (i=0; i<MAX_CLIENTS; i++) {
			if (etdos.clients[i].insnap) {
				client = &etdos.clients[i];
				client->clnum = (char)i;
				memcpy(p, client, sizeof(client_t));
				p += sizeof(client_t);
			}
		}

		//add self
		client = &etdos.clients[etdos.conn.clientNum];
		memcpy(p, client, sizeof(client_t));
		p += sizeof(client_t);

		send_to_master(buf, p-buf);

		free(buf);
	}

	if (first) {
		// we made it trough connect sequence, set rate to requested value
		Cvar_Set2("rate", va("%d", etdos.reqrate), 1);
		char info[MAX_INFO_STRING];
		strncpy(info, Cvar_InfoString(CVAR_USERINFO), sizeof(info));
		sv_reliable_cmd(va("userinfo \"%s\"", info));
		write_packet();
		first = 0;
	}

	// immediately fire off an ucmd to ack message - ping hack
	send_cmds(1);
}

void callback_server_startwwwdl()
{
	int opos;
	int bufsz = 0;
	char *buf = 0;

	buf = malloc(7);
	strcpy(buf, "[www]\xff");
	bufsz += 7;

	if (etdos.www_baseurl[0]) {
		int i = 0;
		char buf2[1024];
		while(etdos.dl_files[i]) {
			strcpy(buf2, etdos.www_baseurl);
			strcat(buf2, etdos.dl_files[i]);
			strcpy(etdos.dl_files[i], buf2);
			i++;
		}
	}

	if (etdos.dl_files) {
		int i = 0;
		while(etdos.dl_files[i]) {
			printf("%s\n", etdos.dl_files[i]);
			opos = bufsz;
			bufsz += strlen(etdos.dl_files[i])+1;
			buf = realloc(buf, bufsz);
			strcpy(buf+opos-1, etdos.dl_files[i]);

			opos = strlen(buf);
			buf[opos] = '\xff';
			buf[opos+1] = '\0';
			i++;
		}
	}

	notify_master("%s", buf);
	free(buf);
	DO_DIE
}

// gamestate
void callback_server_cs()
{
	static char bigstr[16000];
	bigstr[0] = '\0';
	char *s;
	int i;

	notify_master("[cn]%i", etdos.conn.clientNum);

	if (slave_role == ROLE_SCOUT) {
		usleep(10000);

		s = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SYSTEMINFO];
		strcat(bigstr, s);

		s = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
		strcat(bigstr, s);
		notify_master("[svinfo]%s", bigstr);

		for (i=0; i<MAX_CLIENTS; i++) {
			s = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS+i];
			if (*s) {
				usleep(1000);
				notify_master("[c][%i]%s", i, s);
			}
		}
	}

	fill_client_infos();
}

void set_guid(char *s)
{
	if (!strcmp(Info_ValueForKey(s, "cl_guid"), "<random>")) {
		char guid[33];
		sprintf(guid, "%04X%04X%04X%04X%04X%04X%04X%04X",
				randomi(0, 0xffff), randomi(0, 0xffff), randomi(0, 0xffff), randomi(0, 0xffff),
				randomi(0, 0xffff), randomi(0, 0xffff), randomi(0, 0xffff), randomi(0, 0xffff));
		Cvar_Get("cl_guid", guid, CVAR_USERINFO);
	} else
		Cvar_Get ("cl_guid", Info_ValueForKey(s, "cl_guid"), CVAR_USERINFO);
}

// set connection cvars based on info string
void set_ui_cvars(char *s)
{
	etdos.reqrate = atoi(Info_ValueForKey(s, "rate"));
	Cvar_Get ("name", Info_ValueForKey(s, "name"), CVAR_USERINFO);
	Cvar_Get ("rate", Info_ValueForKey(s, "rate"), CVAR_USERINFO);
	Cvar_Get ("snaps", Info_ValueForKey(s, "snaps"), CVAR_USERINFO);
	set_guid(s);
	Cvar_Get ("g_password", Info_ValueForKey(s, "g_password"), CVAR_USERINFO);
	Cvar_Get ("cl_anonymous", Info_ValueForKey(s, "cl_anonymous"), CVAR_USERINFO);
	Cvar_Get ("cl_wwwDownload", "1", CVAR_USERINFO);
	Cvar_Get ("cg_etVersion", Info_ValueForKey(s, "cg_etVersion"), CVAR_USERINFO);
	Cvar_Get ("cg_uinfo", Info_ValueForKey(s, "cg_uinfo"), CVAR_USERINFO);
	Cvar_Get ("maxucmds", Info_ValueForKey(s, "maxucmds"), 0);

	char *p = Info_ValueForKey(s, "protocol");
	if (strlen(p)==0 || *p == '0')
		etdos.proto = ET_protocols[etdos.prot];
	else
		etdos.proto = atoi(p);

	frametime = atoi(Info_ValueForKey(s, "fps"));
	frametime = 1000000/(frametime == 0 ? 100 : frametime);
	if (frametime < 250)
		frametime = 250;
	req_frametime = frametime;
	LOG_WARN { emitv("frametime %i usec\n", frametime); }

	etdos.absucmd = atoi(Info_ValueForKey(s, "absucmds"));

	char *plug = strdup(Info_ValueForKey(s, "plugins"));
	char *k = plug;
	char *l = plug;

	if (plug && *plug) {
		while ((l=strchr(k, ';'))) {
			*l = '\0';
			load_plugin(k);
			k = l+1;
		}
	}
	free(plug);
}

// connection string
// server:port:userinfo string
void connectsv(char *cs)
{
	struct hostent *he;
	char ssv[24];
	char *s;
	int sport;

	s = strchr(cs, ':');
	if (!s) DO_DIE
	memset(ssv, 0, 24);
	memcpy(ssv, cs, s-cs);
	sport = atoi(s+1);
	etdos.net_port = sport;

	s = strchr(s+1, ':');
	if (!s) DO_DIE
	set_ui_cvars(s+1);

	if ((he = gethostbyname(ssv)) == NULL) {
		notify_master("failed to resolve %s", ssv);
		LOG_WARN {emitv("gethostbyname(\"%s\") error %i\n", ssv, errno)}
		sv = NULL;
		return;
	}

	sv = malloc(sizeof(struct sockaddr_in));
	if (!sv)
		DO_DIE

	bcopy(he->h_addr, (char *)&sv->sin_addr, he->h_length);
	sv->sin_family = AF_INET;
	sv->sin_port = htons(sport);
	SockadrToNetadr(sv, &etdos.server);

	etdos.connst = CA_CONNECTING;
	slave_state = ST_CHALLENGING;

	NET_OpenIP();
	Cvar_Get ("qport", va("%i", htons(etdos.net_port)), CVAR_USERINFO);

	// set rate to absolute max (engine controlled) to maximize ping & connect time :)))))
	Cvar_Set2("rate", "90000", 1);
}

int process_mastercmd(void *buf, int len)
{
	int cmd = *(int *)buf;

	switch(cmd)
	{
		default:
			return 0;
		case M_NOP:
		{
			char buf[4];
			*(uint16*)buf = SL_NOP;
			*(uint16*)(buf+2) = slave_state + (slave_role << 8);
			send_to_master(buf, 4);
			break;
		}
		case M_CONNECT:
			LOG_WARN { emitv("connect %s\n", (char*)(buf+4)); }
			connectsv((char*)(buf+4));
			break;
		case M_DIE:
		{
			LOG_WARN { emits("die\n"); }
			DO_DIE;
		}
		case M_SV_DSC:
		{
			LOG_WARN { emits("die/disconnect\n"); }
			DO_DISCONNECT
			DO_DIE
		}
		case M_SV_CMD:
		{
			if (!len)
				break;
			*(char*)(buf+len) = '\0';
			LOG_WARN { emitv("cmd (len=%i, send=%i) %s\n", len, etdos.connst == CA_ACTIVE, (char*)(buf+4)); }
			if (etdos.connst == CA_ACTIVE) {
				if (!strncmp((char*)(buf+4), "name", 4)) {
					char info[MAX_INFO_STRING];
					Cvar_Set2("name", (char*)(buf+5), 1);
					strncpy(info, Cvar_InfoString(CVAR_USERINFO), sizeof(info));
					sv_reliable_cmd(va("userinfo \"%s\"", info));
					write_packet();
					break;
				}

				sv_reliable_cmd(buf+4);
				write_packet();
			}
			break;
		}
		case M_ROLE:
		{
			slave_role = *(uint16*)(buf+4);
			LOG_WARN { emitv("rolechange to %u\n", slave_role); }
			break;
		}
		case M_CFG:
			LOG_WARN { emitv("cfg %s\n", (char*)(buf+4)); }
			break;
		case M_PLUGIN_CMD:
			LOG_WARN { emitv("plugin cmd %s\n", (char*)(buf+4)); }
			if (etdos.plugins){
				int plg = 0;
				plugin_t *plugin = &etdos.plugins[plg];
				while (plugin->handle) {
					plugin->p_extern_cmd((char*)(buf+4));
					plugin = &etdos.plugins[++plg];
				}
			}
			break;

	}

	ctrl.last_mastercmd = etdos.time;

	return 1;
}

// process messages from Master
int master_packets()
{
	static char buf[MAX_MSG_SIZE];
	struct sockaddr_in from;
	socklen_t fromlen;

	fromlen = sizeof(from);
	int ret = recvfrom(sl_sock, buf, MAX_MSG_SIZE, 0, (struct sockaddr *)&from, &fromlen);

	if (!ret)
		return 0;

	if (ret < 0) {
		if (errno == EWOULDBLOCK || errno == ECONNREFUSED)
            return 0;

		LOG_WARN { emits("error\n"); }
		perror("master recv error");
		DO_DIE
	}

	/* after master wakeup ignore packets from other sources */
	if (slave_state > ST_LIMBO && memcmp(&from, &si_master, sizeof(from)))
		return 0;

	if (slave_state == ST_LIMBO && !memcmp(buf, SLAVE_WAKEUP, strlen(SLAVE_WAKEUP))) {
		slave_state = ST_ACK;
		ctrl.master_acktime = etdos.time;
		memcpy(&si_master, &from, sizeof(from));
		send_to_master(SLAVE_ACK, strlen(SLAVE_ACK));

		LOG_WARN {emits("master ack\n");}
		return 1;
	}

	if (slave_state == ST_ACK && !memcmp(buf, MASTER_ACK, strlen(MASTER_ACK))) {
		send_to_master(SLAVE_ACK, strlen(SLAVE_ACK));
		LOG_WARN {emits("associated!\n");}
		slave_state = ST_ASC;
		return 1;
	}

	return process_mastercmd(buf, ret);
}

void timeouts()
{
	if (slave_state == ST_LIMBO && etdos.time > 5000) {
		LOG_WARN {emits("ST_LIMBO\n");}
		DO_DIE
	}

	if (slave_state >= ST_ASC && (etdos.time - ctrl.last_mastercmd) > 5000) {
		LOG_WARN {emits("ST_ASC\n");}
		DO_DISCONNECT
		DO_DIE
	}

	if (slave_state == ST_ACK && (etdos.time - ctrl.master_acktime) > 5000) {
		LOG_WARN {emits("ST_ACK\n");}
		DO_DIE
	}
}

void process_ctrl()
{
	return;

	if (etdos.connst != CA_ACTIVE)
		return;

	if (ctrl.sv_cmd[0]) {
		sv_reliable_cmd(ctrl.sv_cmd);
		send_cmds(1);
		send_cmds(1);
		ctrl.sv_cmd[0] = 0;
	}
}

int main_loop()
{
	while (1)
	{
		etdos.time = Sys_Milliseconds();

		master_packets();

		timeouts();

		if (slave_state >= ST_ASC && sv) {
			int oldstate = etdos.connst;

			// supercharge the main loop to reduce connect ping to an
			// absolute minimum - to avoid "sv_maxping" denials
			if (etdos.connst == CA_CONNECTING || etdos.connst == CA_CHALLANGING)
				frametime = 10;
			else
				frametime = req_frametime;

			send_cmds(0);
			get_packets();
			check_resend();

			// fake level loading
			if (etdos.connst == CA_CONNECTED){
				slave_state = ST_CONNECTING;
				etdos.connst = CA_PRIMED; // ready to send ucmds
			}

			if (oldstate != etdos.connst && etdos.connst == CA_ACTIVE){
				slave_state = ST_JOINED;
				notify_master("[status]connected with clientnum %i", etdos.conn.clientNum);
			}
		}

		usleep(frametime);
	}

	return 0;
}

// just notify Master of our imminent demise
void signal_catcher(int signum)
{
	LOG_WARN {emitv("%i\n", signum);}
	notify_master("caught fatal signal");

	char buf[4];
	*(uint16*)buf=SL_CRASH;
	*(uint16*)(buf+2)=(uint16)signum;
	send_to_master(buf, 4);

	exit(1);
}

// creates a non-blocking udp socket on master supplied
// port for slave <-> master communication
int create_master_sock(int port)
{
	int dummy;

	if ((sl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		LOG_WARN { emits("socket error\n"); }
		perror("socket");
		exit(1);
	}

	// make it non-blocking
	if (ioctl(sl_sock, FIONBIO, &dummy) == -1){
		LOG_WARN { emits("ioctl error\n"); }
		perror("ioctl");
		exit(1);
	}

	memset(&si_slave, 0, sizeof(si_slave));
	si_slave.sin_family = AF_INET;
	si_slave.sin_port = htons(port);
	si_slave.sin_addr.s_addr = INADDR_ANY;

	if (bind(sl_sock, SHH&si_slave, sizeof(si_slave)) == -1){
		LOG_WARN { emits("bind error\n"); }
		perror("bind");
		exit(1);
	}

	return 1;
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

int main(int argc, char **argv)
{
	real_seed();

	int mport;

	if (argc != 2 || strcmp(argv[0], "master")) {
		printf("duh... use etdos_cli instead\n");
		return 1;
	} else
		mport = atoi(argv[1]);

	if (mport < 1024 || mport > 0xfff0) {
		fprintf(stderr, "invalid port, terminating\n");
		return 1;
	}

	#warning <uncomment for debug output>
	char fName[256];
	sprintf(fName, "/tmp/BOT%i-%i.log", rand(), mport);
	dbg_fp = fopen(fName, "w");
	verbosity = 3;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_catcher;

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);

	LOG_WARN { emits("binding... "); }
	if (!create_master_sock(mport)) {
		perror("error setting up feedback socket\n");
		exit(1);
	}
	LOG_WARN { emitv("bound to %i, entering main loop...\n", mport); }

	memset(&etdos, 0, sizeof(etdos_t));
	memset(&ctrl, 0, sizeof(clctrl_t));

	read_paklists();

	return main_loop();
}
