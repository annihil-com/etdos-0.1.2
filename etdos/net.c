/*
	ET fake client simulation stuff
	Proper handling of incomming packets,
	server messages, pure checksumming,
	map restarts, cgame simulation, etc...

	Copyright kobject_
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>
#include <libgen.h>

#include "common.h"
#include "msg.h"
#include "huffman.h"
#include "etdos.h"
#include "cvar.h"
#include "plugin.h"

extern int purechecksum(void *, int, int);

byte sys_packetReceived[MAX_MSGLEN];

void reset_dl()
{
	etdos.dl_idx = 0;
	etdos.dl_block = 0;
	etdos.dl_cnt = 0;
	etdos.dl_size = 0;

	if (etdos.dl_files) {
		int i = 0;
		while(etdos.dl_files[i]) {
			free(etdos.dl_files[i]);
			i++;
		}
		free(etdos.dl_files[i]);
		etdos.dl_files = 0;
	}
}

// sends connect packet
void send_connect()
{
	char	info[MAX_INFO_STRING];
	char	data[MAX_INFO_STRING];
	int i;

	etdos.conn.serverMessage[0] = '\0';
	etdos.conn.connectPacketCount = 0;

	etdos.conn.connectTime = Sys_Milliseconds();

	LOG_FLOW {emits("\n");}

	if (etdos.connst == CA_CONNECTING) {
		NET_OutOfBandPrint(etdos.server, "getchallenge");
		if (etdos.plugins){
			int plg = 0;
			plugin_t *plugin = &etdos.plugins[plg];
   			while (plugin->handle) {
				plugin->p_preconnect();
				plugin = &etdos.plugins[++plg];
			}
		}
	} else if (etdos.connst == CA_CHALLANGING) {
		strncpy(info, Cvar_InfoString(CVAR_USERINFO), sizeof(info));
		Info_SetValueForKey( info, "protocol", va("%i", etdos.proto) );
		Info_SetValueForKey( info, "qport", va("%i", htons(etdos.net_port) ) );
		Info_SetValueForKey( info, "challenge", va("%i", etdos.conn.challenge ) );

		strcpy(data, "connect ");
		data[8] = '"';

		for(i=0;i<strlen(info);i++)
			data[9+i] = info[i];

		data[9+i] = '"';
		data[10+i] = 0;

		NET_OutOfBandData(etdos.server, (byte*)&data[0], i+10);
	}
}

void referenced_paks( const char *pakSums, const char *pakNames )
{
	int		i, c, d = 0;

	Cmd_TokenizeString(pakSums);

	c = Cmd_Argc();

	if (c > MAX_SEARCH_PATHS)
		c = MAX_SEARCH_PATHS;

	for ( i = 0 ; i < c ; i++ )
		etdos.fs_serverReferencedPaks[i] = atoi(Cmd_Argv(i));

	for (i = 0 ; i < sizeof(etdos.sv_refpaknames) / sizeof(*etdos.sv_refpaknames); i++){
		if(etdos.sv_refpaknames[i])
			free(etdos.sv_refpaknames[i]);

		etdos.sv_refpaknames[i] = NULL;
	}

	if ( pakNames && *pakNames ) {
		Cmd_TokenizeString( pakNames );

		d = Cmd_Argc();

		if(d > c)
			d = c;

		for ( i = 0 ; i < d ; i++ )
			etdos.sv_refpaknames[i] = strdup(Cmd_Argv(i));
	}

	// ensure that there are as many checksums as there are pak names.
	if(d < c)
		c = d;

	etdos.nsv_refs = c;
}

typedef enum {
	GAME_ETMAIN,
 	GAME_JAYMOD,
  	GAME_ETPUB,
   	GAME_NOQUARTER,
	GAME_UNKNOWN
} game_t;

char* qvm_checksums(const char* game)
{
	int i, k;
	char base[256];
	char etmain[64];
	static char qvm[32];
	pure_t* pk;
	int is_etmain = 0;

	if (!strcmp(game, "etmain") || *game == '\0') {
		if (etdos.proto == 82)
			strcpy(etmain, "etmain/255/mp_bin");
		else
			strcpy(etmain, "etmain/260/mp_bin");

		for(k=0; k<etdos.npaklist; k++) {
			pk = &etdos.paklist[k];
			if (pk->qvm && strstr(pk->pk3name, etmain)) {
				k = purechecksum(pk->message, pk->blen, etdos.conn.checksumFeed);
				LOG_WARN { emitv("pure2: %s\n", pk->pk3name); }
				sprintf(qvm, "%d %d @ ", k, k);
				return &qvm[0];
			}
		}

		return NULL;
	}

	for (i=0; i<etdos.nsv_refs; i++) {
		strcpy(base, basename(etdos.sv_refpaknames[i]));

		for(k=0; k<etdos.npaklist; k++) {
			pk = &etdos.paklist[k];
			if (pk->qvm && strstr(pk->pk3name, etdos.sv_refpaknames[i])) {
				k = purechecksum(pk->message, pk->blen, etdos.conn.checksumFeed);
				LOG_WARN { emitv("pure1: %s\n", pk->pk3name); }
				sprintf(qvm, "%d %d @ ", k, k);
				return &qvm[0];
			}
		}
	}
	return NULL;
}

int is_ingame(char *pk3, char *game)
{
	char *s = strchr(pk3, '/');
	if (!s)
		return 1;

	if (strcmp(game, "tcetest") && strstr(pk3, "etmain/pak"))
		return 1;

	if (etdos.proto == 82 && strstr(pk3, "260")) return 0;

	int ret = 0;
	char *p = strdup(pk3);
	p[s-pk3] = '\0';
	if (!strcmp(p, game)) ret = 1;
	free(p);
	return ret;
}

/*
	Format for pure checksums is
	cp server_id cgame.pk3 ui.pk3 @ ref1.pk3 ref2.pk3 ....
	after first call, free up the big pk3 list and only
	retain referenced pk3s
*/
void send_pure_checksums()
{
	static int first_time = 1;
	int i = 0;
	char *systemInfo;
	const char *s, *t;
	char *qvm;
	char game[256];
	game_t game_type;

	if (!etdos.paklist) {
		LOG_WARN {emits("no pk3 list loaded... cannot send pure checksum\n");}
		return;
	}

	systemInfo = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SYSTEMINFO];
	s = Info_ValueForKey( systemInfo, "sv_referencedPaks" );
	t = Info_ValueForKey( systemInfo, "sv_referencedPakNames" );
	referenced_paks(s, t);

	// dont bother if not pure server
	if (!atoi(Info_ValueForKey(systemInfo, "sv_pure")))
		return;

	strcpy(game, Info_ValueForKey(systemInfo, "fs_game"));
	if (!game[0]) strcpy(game, "etmain");

	qvm = qvm_checksums(game);
	if (!qvm) {
		LOG_WARN { emits("no matching qvm found\n"); }
		sv_reliable_cmd(va("cp %d -1 -1 @ -1 -1", cl.serverId));
		return;
	}

	char base[256];
	int l, k, j, dupes = 0;
	int q = 0;

	char pure[1024];
	char gref[1024];
	gref[0] = 0;
	int checksum = etdos.conn.checksumFeed;
	pure_t* pk;
	pure_t* backup = 0;

	// check for dupes
	int *checksumlst = malloc(etdos.nsv_refs*sizeof(int));
	memset(checksumlst, 0, etdos.nsv_refs*sizeof(int));

	for (i=0; i<etdos.nsv_refs; i++) {
		strcpy(base, basename(etdos.sv_refpaknames[i]));
		strcat(base, ".pk3");
		l = strlen(base);

		for(k=0; k<etdos.npaklist; k++) {
			pk = &etdos.paklist[k];

			if (!is_ingame(pk->pk3name, game))
				continue;

			s = basename(pk->pk3name);

			if (!strncmp(s, base, l)) {
				int pr = purechecksum(pk->message, pk->blen, etdos.conn.checksumFeed);

				// dupes
				for (j=0; j<etdos.nsv_refs; j++) {
					if (!checksumlst[j]) {
						checksumlst[j] = pr;
						break;
					}

					if (checksumlst[j] == pr) {
						dupes++;
						j = -1;
						break;
					}
				}

				if (j>=0) {
					if (first_time) {
						q++;
						backup = realloc(backup, q*sizeof(pure_t));
						memcpy(&backup[q-1], pk, sizeof(pure_t));
						backup[q-1].message = malloc(pk->blen);
						memcpy(backup[q-1].message, pk->message, pk->blen);
					}

					LOG_WARN { emitv("ref: %s\n", pk->pk3name); }

					strcat(gref, va("%d ", pr));
					checksum ^= pr;
					break;
				}
			}
		}

		// pk3 not found in list
		if (k == etdos.npaklist) {
			etdos.dl_idx++;
			if (!etdos.dl_files) {
				etdos.dl_files = malloc(sizeof(char*));
				etdos.dl_files[0] = 0;
			}
			etdos.dl_files = realloc(etdos.dl_files, (etdos.dl_idx+1)*sizeof(char*));
			etdos.dl_files[etdos.dl_idx-1] = malloc(256);
			etdos.dl_files[etdos.dl_idx] = 0;
			strncpy(etdos.dl_files[etdos.dl_idx-1], etdos.sv_refpaknames[i], 256);
			strcat(etdos.dl_files[etdos.dl_idx-1], ".pk3");
		}
	}

	if (first_time) {
		q++;
		backup = realloc(backup, q*sizeof(pure_t));
		backup[q-1].message = 0;
	}

	free(checksumlst);

	//free up the big list
	if (first_time) {
		for(i=0; i<etdos.npaklist; i++)
			free(etdos.paklist[i].message);
		free(etdos.paklist);
		etdos.paklist = backup;
		etdos.npaklist = q;
		first_time = 0;
	}

	if (etdos.dl_idx) {
		etdos.dl_idx = 0;
		sv_reliable_cmd(va("download %s", etdos.dl_files[etdos.dl_idx]));
		write_packet();
		return;
	}

	checksum ^= (etdos.nsv_refs-dupes);
	sprintf(pure, "cp %d %s%s %d", cl.serverId, qvm, gref, checksum);
	LOG_WARN {emitv("%s\n", pure);}
	sv_reliable_cmd(pure);
}

