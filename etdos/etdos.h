#ifndef _ETDOS_H_
#define _ETDOS_H_

#include "common.h"
#include "msg.h"
#include "plugin.h"

/* a few necessary callbacks for important server events */
extern void callback_server_cmd();			// new servercmd has arrived
extern void callback_server_cs();			// new gamestate
extern void callback_server_print(char*);	// print
extern void callback_server_err(char*);		// generic error msg from server
extern void callback_server_snap();			// new snapshot
extern void callback_server_disconnect();	// connectionless, motivated drops are always server cmds
extern void callback_server_startwwwdl();

static const int ET_protocols[] =
{
	82,
 	84,
  	0,
};

#define NETSTATTIME	3000

typedef struct pure_s
{
	char pk3name[64];
	int blen;
	int qvm;
	void *message;
} pure_t;

typedef struct
{
	int time;
	int outsize;
	int insize;
	unsigned int rx;
	unsigned int tx;
} netstat_t;

typedef struct etdos_s
{
	connstate_t			connst;
	clientConnection_t	conn;
	int					time;
	netadr_t			server;
	int					ip_socket;
	int					net_port;
	int					serverTime;
	int					lastCmdTime;
	int					prot;  // protocol idx for automation
	int					proto; // protocol for manual
	int					maxpackets;
	int					recvgamestate;
	int					absucmd;	// absolute max ucmd rate
	int					reqrate;	// requested rate

	playerState_t		current_ps;
	client_t			clients[MAX_CLIENTS];

	// download stuff
	FILE*				dl_handle;
	int					dl_idx;
	int					dl_block;
	int					dl_cnt;
	int					dl_size;
	char				**dl_files;

	pure_t *paklist;
	int npaklist;
	netstat_t netstats;

	int nsv_refs;
	int fs_serverReferencedPaks[MAX_SEARCH_PATHS];
	char *sv_refpaknames[MAX_SEARCH_PATHS];
	char www_baseurl[1024];

	plugin_t* plugins;
} etdos_t;

extern etdos_t etdos;

#endif
