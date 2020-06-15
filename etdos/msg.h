#ifndef __MSG_H_
#define __MSG_H_

#include "shared.h"

#define PROTOCOL_VERSION 82
#define	PACKET_BACKUP 32
#define	PACKET_MASK (PACKET_BACKUP-1)
#define	MAX_PACKET_USERCMDS 32
#define	PORT_ANY -1
#define	MAX_RELIABLE_COMMANDS 64
#define	MAX_PACKETLEN 1400
#define	FRAGMENT_SIZE (MAX_PACKETLEN - 100)
#define	PACKET_HEADER 10
#define	FRAGMENT_BIT (1<<31)
#define	CL_ENCODE_START 12
#define CL_DECODE_START 4
#define	CMD_BACKUP 64
#define	CMD_MASK (CMD_BACKUP - 1)
#define MAX_SEARCH_PATHS 4096

typedef enum {
	CA_DISCONNECTED,	// inactive
 	CA_CHALLANGING,		// requesting challenge
  	CA_CONNECTING,		// recv'd challenge & awaiting gamestate
   	CA_CONNECTED,		// recv'd gamestate
   	CA_PRIMED,			// send ucmd & wait for snapshot
	CA_ACTIVE			// snap recv'd, all set
} connstate_t;

typedef enum {
	NA_BOT,
	NA_BAD,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX
} netadrtype_t;

typedef struct {
	netadrtype_t type;

	char	ip[4];
	char	ipx[10];

	unsigned short	port;
} netadr_t;

enum svc_ops_e
{
    svc_bad,				// 0
    svc_nop,				// 1
    svc_gamestate,			// 2
    svc_configstring,		// 3
    svc_baseline,			// 4
    svc_serverCommand,		// 5
    svc_download,			// 6
    svc_snapshot,			// 7
    svc_EOF					// 8
};

enum clc_ops_e {
	clc_bad,
	clc_nop,
	clc_move,				// [[usercmd_t]
	clc_moveNoDelta,		// [[usercmd_t]
	clc_clientCommand,		// [string] message
	clc_EOF
};

typedef struct {
	int		p_cmdNumber;		// cl.cmdNumber when packet was sent
	int		p_serverTime;		// usercmd->serverTime when packet was sent
	int		p_realtime;			// cls.realtime when packet was sent
} outPacket_t;

// ET msg_t struct slightly altered ...
typedef struct {
	int		allowoverflow;		// 0
	int		overflowed;			// 4
	int		oob;				// 8
	char	*data;				// 12
	int		maxsize;			// 16
	int		cursize;			// 20
	int		bit_;				// 24
	int		readcount;			// 28
	int		bit;				// 32
} msg_t;

typedef struct {
	int			sock;

	int			dropped;			// between last packet and previous

	netadr_t	remoteAddress;
	int			qport;				// qport value to write when transmitting

	// sequencing variables
	int			incomingSequence;
	int			outgoingSequence;

	// incoming fragment assembly buffer
	int			fragmentSequence;
	int			fragmentLength;
	char		fragmentBuffer[MAX_MSGLEN];

	// outgoing fragment buffer
	// we need to space out the sending of large fragmented messages
	int			unsentFragments;
	int			unsentFragmentStart;
	int			unsentLength;
	char		unsentBuffer[MAX_MSGLEN];
} netchan_t;

typedef struct {

	int			clientNum;
	int			lastPacketSentTime;			// for retransmits during connection
	int			lastPacketTime;				// for timeouts

	int			connectTime;				// for connection retransmits
	int			connectPacketCount;			// for display on connection dialog
	char		serverMessage[MAX_STRING_TOKENS];	// for display on connection dialog

	int			challenge;					// from the server to use for connecting
	int			checksumFeed;				// from the server for checksum calculations

	// these are our reliable messages that go to the server
	int			reliableSequence;
	int			reliableAcknowledge;		// the last one the server has executed
	char		reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	// server message (unreliable) and command (reliable) sequence
	// numbers are NOT cleared at level changes, but continue to
	// increase as long as the connection is valid

	// message sequence is used by both the network layer and the
	// delta compression layer
	int			serverMessageSequence;

	// reliable messages received from server
	int			serverCommandSequence;
	int			lastExecutedServerCommand;		// last server command grabbed or executed with CL_GetServerCommand
	char		serverCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	// file transfer from server
	char		downloadTempName[MAX_OSPATH];
	char		downloadName[MAX_OSPATH];

	int			sv_allowDownload;
	int			downloadNumber;
	int			downloadBlock;	// block we are waiting for
	int			downloadCount;	// how many bytes we got
	int			downloadSize;	// how many bytes we got
	char		downloadList[MAX_INFO_STRING]; // list of paks we need to download
	qboolean	downloadRestart;	// if true, we need to do another FS_Restart because we downloaded a pak

	// big stuff at end of structure so most offsets are 15 bits or less
	netchan_t	netchan;
} clientConnection_t;