// resend connection stuff
void check_resend()
{
	if (etdos.connst != CA_CONNECTING && etdos.connst != CA_CHALLANGING)
		return;

	if ((etdos.time - etdos.conn.connectTime) < 3000)
		return;

	send_connect();
}

// "connectionless" implies plaintext packet
// usually init stuff
void connectionless_packet(netadr_t *from, msg_t *msg)
{
    char	*s;
    char	*c;

    MSG_BeginReadingOOB( msg );
    MSG_ReadLong( msg );	// skip header

    s = MSG_ReadStringLine( msg );

    Cmd_TokenizeString( s );

    c = Cmd_Argv(0);

    LOG_DATA {emitv("%s: %s\n", NET_AdrToString(*from), c);}

	// challenge from the server we are connecting to
    if ( !Q_stricmp(c, "challengeResponse") ) {
        if ( etdos.connst != CA_CONNECTING ) {
            LOG_WARN {emits( "Unwanted challenge response received.  Ignored.\n" );}
        } else {
	   // start sending challenge repsonse instead of challenge request packets
            etdos.conn.challenge = atoi(Cmd_Argv(1));
            etdos.connst = CA_CHALLANGING;
            etdos.conn.connectPacketCount = 0;
            etdos.conn.connectTime = -99999;

            etdos.server = *from;
            LOG_WARN {emitv("challengeResponse: %d\n", etdos.conn.challenge);}
        }
        return;
    }

	// server connection
    if ( !Q_stricmp(c, "connectResponse") ) {
        if ( etdos.connst >= CA_CONNECTED ) {
            LOG_WARN {emits("Dup connect received.  Ignored.\n");}
            return;
        }
        if ( etdos.connst != CA_CHALLANGING ) {
            LOG_WARN {emits("connectResponse packet while not connecting.  Ignored.\n");}
            return;
        }
        if ( !NET_CompareBaseAdr( *from, etdos.server ) ) {
            LOG_WARN {emits( "connectResponse from a different address.  Ignored.\n" );}
            LOG_WARN {emitv( "%s should have been %s\n", NET_AdrToString( *from ), NET_AdrToString( etdos.server ) );}
            return;
        }

        Netchan_Setup(&etdos.conn.netchan, *from, htons(etdos.net_port));
        etdos.connst = CA_CONNECTED;
        etdos.conn.lastPacketSentTime = -9999;
        return;
    }

	if (!Q_stricmp(c, "disconnect")) {
        if (NET_CompareAdr(*from, etdos.server)) {
			callback_server_disconnect(NULL);
			LOG_WARN {emits( "disconnect\n")}
			ctrl.sv_quit = SV_DISCONNECT;
		}

        return;
    }

	// echo request from server
    if ( !Q_stricmp(c, "echo") ) {
        NET_OutOfBandPrint(*from, "%s", Cmd_Argv(1));
		LOG_WARN {emitv( "echo %s\n", Cmd_Argv(1)); }
        return;
    }

	// global MOTD from id
    if ( !Q_stricmp(c, "motd") ) {
        //CL_MotdPacket( from );
        return;
    }

	// echo request from server
	// NOTE: this is always fatal error msg in ET
    if ( !Q_stricmp(c, "print") ) {
        s = MSG_ReadString( msg );

		// if protocol mismatch try another :)
		if (!strncmp(s, "[err_prot]", 10)) {
			etdos.prot++;
			if (!ET_protocols[etdos.prot]) {
				etdos.connst = CA_DISCONNECTED;
				ctrl.sv_quit = SV_DISCONNECT;
			} else
				etdos.connst = CA_CONNECTING;
			etdos.proto = ET_protocols[etdos.prot];
		}

		callback_server_print(s);
        Q_strncpyz(etdos.conn.serverMessage, s, sizeof(etdos.conn.serverMessage));
        LOG_WARN {emitv("print %s\n", s); }
        return;
    }
}

