/*
	etdos slave & master control protocols
*/

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#ifndef MAX_STRING_CHARS
#define MAX_STRING_CHARS 1024
#endif

#define MAX_MSG_SIZE 32768

/* server disconnect type */
typedef enum
{
	ST_LIMBO,			// in the electronic void... die after sunset
 	ST_ACK,				// rec'vd master cmd
  	ST_ASC,				// associated & ready
   	ST_CHALLENGING,
   	ST_CONNECTING,
	ST_JOINED,
} sl_status_t;

typedef enum
{
	ROLE_NORMAL,		// normal slave
 	ROLE_SCOUT,			// scout - tries first connect & sends servercfg back to master
} sl_role_t;

/* server disconnect type */
typedef enum
{
	SV_NOP,
 	SV_DISCONNECT,
  	SV_KICK
} sv_quit_t;

/* slave commands to master */
typedef enum
{
	SL_NOP,		// keep alive
 	SL_CRASH,	// slave crashed
  	SL_SV,		// server quit
    SL_NOTIFY,	// notification
	SL_CLIENTS,	// client listing (each snap)
} slave_cmd_t;

#define SLAVE_WAKEUP	"Slave, I Command YoU!"
#define MASTER_ACK		"Obey!"
#define SLAVE_ACK		"Yar!"

/* master commands to slave */
typedef enum
{
	M_NOP,			// keep alive
 	M_CONNECT,		// connect to serv. serverip:port:userinfo string
   	M_DIE,			// immediate die
	M_SV_DSC,		// disconnect from server & die
   	M_SV_CMD,		// send cmd to serv
	M_ROLE,			// role change
	M_CFG,			// change cfg setting
 	M_PLUGIN_CMD,	// private plugin cmd
} master_cmd_t;

typedef struct clctrl_s
{
	sv_quit_t	sv_quit;
	char		sv_quit_reason[MAX_STRING_CHARS];
	int			last_mastercmd;
	int			master_acktime;

	char		sv_cmd[MAX_STRING_CHARS];

	char		plugins[4][256];
} clctrl_t;

typedef struct
{
	char clnum;
	int valid;
	int insnap;
	int flags;
	int team;
	float origin[3];
	float velocity[3];
	float angles[3];
} client_t;

#endif