typedef struct {
	usercmd_t	cmds[CMD_BACKUP];
	int			cmdNumber;
	outPacket_t	outPackets[PACKET_BACKUP];
	int			serverId;
	int			serverTime;

	int 	parseEntitiesNum;
	qboolean	newSnapshots;
	clSnapshot_t snap;
	gameState_t gameState;
	clSnapshot_t snapshots[PACKET_BACKUP];
	entityState_t entityBaselines[MAX_GENTITIES];
	entityState_t parseEntities[MAX_PARSE_ENTITIES];
} cl_t;

int MSG_ReadBits( msg_t *msg, int bits );
void NET_GetLocalAddress( void );
void Netchan_Setup(netchan_t *chan, netadr_t adr, int qport );
int NET_CompareBaseAdr (netadr_t a, netadr_t b);
const char *NET_AdrToString (netadr_t a);
int NET_CompareAdr (netadr_t a, netadr_t b);
int NET_IsLocalAddress( netadr_t adr );
void NetadrToSockadr (netadr_t *a, struct sockaddr_in *s);
void SockadrToNetadr (struct sockaddr_in *s, netadr_t *a);
void Sys_SendPacket(netadr_t to, int length, const void *data);
void NET_SendPacket(netadr_t to, int length, const void *data);
void NET_OutOfBandPrint(netadr_t adr, const char *format, ... );
void NET_OutOfBandData(netadr_t adr, byte *format, int len);
void MSG_InitOOB( msg_t *buf, byte *data, int length );
void MSG_Bitstream( msg_t *buf );
void MSG_BeginReading( msg_t *msg );
void MSG_BeginReadingOOB( msg_t *msg );

int MSG_ReadByte( msg_t *msg );
int MSG_ReadShort( msg_t *msg );

int MSG_ReadLong( msg_t *msg );
char *MSG_ReadString( msg_t *msg );
char *MSG_ReadBigString( msg_t *msg );
char *MSG_ReadStringLine( msg_t *msg );
void MSG_ReadData( msg_t *msg, void *data, int len );
void MSG_initHuffman( void );
void NET_OpenIP (void);
void MSG_Init( msg_t *buf, byte *data, int length );

void MSG_WriteBits( msg_t *msg, int value, int bits );
void MSG_WriteDelta( msg_t *msg, int oldV, int newV, int bits );
int	MSG_ReadDelta( msg_t *msg, int oldV, int bits );
float MSG_ReadDeltaFloat( msg_t *msg, float oldV );
void MSG_WriteDeltaKey( msg_t *msg, int key, int oldV, int newV, int bits );
int	MSG_ReadDeltaKey( msg_t *msg, int key, int oldV, int bits );

float MSG_ReadDeltaKeyFloat( msg_t *msg, int key, float oldV );
void MSG_WriteDeltaUsercmd( msg_t *msg, usercmd_t *from, usercmd_t *to );
void MSG_WriteDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to );
void MSG_WriteString( msg_t *sb, const char *s );
void MSG_WriteByte( msg_t *sb, int c );
void MSG_WriteLong( msg_t *sb, int c );
void MSG_WriteShort( msg_t *sb, int c );
void MSG_WriteData( msg_t *buf, const void *data, int length );
void MSG_ReadDeltaEntity( msg_t *msg, entityState_t *from, entityState_t *to, int number);
void MSG_ReadDeltaPlayerstate (msg_t *msg, playerState_t *from, playerState_t *to );

void write_packet(void);

void Netchan_Transmit( netchan_t *chan, int length, const byte *data );
void Netchan_TransmitNextFragment( netchan_t *chan );
qboolean CL_Netchan_Process( netchan_t *chan, msg_t *msg );
void CL_Netchan_Transmit( netchan_t *chan, msg_t* msg );
void CL_Netchan_Encode( msg_t *msg );
void CL_DeltaEntity (msg_t *msg, clSnapshot_t *frame, int newnum, entityState_t *old, qboolean unchanged);
void CL_ParsePacketEntities( msg_t *msg, clSnapshot_t *oldframe, clSnapshot_t *newframe);
void sv_reliable_cmd( const char *cmd );

extern cl_t cl;

#endif