void parse_gamestate(msg_t *msg)
{
	entityState_t nullstate;
	entityState_t *es;
	int cmd, newnum, i;
	char *s;
	const char *serverInfo;

	etdos.conn.connectPacketCount = 0;
	memset(&cl, 0, sizeof(cl));
	etdos.conn.serverCommandSequence = MSG_ReadLong(msg);
	cl.gameState.dataCount = 1;

	while (1) {
		cmd = MSG_ReadByte( msg );

		if ( cmd == svc_EOF ) {
			break;
		}

		if ( cmd == svc_configstring ) {
			int		len;

			i = MSG_ReadShort(msg);
			if ( i < 0 || i >= MAX_CONFIGSTRINGS )
				LOG_WARN { emits("configstring > MAX_CONFIGSTRINGS" ); }

			s = MSG_ReadBigString(msg);
			len = strlen( s );

			LOG_FLOW { emitv("gamestate %i: %s\n", i, s) }

			if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS )
				LOG_WARN { emits("MAX_GAMESTATE_CHARS exceeded" ); }

			// append it to the gameState string buffer
			cl.gameState.stringOffsets[i] = cl.gameState.dataCount;
			memcpy( cl.gameState.stringData + cl.gameState.dataCount, s, len + 1 );
			cl.gameState.dataCount += len + 1;
		} else if ( cmd == svc_baseline ) {
			newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );
			if ( newnum < 0 || newnum >= MAX_GENTITIES )
				LOG_WARN { emitv("Baseline number out of range: %i", newnum );}

			memset(&nullstate, 0, sizeof(nullstate));
			es = &cl.entityBaselines[newnum];
			MSG_ReadDeltaEntity( msg, &nullstate, es, newnum );
		} else {
			break;
			LOG_WARN { emits("bad command byte" ); }
		}
	}

	etdos.conn.clientNum = MSG_ReadLong(msg);

	LOG_WARN {
		static int info_clientnum = 0;
		if (!info_clientnum) {
			emitv("clientnum %i\n", etdos.conn.clientNum);
			info_clientnum = 1;
		}
	}

	etdos.recvgamestate = 1;
	etdos.conn.checksumFeed = MSG_ReadLong( msg );
	LOG_WARN { emitv("sv_checksumfeed = 0x%x\n", etdos.conn.checksumFeed); }

	serverInfo = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	etdos.conn.sv_allowDownload = atoi(Info_ValueForKey(serverInfo,"sv_allowDownload"));

	char *systemInfo;
	systemInfo = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SYSTEMINFO];
	cl.serverId = atoi(Info_ValueForKey(systemInfo, "sv_serverid"));

	if (etdos.plugins){
		int plg = 0;
		plugin_t *plugin = &etdos.plugins[plg];
  while (plugin->handle) {
			plugin->p_gamestate(&cl.gameState);
			plugin = &etdos.plugins[++plg];
		}
	}
}

void parse_snapshot(msg_t *msg)
{
	int			len;
	clSnapshot_t *old;
	clSnapshot_t newSnap;
	int			deltaNum;
	int			oldMessageNum;
	int			i, packetNum;

	/*
	if (!etdos.recvgamestate) {
		LOG_WARN{ emits("snap before gamestate ??\n"); }
		callback_server_print("snap before gamestate");
		return;
	}*/

	memset(&newSnap, 0, sizeof(newSnap));
	newSnap.serverCommandNum = etdos.conn.serverCommandSequence;
	newSnap.serverTime = MSG_ReadLong(msg);
	etdos.serverTime = newSnap.serverTime;
	newSnap.messageNum = etdos.conn.serverMessageSequence;

	deltaNum = MSG_ReadByte(msg);
	if ( !deltaNum )
		newSnap.deltaNum = -1;
	else
		newSnap.deltaNum = newSnap.messageNum - deltaNum;

	newSnap.snapFlags = MSG_ReadByte(msg);

	if ( newSnap.deltaNum <= 0 ) {
		newSnap.valid = qtrue;
		old = NULL;
	} else {
		old = &cl.snapshots[newSnap.deltaNum & PACKET_MASK];
		if ( !old->valid ) {
			LOG_WARN { emits("Delta from invalid frame (not supposed to happen!).\n"); }
		} else if ( old->messageNum != newSnap.deltaNum ) {
			LOG_WARN { emits("Delta frame too old.\n"); }
		} else if ( cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES-128 ) {
			LOG_WARN { emits("Delta parseEntitiesNum too old.\n"); }
		} else {
			newSnap.valid = qtrue;	// valid delta parse
		}
	}

	// read areamask
	len = MSG_ReadByte(msg);

	if(len > sizeof(newSnap.areamask))
	{
		LOG_WARN { emitv("CL_ParseSnapshot: Invalid size %d for areamask.", len); }
		return;
	}

	MSG_ReadData(msg, &newSnap.areamask, len);

	if ( old ) {
		MSG_ReadDeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	} else {
		MSG_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);
	}

	CL_ParsePacketEntities( msg, old, &newSnap );

	if ( !newSnap.valid ) {
		return;
	}

	oldMessageNum = cl.snap.messageNum + 1;

	if ( newSnap.messageNum - oldMessageNum >= PACKET_BACKUP )
		oldMessageNum = newSnap.messageNum - ( PACKET_BACKUP - 1 );

	for ( ; oldMessageNum < newSnap.messageNum ; oldMessageNum++ )
		cl.snapshots[oldMessageNum & PACKET_MASK].valid = qfalse;

	cl.snap = newSnap;
	cl.snap.ping = 999;

	for ( i = 0 ; i < PACKET_BACKUP ; i++ ) {
		packetNum = ( etdos.conn.netchan.outgoingSequence - 1 - i ) & PACKET_MASK;
		if ( cl.snap.ps.commandTime >= cl.outPackets[packetNum].p_serverTime ) {
			cl.snap.ping = etdos.time - cl.outPackets[packetNum].p_realtime;
			break;
		}
	}

	cl.snapshots[cl.snap.messageNum & PACKET_MASK] = cl.snap;
	memcpy(&etdos.current_ps, &cl.snap.ps, sizeof(playerState_t));
	cl.newSnapshots = qtrue;
}

void parse_config_string(const char *q)
{
	gameState_t	oldGs;
	int i, index;
	char *old, *s;
	char *dup;
	int len;

	Cmd_TokenizeString(q);
	index = atoi(Cmd_Argv(1));

	if (index == 1) {
		int sv_id = atoi(Info_ValueForKey(Cmd_Argv(2), "sv_serverid"));
		if (sv_id)
			cl.serverId = sv_id;
	}
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		return;

	s = (char*)Cmd_ArgsFrom(2);

	old = cl.gameState.stringData + cl.gameState.stringOffsets[index];
	if (!strcmp(old, s))
		return;

	// build the new gameState_t
	oldGs = cl.gameState;

	memset(&cl.gameState, 0, sizeof(cl.gameState));

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;

	for (i = 0; i<MAX_CONFIGSTRINGS; i++) {
		if (i == index)
			dup = s;
		else
			dup = oldGs.stringData + oldGs.stringOffsets[i];

		if (!dup[0])
			continue;

		len = strlen(dup);

		if (len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS)
			return;

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[i] = cl.gameState.dataCount;
		memcpy(cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1);
		cl.gameState.dataCount += len + 1;
	}
}

void parse_server_command(msg_t *msg)
{
	char	*s;
	int		seq;
	int		index;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	if ( etdos.conn.serverCommandSequence >= seq )
		return;

	etdos.conn.serverCommandSequence = seq;

	index = seq & (MAX_RELIABLE_COMMANDS-1);
	Q_strncpyz(etdos.conn.serverCommands[index], s, sizeof(etdos.conn.serverCommands[index]));

	if (!strncmp(s, "cs", 2))
		parse_config_string(s);
	else if (!strncmp(s, "disconnect", 10)) {
		strcpy(ctrl.sv_quit_reason, s+12);
		ctrl.sv_quit = SV_DISCONNECT;
		etdos.connst = CA_DISCONNECTED;
	}
}

/* reversed from et.x86 */
void parse_server_download(msg_t *msg)
{
	int block;
	int size;
	unsigned char data[MAX_MSGLEN];

	if (!etdos.dl_files || !etdos.dl_files[etdos.dl_idx]) {
		sv_reliable_cmd("stopdl");
		write_packet();
		return;
	}

	// read the data
	block = MSG_ReadShort(msg);

	// -1 indicates wwwdl redirect
	// keep sending dl requests for all needed files before ack'ing
	if (block == -1) {
		char *s = MSG_ReadString(msg);
		unsigned int size = MSG_ReadLong(msg);
		unsigned int q = MSG_ReadLong(msg);

		if (etdos.dl_files && etdos.dl_files[etdos.dl_idx] && etdos.dl_files[etdos.dl_idx][0]) {
			char *f = strstr(s, etdos.dl_files[etdos.dl_idx]);
			if (f) {
				strcpy(etdos.www_baseurl, s);
				etdos.www_baseurl[f-s] = '\0';
				sv_reliable_cmd("wwwdl bbl8r");
				write_packet();
				write_packet();
				callback_server_startwwwdl();
				return;
			}
		}

		if (etdos.dl_files && etdos.dl_files[etdos.dl_idx] && etdos.dl_files[etdos.dl_idx][0]) {
			strcpy(etdos.dl_files[etdos.dl_idx], s);
			etdos.dl_idx++;
			etdos.dl_block = 0;
			etdos.dl_cnt = 0;
			etdos.dl_size = 0;
			if (etdos.dl_files[etdos.dl_idx]) {
				sv_reliable_cmd(va("download %s", etdos.dl_files[etdos.dl_idx]));
				write_packet();
				return;
			}
		}

		callback_server_startwwwdl();

		return;
	}

	if (!block){
		etdos.dl_size = MSG_ReadLong(msg);
		LOG_WARN { emitv("download size: %i\n", etdos.dl_size); }

		if (etdos.dl_size < 0) {
			callback_server_err(MSG_ReadString(msg));
			return;
		}
	}

	size = MSG_ReadShort(msg);
	LOG_WARN { emitv("size: %i\n", size); }
	if (size < 0 || size > sizeof(data)) {
		LOG_WARN { emitv("recv'd invalid size %i\n", size); }
		return;
	}

	MSG_ReadData(msg, data, size);
	LOG_WARN { emit_data(data, size); }

	if (etdos.dl_block != block) {
		LOG_WARN { emitv("invalid block, want %i got %i\n", etdos.dl_block, block); }
		return;
	}

	// open the file if not opened yet
	if (!etdos.dl_handle) {
		LOG_WARN { emitv("opening %s\n", etdos.dl_files[etdos.dl_idx]); }
		etdos.dl_handle = fopen(etdos.dl_files[etdos.dl_idx], "w");

		if (!etdos.dl_handle) {
			LOG_WARN("could not create %s for writing\n", etdos.dl_handle);
			sv_reliable_cmd("stopdl");
			write_packet();
			return;
		}
	}

	if (size)
		fwrite(data, size, 1, etdos.dl_handle);

	sv_reliable_cmd(va("nextdl %d", etdos.dl_block));
	etdos.dl_block++;
	etdos.dl_cnt += size;

	// A zero length block means EOF
	if (!size) {
		if (etdos.dl_handle) {
			fclose(etdos.dl_handle);
			etdos.dl_handle = 0;
		}

		if (etdos.dl_files && etdos.dl_files[etdos.dl_idx] && etdos.dl_files[etdos.dl_idx][0]) {
			etdos.dl_idx++;
			etdos.dl_block = 0;
			etdos.dl_cnt = 0;
			etdos.dl_size = 0;
			if (etdos.dl_files[etdos.dl_idx])
				sv_reliable_cmd(va("download %s", etdos.dl_files[etdos.dl_idx]));
		} else
			reset_dl();

		write_packet();
		write_packet();
	}
}

void parse_server_message(msg_t *msg)
{
    int cmd;

    MSG_Bitstream(msg);
    etdos.conn.reliableAcknowledge = MSG_ReadLong(msg);

    if (etdos.conn.reliableAcknowledge < etdos.conn.reliableSequence - MAX_RELIABLE_COMMANDS)
        etdos.conn.reliableAcknowledge = etdos.conn.reliableSequence;

    while ( 1 ) {
        if (msg->readcount > msg->cursize) {
            LOG_WARN {emits("read past end of server message\n");}
            break;
        }

        cmd = MSG_ReadByte(msg);

        if (cmd == svc_EOF)
            break;

		switch ( cmd ) {
			default:
				LOG_WARN {emits("illegible server message\n");}
				break;
			case svc_nop:
				break;
			case svc_serverCommand:
				parse_server_command(msg);
				LOG_WARN {
					int idx = etdos.conn.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1);
					LOG_FLOW {
						emitv("svc_serverCommand %s", etdos.conn.serverCommands[idx]);
					} else {
						if (!strncmp(etdos.conn.serverCommands[idx], "map_restart", 11))
						emits("map_restart()\n");
					}
				}
				callback_server_cmd();
				if (etdos.connst == CA_DISCONNECTED)
					return;
				break;
			case svc_gamestate:
				LOG_FLOW { emits("svc_gamestate\n"); }
				parse_gamestate(msg);
				send_pure_checksums();
				callback_server_cs();
				break;
			case svc_snapshot:
				if (etdos.connst < CA_ACTIVE && etdos.recvgamestate) {
					LOG_WARN {emitv("connected with clientnum %i\n", etdos.conn.clientNum);}
					etdos.connst = CA_ACTIVE;
					if (etdos.plugins){
						int plg = 0;
						plugin_t *plugin = &etdos.plugins[plg];
      					while (plugin->handle) {
							plugin->p_connected();
							plugin = &etdos.plugins[++plg];
						}
					}
				}

				LOG_FLOW { emits("svc_snapshot\n"); }
				parse_snapshot(msg);

				if (etdos.recvgamestate) {
					callback_server_snap();
					if (etdos.plugins){
						int plg = 0;
						plugin_t *plugin = &etdos.plugins[plg];
						while (plugin->handle) {
							plugin->p_snapshot(&cl.snap);
							plugin = &etdos.plugins[++plg];
						}
					}
				}
				break;
			case svc_download:
				LOG_FLOW { emits("svc_download\n"); }
				parse_server_download(msg);
				break;
		}
        //dostuff
    }
}

void process_packet(netadr_t *from, msg_t *msg)
{
    etdos.conn.lastPacketTime = etdos.time;

    if ( msg->cursize >= 4 && *(int *)msg->data == -1 ) {
        connectionless_packet( from, msg );
        return;
    }

    if ( etdos.connst < CA_CONNECTED )
        return;

    if ( msg->cursize < 4 ){
        LOG_WARN {emitv("%s: Runt packet\n", NET_AdrToString(*from));}
        return;
    }

    if (!NET_CompareAdr(*from, etdos.conn.netchan.remoteAddress)) {
        emitv("%s:sequenced packet without connection\n", NET_AdrToString(*from));
        return;
    }

    if (!CL_Netchan_Process(&etdos.conn.netchan, msg))
        return;

    etdos.conn.serverMessageSequence = LittleLong(*(int *)msg->data);

    etdos.conn.lastPacketTime = etdos.time;
    parse_server_message(msg);
}

void get_packets()
{
    msg_t   netmsg;
    struct sockaddr_in	from;
    socklen_t	fromlen;
    netadr_t    fromadr;

	if (etdos.connst == CA_DISCONNECTED)
		return;

    MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );

    fromlen = sizeof(from);
    int ret = recvfrom (etdos.ip_socket, netmsg.data, netmsg.maxsize, 0, (struct sockaddr *)&from, &fromlen);
    SockadrToNetadr (&from, &fromadr);
    netmsg.readcount = 0;

    if (ret == -1)
    {
        if (errno == EWOULDBLOCK || errno == ECONNREFUSED)
            return;
        LOG_WARN {emitv ("recvfrom error %i\n", errno);}
        return;
    }

    if (ret == netmsg.maxsize)
    {
        LOG_WARN {emits("Oversize packet\n");}
        return;
    }

	etdos.netstats.insize += ret;

    netmsg.cursize = ret;

	LOG_LOWLEVEL {emit_data(netmsg.data, netmsg.cursize);}

    process_packet(&fromadr, &netmsg);

    return;
}

usercmd_t CL_CreateCmd()
{
	usercmd_t cmd;
	memset( &cmd, 0, sizeof( cmd ) );
	cmd.serverTime = cl.serverTime;
	return cmd;
}

void CL_CreateNewCommands()
{
	usercmd_t	*cmd;
	int			cmdNum;

	// no need to create usercmds until we have a gamestate
	if ( etdos.connst < CA_PRIMED )
		return;

	cl.cmdNumber++;
	cmdNum = cl.cmdNumber & CMD_MASK;

	if (etdos.plugins){
		int plg = 0;
		plugin_t *plugin = &etdos.plugins[plg];
  while (plugin->handle) {
			cl.cmds[cmdNum] = plugin->p_ucmd();
			plugin = &etdos.plugins[++plg];
		}
		cl.cmds[cmdNum].serverTime = cl.serverTime;
	} else
		cl.cmds[cmdNum] = CL_CreateCmd();

	cmd = &cl.cmds[cmdNum];
	cmd->serverTime = etdos.serverTime;
}

void send_cmds(int force)
{
	static cvar_t *maxpackets = NULL;

	if (etdos.connst < CA_CONNECTED)
		return;

	if (!maxpackets)
		maxpackets = Cvar_Get("maxucmds", "5", 0);

	etdos.lastCmdTime = etdos.time;

	int oldPacketNum = (etdos.conn.netchan.outgoingSequence - 1) & PACKET_MASK;
	int delta = etdos.time - cl.outPackets[oldPacketNum].p_realtime;
	int maxrate;

	if (!etdos.recvgamestate)
		maxrate = 1;
	else
		maxrate = maxpackets->integer > 0 ? maxpackets->integer : 5;

	if (!force && delta < (1000 / maxrate))
			return;

	if (delta < (1000 / (etdos.absucmd+2)))
			return;

	if (etdos.connst < CA_PRIMED)
		return;

	CL_CreateNewCommands();
	write_packet();
}

void sv_download(const char *remote_file)
{
	if (etdos.dl_handle){
		fclose(etdos.dl_handle);
		etdos.dl_handle = 0;
	}

	reset_dl();

	etdos.dl_files = (char**)malloc(2*sizeof(char*));
	etdos.dl_files[0] = (char*)malloc(1024);
	etdos.dl_files[1] = 0;
	strncpy(etdos.dl_files[0], remote_file, 1024);
	sv_reliable_cmd(va("download %s", remote_file));
}
