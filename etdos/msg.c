/*	msg.c

	Mostly a selection of Q3 sdk functions that
	take care of server<->client communications

	Some ET specific modifications are applied here
	as found by looking at et.x86 disasm like
	different msg bitfields. Read order is very
	sensitive, as an error in the beginning of the
	stream messes up the rest of the packet.

	Copyright kobject_, iD s0ft.
*/


#include <stdio.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>

#include "common.h"
#include "msg.h"
#include "huffman.h"
#include "etdos.h"

static huffman_t msgHuff;
static int msgInit = 0;
int numIP;
#define MAX_IPS	16
static	char localIP[MAX_IPS][4];

int ip_socket;
int newsize = 0;
int oldsize;
int	overflows;
cl_t cl;

void MSG_initHuffman();

void MSG_Init( msg_t *buf, byte *data, int length )
{
	if (!msgInit)
		MSG_initHuffman();

	memset (buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = length;
}

void NET_GetLocalAddress( void )
{
	char				hostname[256];
	struct hostent		*hostInfo;

	char				*p;
	int					ip;
	int					n;

	if ( gethostname( hostname, 256 ) == -1 ) {
		return;
	}

	hostInfo = gethostbyname( hostname );
	if ( !hostInfo ) {
		return;
	}

	LOG_FLOW {emitv( "Hostname: %s\n", hostInfo->h_name );}
	n = 0;
	while( ( p = hostInfo->h_aliases[n++] ) != NULL ) {
		LOG_FLOW {emitv( "Alias: %s\n", p );}
	}

	if ( hostInfo->h_addrtype != AF_INET ) {
		return;
	}

	numIP = 0;
	while( ( p = hostInfo->h_addr_list[numIP++] ) != NULL && numIP < MAX_IPS ) {
		ip = ntohl( *(int *)p );
		localIP[ numIP ][0] = p[0];
		localIP[ numIP ][1] = p[1];
		localIP[ numIP ][2] = p[2];
		localIP[ numIP ][3] = p[3];
		LOG_FLOW {emitv( "IP: %i.%i.%i.%i\n", ( ip >> 24 ) & 0xff, ( ip >> 16 ) & 0xff, ( ip >> 8 ) & 0xff, ip & 0xff );}
	}
}

int MSG_ReadBits( msg_t *msg, int bits )
{
	int			value;
	int			get;
	qboolean	sgn;
	int			i, nbits;

	value = 0;

	if ( bits < 0 ) {
		bits = -bits;
		sgn = qtrue;
	} else {
		sgn = qfalse;
	}

	if (msg->oob) {
		if (bits==8) {
			value = msg->data[msg->readcount];
			msg->readcount += 1;
			msg->bit += 8;
		} else if (bits==16) {
			unsigned short *sp = (unsigned short *)&msg->data[msg->readcount];
			value = LittleShort(*sp);
			msg->readcount += 2;
			msg->bit += 16;
		} else if (bits==32) {
			unsigned int *ip = (unsigned int *)&msg->data[msg->readcount];
			value = LittleLong(*ip);
			msg->readcount += 4;
			msg->bit += 32;
		} else {
			LOG_WARN {emitv("can't read %d bits\n", bits);}
			return;
		}
	} else {
		nbits = 0;
		if (bits&7) {
			nbits = bits&7;
			for(i=0;i<nbits;i++) {
				value |= (Huff_getBit(msg->data, &msg->bit)<<i);
			}
			bits = bits - nbits;
		}
		if (bits) {
			for(i=0;i<bits;i+=8) {
				Huff_offsetReceive (msgHuff.decompressor.tree, &get, msg->data, &msg->bit);
				value |= (get<<(i+nbits));
			}
		}
		msg->readcount = (msg->bit>>3)+1;
	}
	if ( sgn ) {
		if ( value & ( 1 << ( bits - 1 ) ) ) {
			value |= -1 ^ ( ( 1 << bits ) - 1 );
		}
	}

	return value;
}

int Sys_StringToSockaddr (const char *s, struct sockaddr *sadr)
{
	struct hostent	*h;

	memset (sadr, 0, sizeof(*sadr));
	((struct sockaddr_in *)sadr)->sin_family = AF_INET;

	((struct sockaddr_in *)sadr)->sin_port = 0;

	if ( s[0] >= '0' && s[0] <= '9')
	{
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(s);
	}
	else
	{
		if (! (h = gethostbyname(s)) )
			return 0;
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
	}

	return 1;
}

int NET_IPSocket (char *net_interface, int port)
{
	int newsocket;
	struct sockaddr_in address;
	qboolean _qtrue = qtrue;
	int	i = 1;

	if ( net_interface ) {
		LOG_WARN {emitv("Opening IP socket: %s:%i\n", net_interface, port );}
	} else {
		LOG_WARN {emitv("Opening IP socket: localhost:%i\n", port );}
	}

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		LOG_WARN {emitv ("ERROR: UDP_OpenSocket: socket: %i", errno);}
		return 0;
	}

	// make it non-blocking
	if (ioctl (newsocket, FIONBIO, &_qtrue) == -1)
	{
		LOG_WARN {emitv ("ERROR: UDP_OpenSocket: ioctl FIONBIO:%i\n", errno);}
		return 0;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == -1)
	{
		LOG_WARN {emitv ("ERROR: UDP_OpenSocket: setsockopt SO_BROADCAST:%i\n", errno);}
		return 0;
	}

	if (!net_interface || !net_interface[0] || !strcmp(net_interface, "localhost"))
		address.sin_addr.s_addr = INADDR_ANY;
	else
		Sys_StringToSockaddr (net_interface, (struct sockaddr *)&address);

	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);

	address.sin_family = AF_INET;

	if( bind (newsocket, (void *)&address, sizeof(address)) == -1)
	{
		LOG_WARN {emitv("ERROR: UDP_OpenSocket: bind: %i\n", errno);}
		close (newsocket);
		return 0;
	}

	return newsocket;
}

// open a random port > 28000 to avoid accessing
// priviledged ports
void NET_OpenIP (void)
{
	int		port;
	int		i;

	for ( i = 0 ; i < 100 ; i++ ) {
		port = (28000 + rand()) & 0xffff;
		if (port < 28000) {
			i++;
			continue;
		}

		ip_socket = NET_IPSocket (NULL, port);
		if ( ip_socket ) {
			etdos.ip_socket = ip_socket;
			etdos.net_port = port;
			NET_GetLocalAddress();
			return;
		}
	}
	LOG_WARN {emits("Couldn't allocate IP port");}
}

void Netchan_Setup(netchan_t *chan, netadr_t adr, int qport )
{
    memset (chan, 0, sizeof(*chan));

    chan->remoteAddress = adr;
    chan->qport = qport;
    chan->incomingSequence = 0;
    chan->outgoingSequence = 1;
}

int NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
    if (a.type != b.type)
        return 0;

    if (a.type == NA_LOOPBACK)
        return 1;

    if (a.type == NA_IP)
    {
        if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
            return 1;
        return 0;
    }

    if (a.type == NA_IPX)
    {
        if ((memcmp(a.ipx, b.ipx, 10) == 0))
            return 1;
        return 0;
    }

    LOG_WARN {emits("NET_CompareBaseAdr: bad address type\n");}
    return 0;
}

const char *NET_AdrToString (netadr_t a)
{
    static	char	s[64];

    if (a.type == NA_LOOPBACK) {
        snprintf (s, sizeof(s), "loopback");
    } else if (a.type == NA_BOT) {
        snprintf (s, sizeof(s), "bot");
    } else if (a.type == NA_IP) {
        snprintf (s, sizeof(s), "%i.%i.%i.%i:%hu",
                     a.ip[0], a.ip[1], a.ip[2], a.ip[3], htons(a.port));
    } else {
        snprintf (s, sizeof(s), "%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x:%hu",
                     a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9],
                     htons(a.port));
    }

    return s;
}

int NET_CompareAdr (netadr_t a, netadr_t b)
{
    if (a.type != b.type)
        return 0;

    if (a.type == NA_LOOPBACK)
        return 1;

    if (a.type == NA_IP)
    {
        if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
            return 1;
        return 0;
    }

    if (a.type == NA_IPX)
    {
        if ((memcmp(a.ipx, b.ipx, 10) == 0) && a.port == b.port)
            return 1;
        return 0;
    }

    LOG_WARN {emits("NET_CompareAdr: bad address type\n");}
    return 0;
}

int NET_IsLocalAddress( netadr_t adr )
{
    return adr.type == NA_LOOPBACK;
}

void NetadrToSockadr (netadr_t *a, struct sockaddr_in *s)
{
    memset (s, 0, sizeof(*s));

    if (a->type == NA_BROADCAST)
    {
        s->sin_family = AF_INET;

        s->sin_port = a->port;
        *(int *)&s->sin_addr = -1;
    }
    else if (a->type == NA_IP)
    {
        s->sin_family = AF_INET;

        *(int *)&s->sin_addr = *(int *)&a->ip;
        s->sin_port = a->port;
    }
}

void SockadrToNetadr (struct sockaddr_in *s, netadr_t *a)
{
    *(int *)&a->ip = *(int *)&s->sin_addr;
    a->port = s->sin_port;
    a->type = NA_IP;
}

void Sys_SendPacket(netadr_t to, int length, const void *data)
{
    int ret;
    struct sockaddr_in addr;
    int net_socket;

    if (!etdos.ip_socket)
        return;

    NetadrToSockadr (&to, &addr);

    ret = sendto (etdos.ip_socket, data, length, 0, (struct sockaddr *)&addr, sizeof(addr) );
    if (ret == -1)
    {
		LOG_WARN {emits("Sys_SendPacket ERROR\n");}
    }

	etdos.netstats.outsize += ret;
}

void NET_SendPacket(netadr_t to, int length, const void *data)
{
	Sys_SendPacket(to, length, data);
}

void NET_OutOfBandPrint(netadr_t adr, const char *format, ... )
{
	va_list		argptr;
	char		string[MAX_MSGLEN];

	string[0] = -1;
	string[1] = -1;
	string[2] = -1;
	string[3] = -1;

	va_start( argptr, format );
	vsnprintf( string+4, sizeof(string)-4, format, argptr );
	va_end( argptr );

	NET_SendPacket(adr, strlen( string ), string);
}

void NET_OutOfBandData(netadr_t adr, byte *format, int len)
{
	byte		string[MAX_MSGLEN*2];
	int			i;
	msg_t		mbuf;

	string[0] = 0xff;
	string[1] = 0xff;
	string[2] = 0xff;
	string[3] = 0xff;

	for(i=0;i<len;i++) {
		string[i+4] = format[i];
	}

	mbuf.data = string;
	mbuf.cursize = len+4;
	Huff_Compress( &mbuf, 12);
	NET_SendPacket(adr, mbuf.cursize, mbuf.data);
}

void MSG_InitOOB( msg_t *buf, byte *data, int length )
{
    if (!msgInit) {
        MSG_initHuffman();
    }

    memset (buf, 0, sizeof(*buf));
    buf->data = data;
    buf->maxsize = length;
    buf->oob = qtrue;
}

void MSG_Bitstream( msg_t *buf )
{
    buf->oob = 0;
}

void MSG_BeginReading( msg_t *msg )
{
    msg->readcount = 0;
    msg->bit = 0;
    msg->oob = 0;
}

void MSG_BeginReadingOOB( msg_t *msg )
{
    msg->readcount = 0;
    msg->bit = 0;
    msg->oob = 1;
}

int MSG_ReadByte( msg_t *msg )
{
    int	c;

    c = (unsigned char)MSG_ReadBits( msg, 8 );
    if ( msg->readcount > msg->cursize ) {
        c = -1;
    }
    return c;
}

int MSG_ReadShort( msg_t *msg )
{
    int	c;

    c = (short)MSG_ReadBits( msg, 16 );
    if ( msg->readcount > msg->cursize ) {
        c = -1;
    }

    return c;
}

int MSG_ReadLong( msg_t *msg )
{
    int	c;

    c = MSG_ReadBits( msg, 32 );
    if ( msg->readcount > msg->cursize ) {
        c = -1;
    }

    return c;
}

void CL_DeltaEntity (msg_t *msg, clSnapshot_t *frame, int newnum, entityState_t *old, qboolean unchanged)
{
	entityState_t	*state;
	state = &cl.parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES-1)];

	if ( unchanged )
		*state = *old;
	else
		MSG_ReadDeltaEntity(msg, old, state, newnum);

	if ( state->number == (MAX_GENTITIES-1))
		return;

	cl.parseEntitiesNum++;
	frame->numEntities++;
}

char *MSG_ReadString( msg_t *msg )
{
    static char	string[MAX_STRING_CHARS];
    int		l,c;

    l = 0;
    do {
        c = MSG_ReadByte(msg);
        if ( c == -1 || c == 0 )
            break;

        if ( c == '%' )
            c = '.';

        if ( c > 127 )
            c = '.';

        string[l] = c;
        l++;
    } while (l < sizeof(string)-1);

    string[l] = 0;

    return string;
}

char *MSG_ReadBigString( msg_t *msg )
{
    static char	string[BIG_INFO_STRING];
    int		l,c;

    l = 0;
    do {
        c = MSG_ReadByte(msg);
        if ( c == -1 || c == 0 )
            break;

        if ( c == '%' )
            c = '.';

        if ( c > 127 )
            c = '.';

        string[l] = c;
        l++;
    } while (l < sizeof(string)-1);

    string[l] = 0;

    return string;
}

char *MSG_ReadStringLine( msg_t *msg )
{
    static char	string[MAX_STRING_CHARS];
    int		l,c;

    l = 0;
    do {
        c = MSG_ReadByte(msg);
        if (c == -1 || c == 0 || c == '\n')
            break;

        if ( c == '%' )
            c = '.';

		if ( c > 127 )
            c = '.';

		string[l] = c;
        l++;
    } while (l < sizeof(string)-1);

    string[l] = 0;

    return string;
}

void MSG_ReadData( msg_t *msg, void *data, int len )
{
    int		i;

    for (i=0 ; i<len ; i++) {
        ((byte *)data)[i] = MSG_ReadByte (msg);
    }
}

int msg_hData[256] = {
250315,			// 0
41193,			// 1
6292,			// 2
7106,			// 3
3730,			// 4
3750,			// 5
6110,			// 6
23283,			// 7
33317,			// 8
6950,			// 9
7838,			// 10
9714,			// 11
9257,			// 12
17259,			// 13
3949,			// 14
1778,			// 15
8288,			// 16
1604,			// 17
1590,			// 18
1663,			// 19
1100,			// 20
1213,			// 21
1238,			// 22
1134,			// 23
1749,			// 24
1059,			// 25
1246,			// 26
1149,			// 27
1273,			// 28
4486,			// 29
2805,			// 30
3472,			// 31
21819,			// 32
1159,			// 33
1670,			// 34
1066,			// 35
1043,			// 36
1012,			// 37
1053,			// 38
1070,			// 39
1726,			// 40
888,			// 41
1180,			// 42
850,			// 43
960,			// 44
780,			// 45
1752,			// 46
3296,			// 47
10630,			// 48
4514,			// 49
5881,			// 50
2685,			// 51
4650,			// 52
3837,			// 53
2093,			// 54
1867,			// 55
2584,			// 56
1949,			// 57
1972,			// 58
940,			// 59
1134,			// 60
1788,			// 61
1670,			// 62
1206,			// 63
5719,			// 64
6128,			// 65
7222,			// 66
6654,			// 67
3710,			// 68
3795,			// 69
1492,			// 70
1524,			// 71
2215,			// 72
1140,			// 73
1355,			// 74
971,			// 75
2180,			// 76
1248,			// 77
1328,			// 78
1195,			// 79
1770,			// 80
1078,			// 81
1264,			// 82
1266,			// 83
1168,			// 84
965,			// 85
1155,			// 86
1186,			// 87
1347,			// 88
1228,			// 89
1529,			// 90
1600,			// 91
2617,			// 92
2048,			// 93
2546,			// 94
3275,			// 95
2410,			// 96
3585,			// 97
2504,			// 98
2800,			// 99
2675,			// 100
6146,			// 101
3663,			// 102
2840,			// 103
14253,			// 104
3164,			// 105
2221,			// 106
1687,			// 107
3208,			// 108
2739,			// 109
3512,			// 110
4796,			// 111
4091,			// 112
3515,			// 113
5288,			// 114
4016,			// 115
7937,			// 116
6031,			// 117
5360,			// 118
3924,			// 119
4892,			// 120
3743,			// 121
4566,			// 122
4807,			// 123
5852,			// 124
6400,			// 125
6225,			// 126
8291,			// 127
23243,			// 128
7838,			// 129
7073,			// 130
8935,			// 131
5437,			// 132
4483,			// 133
3641,			// 134
5256,			// 135
5312,			// 136
5328,			// 137
5370,			// 138
3492,			// 139
2458,			// 140
1694,			// 141
1821,			// 142
2121,			// 143
1916,			// 144
1149,			// 145
1516,			// 146
1367,			// 147
1236,			// 148
1029,			// 149
1258,			// 150
1104,			// 151
1245,			// 152
1006,			// 153
1149,			// 154
1025,			// 155
1241,			// 156
952,			// 157
1287,			// 158
997,			// 159
1713,			// 160
1009,			// 161
1187,			// 162
879,			// 163
1099,			// 164
929,			// 165
1078,			// 166
951,			// 167
1656,			// 168
930,			// 169
1153,			// 170
1030,			// 171
1262,			// 172
1062,			// 173
1214,			// 174
1060,			// 175
1621,			// 176
930,			// 177
1106,			// 178
912,			// 179
1034,			// 180
892,			// 181
1158,			// 182
990,			// 183
1175,			// 184
850,			// 185
1121,			// 186
903,			// 187
1087,			// 188
920,			// 189
1144,			// 190
1056,			// 191
3462,			// 192
2240,			// 193
4397,			// 194
12136,			// 195
7758,			// 196
1345,			// 197
1307,			// 198
3278,			// 199
1950,			// 200
886,			// 201
1023,			// 202
1112,			// 203
1077,			// 204
1042,			// 205
1061,			// 206
1071,			// 207
1484,			// 208
1001,			// 209
1096,			// 210
915,			// 211
1052,			// 212
995,			// 213
1070,			// 214
876,			// 215
1111,			// 216
851,			// 217
1059,			// 218
805,			// 219
1112,			// 220
923,			// 221
1103,			// 222
817,			// 223
1899,			// 224
1872,			// 225
976,			// 226
841,			// 227
1127,			// 228
956,			// 229
1159,			// 230
950,			// 231
7791,			// 232
954,			// 233
1289,			// 234
933,			// 235
1127,			// 236
3207,			// 237
1020,			// 238
927,			// 239
1355,			// 240
768,			// 241
1040,			// 242
745,			// 243
952,			// 244
805,			// 245
1073,			// 246
740,			// 247
1013,			// 248
805,			// 249
1008,			// 250
796,			// 251
996,			// 252
1057,			// 253
11457,			// 254
13504,			// 255
};

void MSG_initHuffman( void )
{
	int i,j;

	msgInit = qtrue;
	Huff_Init(&msgHuff);
	for(i=0;i<256;i++) {
		for (j=0;j<msg_hData[i];j++) {
			Huff_addRef(&msgHuff.compressor,	(byte)i);
			Huff_addRef(&msgHuff.decompressor,	(byte)i);
		}
	}
}

qboolean Netchan_Process(netchan_t *chan, msg_t *msg)
{

	int			sequence;
	int			qport;
	int			fragmentStart, fragmentLength;
	qboolean	fragmented;

	// get sequence numbers
	MSG_BeginReadingOOB( msg );
	sequence = MSG_ReadLong( msg );

	// check for fragment information
	if ( sequence & FRAGMENT_BIT ) {
		sequence &= ~FRAGMENT_BIT;
		fragmented = qtrue;
	} else {
		fragmented = qfalse;
	}

	// read the fragment information
	if ( fragmented ) {
		fragmentStart = MSG_ReadShort( msg );
		fragmentLength = MSG_ReadShort( msg );
	} else {
		fragmentStart = 0;
		fragmentLength = 0;
	}

	if (sequence <= chan->incomingSequence) {
		LOG_WARN {emitv("%s:Out of order packet %i at %i\n", NET_AdrToString(chan->remoteAddress), sequence, chan->incomingSequence);}
		return qfalse;
	}

	chan->dropped = sequence - (chan->incomingSequence+1);

	if ( chan->dropped > 0 )
		LOG_FLOW {emitv("%s:Dropped %i packets at %i\n", NET_AdrToString(chan->remoteAddress), chan->dropped, sequence);}

	if ( fragmented ) {
		if ( sequence != chan->fragmentSequence ) {
			chan->fragmentSequence = sequence;
			chan->fragmentLength = 0;
		}

		// if we missed a fragment, dump the message
		if ( fragmentStart != chan->fragmentLength ) {
			LOG_FLOW {emitv("%s:Dropped a message fragment\n", NET_AdrToString( chan->remoteAddress));}
			return qfalse;
		}

		// copy the fragment to the fragment buffer
		if ( fragmentLength < 0 || msg->readcount + fragmentLength > msg->cursize ||
			chan->fragmentLength + fragmentLength > sizeof( chan->fragmentBuffer ) ) {
			LOG_FLOW {emitv("%s:illegal fragment length\n", NET_AdrToString(chan->remoteAddress));}
			return qfalse;
		}

		memcpy(chan->fragmentBuffer + chan->fragmentLength, msg->data + msg->readcount, fragmentLength);
		chan->fragmentLength += fragmentLength;

		// if this wasn't the last fragment, don't process anything
		if (fragmentLength == FRAGMENT_SIZE)
			return qfalse;

		if ( chan->fragmentLength > msg->maxsize ) {
			LOG_WARN {emitv("%s:fragmentLength %i > msg->maxsize\n", NET_AdrToString (chan->remoteAddress), chan->fragmentLength);}
			return qfalse;
		}

		*(int *)msg->data = LittleLong(sequence);
		memcpy(msg->data + 4, chan->fragmentBuffer, chan->fragmentLength);
		msg->cursize = chan->fragmentLength + 4;
		chan->fragmentLength = 0;
		msg->readcount = 4;
		msg->bit = 32;
		chan->incomingSequence = sequence;
		return qtrue;
	}

	chan->incomingSequence = sequence;
	return qtrue;
}

void CL_ParsePacketEntities( msg_t *msg, clSnapshot_t *oldframe, clSnapshot_t *newframe)
{
	int			newnum;
	entityState_t	 *oldstate;
	int			oldindex, oldnum;

	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	oldindex = 0;
	oldstate = NULL;
	if (!oldframe) {
		oldnum = 99999;
	} else {
		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseEntities[(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	while ( 1 ) {
		newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );

		if ( newnum == (MAX_GENTITIES-1) )
			break;

		if ( msg->readcount > msg->cursize )
			LOG_WARN { emits("CL_ParsePacketEntities: end of message\n"); }

		while ( oldnum < newnum ) {
			CL_DeltaEntity(msg, newframe, oldnum, oldstate, qtrue);

			oldindex++;

			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}

		if (oldnum == newnum) {
			CL_DeltaEntity(msg, newframe, newnum, oldstate, qfalse);

			oldindex++;

			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if ( oldnum > newnum ){
			CL_DeltaEntity( msg, newframe, newnum, &cl.entityBaselines[newnum], qfalse );
			continue;
		}
	}

	while ( oldnum != 99999 ) {
		CL_DeltaEntity(msg, newframe, oldnum, oldstate, qtrue);
		oldindex++;
		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseEntities[(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}

static void CL_Netchan_Decode( msg_t *msg )
{
	long reliableAcknowledge, i, index;
	byte key, *string;
	int	srdc, sbit, soob;

	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->oob = 0;

	reliableAcknowledge = MSG_ReadLong(msg);

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	string = (byte *) etdos.conn.reliableCommands[reliableAcknowledge & (MAX_RELIABLE_COMMANDS-1)];
	index = 0;

	// xor the client challenge with the netchan sequence number (need something that changes every message)
	key = etdos.conn.challenge ^ LittleLong( *(unsigned *)msg->data );
	for (i = msg->readcount + CL_DECODE_START; i < msg->cursize; i++) {
		// modify the key with the last sent and with this message acknowledged client command
		if (!string[index])
			index = 0;
		if (string[index] > 127 || string[index] == '%') {
			key ^= '.' << (i & 1);
		}
		else {
			key ^= string[index] << (i & 1);
		}
		index++;
		*(msg->data + i) = *(msg->data + i) ^ key;
	}
}

qboolean CL_Netchan_Process( netchan_t *chan, msg_t *msg )
{
	int ret;

	ret = Netchan_Process( chan, msg );
	if (!ret)
		return qfalse;

	CL_Netchan_Decode( msg );
	newsize += msg->cursize;

	return qtrue;
}

void MSG_WriteBits( msg_t *msg, int value, int bits )
{
	int	i;

	oldsize += bits;

	// this isn't an exact overflow check, but close enough
	if ( msg->maxsize - msg->cursize < 4 ) {
		msg->overflowed = qtrue;
		return;
	}

	if ( bits == 0 || bits < -31 || bits > 32 )
		LOG_WARN {emitv("MSG_WriteBits: bad bits %i", bits);}

	// check for overflows
	if ( bits != 32 ) {
		if ( bits > 0 ) {
			if ( value > ( ( 1 << bits ) - 1 ) || value < 0 ) {
				overflows++;
			}
		} else {
			int	r;

			r = 1 << (bits-1);

			if ( value >  r - 1 || value < -r ) {
				overflows++;
			}
		}
	}

	if ( bits < 0 )
		bits = -bits;

	if (msg->oob) {
		if (bits==8) {
			msg->data[msg->cursize] = value;
			msg->cursize += 1;
			msg->bit += 8;
		} else if (bits==16) {
			unsigned short *sp = (unsigned short *)&msg->data[msg->cursize];
			*sp = LittleShort(value);
			msg->cursize += 2;
			msg->bit += 16;
		} else if (bits==32) {
			unsigned int *ip = (unsigned int *)&msg->data[msg->cursize];
			*ip = LittleLong(value);
			msg->cursize += 4;
			msg->bit += 32;
		} else {
			LOG_WARN {emitv("can't read %d bits\n", bits);}
		}
	} else {
		value &= (0xffffffff>>(32-bits));
		if (bits&7) {
			int nbits;
			nbits = bits&7;
			for(i=0;i<nbits;i++) {
				Huff_putBit((value&1), msg->data, &msg->bit);
				value = (value>>1);
			}
			bits = bits - nbits;
		}
		if (bits) {
			for(i=0;i<bits;i+=8) {
				Huff_offsetTransmit (&msgHuff.compressor, (value&0xff), msg->data, &msg->bit);
				value = (value>>8);
			}
		}
		msg->cursize = (msg->bit>>3)+1;
	}
}

void MSG_WriteDelta( msg_t *msg, int oldV, int newV, int bits )
{
	if ( oldV == newV ) {
		MSG_WriteBits( msg, 0, 1 );
		return;
	}

	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteBits( msg, newV, bits );
}

int	MSG_ReadDelta( msg_t *msg, int oldV, int bits )
{
	if ( MSG_ReadBits( msg, 1 ) )
		return MSG_ReadBits( msg, bits );

	return oldV;
}

float MSG_ReadDeltaFloat( msg_t *msg, float oldV )
{
	if (MSG_ReadBits(msg, 1)) {
		float	newV;

		*(int *)&newV = MSG_ReadBits( msg, 32 );
		return newV;
	}

	return oldV;
}

int kbitmask[32] = {
	0x00000001, 0x00000003, 0x00000007, 0x0000000F,
	0x0000001F,	0x0000003F,	0x0000007F,	0x000000FF,
	0x000001FF,	0x000003FF,	0x000007FF,	0x00000FFF,
	0x00001FFF,	0x00003FFF,	0x00007FFF,	0x0000FFFF,
	0x0001FFFF,	0x0003FFFF,	0x0007FFFF,	0x000FFFFF,
	0x001FFFFf,	0x003FFFFF,	0x007FFFFF,	0x00FFFFFF,
	0x01FFFFFF,	0x03FFFFFF,	0x07FFFFFF,	0x0FFFFFFF,
	0x1FFFFFFF,	0x3FFFFFFF,	0x7FFFFFFF,	0xFFFFFFFF,
};

void MSG_WriteDeltaKey( msg_t *msg, int key, int oldV, int newV, int bits )
{
	if ( oldV == newV ) {
		MSG_WriteBits(msg, 0, 1);
		return;
	}
	MSG_WriteBits(msg, 1, 1);
	MSG_WriteBits(msg, newV ^ key, bits);
}

int	MSG_ReadDeltaKey( msg_t *msg, int key, int oldV, int bits )
{
	if (MSG_ReadBits(msg, 1))
		return MSG_ReadBits( msg, bits ) ^ (key & kbitmask[bits]);

	return oldV;
}

typedef struct {
	char	*name;
	int		offset;
	int		bits;
} netField_t;

#define	NETF(x) #x,(int)&((entityState_t*)0)->x
#define	FLOAT_INT_BITS	13
#define	FLOAT_INT_BIAS	(1<<(FLOAT_INT_BITS-1))

netField_t	entityStateFields[] =
{
{ NETF(eType), 8 },
{ NETF(eFlags), 24 },
{ NETF(pos.trType), 8 },
{ NETF(pos.trTime), 32 },
{ NETF(pos.trDuration), 32 },
{ NETF(pos.trBase[0]), 0 },
{ NETF(pos.trBase[1]), 0 },
{ NETF(pos.trBase[2]), 0 },
{ NETF(pos.trDelta[0]), 0 },
{ NETF(pos.trDelta[1]), 0 },
{ NETF(pos.trDelta[2]), 0 },
{ NETF(apos.trType), 8 },
{ NETF(apos.trTime), 32 },
{ NETF(apos.trDuration), 32 },
{ NETF(apos.trBase[0]), 0 },
{ NETF(apos.trBase[1]), 0 },
{ NETF(apos.trBase[2]), 0 },
{ NETF(apos.trDelta[0]), 0 },
{ NETF(apos.trDelta[1]), 0 },
{ NETF(apos.trDelta[2]), 0 },
{ NETF(time), 32 },
{ NETF(time2), 32 },
{ NETF(origin[0]), 0 },
{ NETF(origin[1]), 0 },
{ NETF(origin[2]), 0 },
{ NETF(origin2[0]), 0 },
{ NETF(origin2[1]), 0 },
{ NETF(origin2[2]), 0 },
{ NETF(angles[0]), 0 },
{ NETF(angles[1]), 0 },
{ NETF(angles[2]), 0 },
{ NETF(angles2[0]), 0 },
{ NETF(angles2[1]), 0 },
{ NETF(angles2[2]), 0 },
{ NETF(otherEntityNum), 10 },
{ NETF(otherEntityNum2), 10 },
{ NETF(groundEntityNum), 10 },
{ NETF(loopSound), 8 },
{ NETF(constantLight), 32 },
{ NETF(dl_intensity), 32 },
{ NETF(modelindex), 9 },
{ NETF(modelindex2), 9 },
{ NETF(frame), 16 },
{ NETF(clientNum), 8 },
{ NETF(solid), 24 },
{ NETF(event), 10 },
{ NETF(eventParm), 8 },
{ NETF(eventSequence), 8 },
{ NETF(events[0]), 8 },
{ NETF(events[1]), 8 },
{ NETF(events[2]), 8 },
{ NETF(events[3]), 8 },
{ NETF(eventParms[0]), 8 },
{ NETF(eventParms[1]), 8 },
{ NETF(eventParms[2]), 8 },
{ NETF(eventParms[3]), 8 },
{ NETF(powerups), 16 },
{ NETF(weapon), 8 },
{ NETF(legsAnim), 10 },
{ NETF(torsoAnim), 10 },
{ NETF(density), 10 },
{ NETF(dmgFlags), 32 },
{ NETF(onFireStart), 32 },
{ NETF(onFireEnd), 32 },
{ NETF(nextWeapon), 8 },
{ NETF(teamNum), 8 },
{ NETF(effect1Time), 32 },
{ NETF(effect2Time), 32 },
{ NETF(effect3Time), 32 },
{ NETF(animMovetype), 4 },
{ NETF(aiState), 2 }
};

void MSG_ReadDeltaEntity( msg_t *msg, entityState_t *from, entityState_t *to, int number)
{
	int			i, lc;
	int			numFields;
	netField_t	*field;
	int			*fromF, *toF;
	int			print;
	int			trunc;
	int			startBit, endBit;

	if ( number < 0 || number >= MAX_GENTITIES)
		LOG_WARN {emitv("Bad delta entity number: %i", number);}

	if ( msg->bit == 0 )
		startBit = msg->readcount * 8 - GENTITYNUM_BITS;
	else
		startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;

	if ( MSG_ReadBits( msg, 1 ) == 1 ) {
		memset( to, 0, sizeof( *to ) );
		to->number = MAX_GENTITIES - 1;
		return;
	}

	if ( MSG_ReadBits( msg, 1 ) == 0 ) {
		*to = *from;
		to->number = number;
		return;
	}

	numFields = sizeof(entityStateFields)/sizeof(entityStateFields[0]);
	lc = MSG_ReadByte(msg);
	to->number = number;

	for ( i = 0, field = entityStateFields ; i < lc ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( ! MSG_ReadBits( msg, 1 ) ) {
			*toF = *fromF;
		} else {
			if ( field->bits == 0 ) {
				if ( MSG_ReadBits( msg, 1 ) == 0 ) {
						*(float *)toF = 0.0f;
				} else {
					if ( MSG_ReadBits( msg, 1 ) == 0 ) {
						trunc = MSG_ReadBits( msg, FLOAT_INT_BITS );
						trunc -= FLOAT_INT_BIAS;
						*(float *)toF = trunc;
					} else {
						*toF = MSG_ReadBits( msg, 32 );
					}
				}
			} else {
				if ( MSG_ReadBits( msg, 1 ) == 0 ) {
					*toF = 0;
				} else {
					*toF = MSG_ReadBits( msg, field->bits );
				}
			}
		}
	}
	for ( i = lc, field = &entityStateFields[lc] ; i < numFields ; i++, field++ ) {
		if (i < 0)
			break;

		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		*toF = *fromF;
	}
}

float MSG_ReadDeltaKeyFloat( msg_t *msg, int key, float oldV )
{
	if (MSG_ReadBits(msg, 1)) {
		float	newV;

		*(int *)&newV = MSG_ReadBits( msg, 32 ) ^ key;
		return newV;
	}
	return oldV;
}

// ms is allways sent, the others are optional
#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE2 	(1<<1)
#define	CM_ANGLE3 	(1<<2)
#define	CM_FORWARD	(1<<3)
#define	CM_SIDE		(1<<4)
#define	CM_UP		(1<<5)
#define	CM_BUTTONS	(1<<6)
#define CM_WEAPON	(1<<7)

/* Quite different from Q3 sdk, reversed from ET.x86 */
void MSG_WriteDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to )
{
	int i;

	if ( to->serverTime - from->serverTime < 256 ) {
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, to->serverTime - from->serverTime, 8);
	} else {
		MSG_WriteBits(msg, 0, 1);
		MSG_WriteBits(msg, to->serverTime, 32);
	}

	if (from->angles[0] == to->angles[0] &&
		from->angles[1] == to->angles[1] &&
		from->angles[2] == to->angles[2] &&
		from->buttons == to->buttons &&
		from->wbuttons == to->wbuttons &&
		from->weapon == to->weapon &&
		from->flags == to->flags &&
		from->forwardmove == to->forwardmove &&
		from->rightmove == to->rightmove &&
		from->upmove == to->upmove &&
		from->doubleTap == to->doubleTap &&
		from->identClient == to->identClient
		) {
			MSG_WriteBits(msg, 0, 1);
			oldsize += 7;
			return;
	}

	key ^= to->serverTime;
	MSG_WriteBits(msg, 1, 1);

	for (i=0; i<3; i++) {
		if ( from->angles[i] == to->angles[i] )
			MSG_WriteBits(msg, 0, 1);
		else {
			MSG_WriteBits(msg, 1, 1);
			MSG_WriteBits(msg, key ^ to->angles[i], 16);
		}
	}

	if ( from->forwardmove == to->forwardmove )
		MSG_WriteBits(msg, 0, 1);
	else {
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, key ^ to->forwardmove, 8);
	}

	if ( from->rightmove == to->rightmove )
		MSG_WriteBits(msg, 0, 1);
	else {
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, key ^ to->rightmove, 8);
	}

	if ( from->upmove == to->upmove )
		MSG_WriteBits(msg, 0, 1);
	else {
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, key ^ to->upmove, 8);
	}

	if ( from->buttons == to->buttons )
		MSG_WriteBits(msg, 0, 1);
	else {
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, key ^ to->buttons, 8);
	}

	if ( from->wbuttons == to->wbuttons )
		MSG_WriteBits(msg, 0, 1);
	else {
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, key ^ to->wbuttons, 8);
	}

	if ( from->weapon == to->weapon )
		MSG_WriteBits(msg, 0, 1);
	else {
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, key ^ to->weapon, 8);
	}

	if ( from->flags == to->flags )
		MSG_WriteBits(msg, 0, 1);
	else {
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, key ^ to->flags, 8);
	}

	if ( from->doubleTap == to->doubleTap )
		MSG_WriteBits(msg, 0, 1);
	else {
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, key ^ to->doubleTap, 3);
	}

	if ( from->identClient == to->identClient )
		MSG_WriteBits(msg, 0, 1);
	else {
		MSG_WriteBits(msg, 1, 1);
		MSG_WriteBits(msg, key ^ to->identClient, 8);
	}
}

#define	PSF(x) #x,(int)&((playerState_t*)0)->x

netField_t	playerStateFields[] =
{
{ PSF(commandTime), 32 },
{ PSF(pm_type), 8 },
{ PSF(bobCycle), 8 },
{ PSF(pm_flags), 16 },
{ PSF(pm_time), -16 },
{ PSF(origin[0]), 0 },
{ PSF(origin[1]), 0 },
{ PSF(origin[2]), 0 },
{ PSF(velocity[0]), 0 },
{ PSF(velocity[1]), 0 },
{ PSF(velocity[2]), 0 },
{ PSF(weaponTime), -16 },
{ PSF(weaponDelay), -16 },
{ PSF(grenadeTimeLeft), -16 },
{ PSF(gravity), 16 },
{ PSF(leanf), 0 },
{ PSF(speed), 16 },
{ PSF(delta_angles[0]), 16 },
{ PSF(delta_angles[1]), 16 },
{ PSF(delta_angles[2]), 16 },
{ PSF(groundEntityNum), 10 },
{ PSF(legsTimer), 16 },
{ PSF(torsoTimer), 16 },
{ PSF(legsAnim), 10 },
{ PSF(torsoAnim), 10 },
{ PSF(movementDir), 8 },
{ PSF(eFlags), 24 },
{ PSF(eventSequence), 8 },
{ PSF(events[0]), 8 },
{ PSF(events[1]), 8 },
{ PSF(events[2]), 8 },
{ PSF(events[3]), 8 },
{ PSF(eventParms[0]), 8 },
{ PSF(eventParms[1]), 8 },
{ PSF(eventParms[2]), 8 },
{ PSF(eventParms[3]), 8 },
{ PSF(clientNum), 8 },
{ PSF(weapons[0]), 32 },
{ PSF(weapons[1]), 32 },
{ PSF(weapon), 7 },
{ PSF(weaponstate), 4 },
{ PSF(weapAnim), 10 },
{ PSF(viewangles[0]), 0 },
{ PSF(viewangles[1]), 0 },
{ PSF(viewangles[2]), 0 },
{ PSF(viewheight), -8 },
{ PSF(damageEvent), 8 },
{ PSF(damageYaw), 8 },
{ PSF(damagePitch), 8 },
{ PSF(damageCount), 8 },
{ PSF(mins[0]), 0 },
{ PSF(mins[1]), 0 },
{ PSF(mins[2]), 0 },
{ PSF(maxs[0]), 0 },
{ PSF(maxs[1]), 0 },
{ PSF(maxs[2]), 0 },
{ PSF(crouchMaxZ), 0 },
{ PSF(crouchViewHeight), 0 },
{ PSF(standViewHeight), 0 },
{ PSF(deadViewHeight), 0 },
{ PSF(runSpeedScale), 0 },
{ PSF(sprintSpeedScale), 0 },
{ PSF(crouchSpeedScale), 0 },
{ PSF(friction), 0 },
{ PSF(viewlocked), 8 },
{ PSF(viewlocked_entNum), 16 },
{ PSF(nextWeapon), 8 },
{ PSF(teamNum), 8 },
{ PSF(onFireStart), 32 },
{ PSF(curWeapHeat), 8 },
{ PSF(aimSpreadScale), 8 },
{ PSF(serverCursorHint), 8 },
{ PSF(serverCursorHintVal), 8 },
{ PSF(classWeaponTime), 32 },
{ PSF(identifyClient), 8 },
{ PSF(identifyClientHealth), 8 },
{ PSF(aiState), 2 }
};

void MSG_ReadDeltaPlayerstate (msg_t *msg, playerState_t *from, playerState_t *to )
{
	int			i, lc;
	int			bits;
	netField_t	*field;
	int			numFields;
	int			startBit, endBit;
	int			print;
	int			*fromF, *toF;
	int			trunc;
	playerState_t dummy;

	if ( !from ) {
		from = &dummy;
		memset(&dummy, 0, sizeof(dummy));
	}

	if (to->commandTime & 4) {
		to->commandTime = from->commandTime;
		memcpy(to+4, from+4, sizeof(playerState_t)-4);
	} else
		memcpy(to, from, sizeof(playerState_t));

	if ( msg->bit == 0 )
		startBit = msg->readcount * 8 - GENTITYNUM_BITS;
	else
		startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;

	numFields = sizeof(playerStateFields) / sizeof(playerStateFields[0]);
	lc = MSG_ReadByte(msg);

	if (msg->readcount > msg->cursize)
		lc = -1;

	if (lc > 0) {
		for ( i = 0, field = playerStateFields ; i < lc ; i++, field++ ) {
			fromF = (int *)( (byte *)from + field->offset );
			toF = (int *)( (byte *)to + field->offset );

			if ( ! MSG_ReadBits( msg, 1 ) ) {
				*toF = *fromF;
			} else {
				if ( field->bits == 0 ) {
					if ( MSG_ReadBits( msg, 1 ) == 0 ) {
						trunc = MSG_ReadBits( msg, FLOAT_INT_BITS );
						trunc -= FLOAT_INT_BIAS;
						*(float *)toF = trunc;
					} else
						*toF = MSG_ReadBits( msg, 32 );
				} else
					*toF = MSG_ReadBits( msg, field->bits );
			}
		}
	}

	for ( i=lc,field = &playerStateFields[lc];i<numFields; i++, field++) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		*toF = *fromF;
	}

	if (MSG_ReadBits( msg, 1 ) ) {
		int idx;

		// parse stats
		if ( MSG_ReadBits( msg, 1 ) ) {
			bits = MSG_ReadBits (msg, MAX_STATS);
			if (msg->readcount > msg->cursize)
				bits = -1;

			for (i=0 ; i<MAX_STATS ; i++) {
				if (bits & (1<<i) ) {
					to->stats[i] = MSG_ReadShort(msg);
					if (msg->readcount > msg->cursize)
						to->stats[i] = -1;
				}
			}
		}

		// parse persistant stats
		if ( MSG_ReadBits( msg, 1 ) ) {
			bits = MSG_ReadBits (msg, MAX_PERSISTANT);
			if (msg->readcount > msg->cursize)
				bits = -1;

			for (i=0 ; i<MAX_PERSISTANT ; i++) {
				if (bits & (1<<i) ) {
					to->persistant[i] = MSG_ReadShort(msg);
					if (msg->readcount > msg->cursize)
						to->persistant[i] = -1;
				}
			}
		}

		// parse holdable
		if ( MSG_ReadBits( msg, 1 ) ) {
			bits = MSG_ReadBits (msg, MAX_HOLDABLE);
			if (msg->readcount > msg->cursize)
				bits = -1;
			for (i=0 ; i<MAX_HOLDABLE ; i++) {
				if (bits & (1<<i) ) {
					to->holdable[i] = MSG_ReadShort(msg);
					if (msg->readcount > msg->cursize)
						to->holdable[i] = -1;
				}
			}
		}

		// parse powerups
		if ( MSG_ReadBits( msg, 1 ) ) {
			bits = MSG_ReadBits (msg, MAX_POWERUPS);
			if (msg->readcount > msg->cursize)
				bits = -1;
			for (i=0 ; i<MAX_POWERUPS ; i++) {
				if (bits & (1<<i) ) {
					to->powerups[i] = MSG_ReadLong(msg);
					if (msg->readcount > msg->cursize)
						to->powerups[i] = -1;
				}
			}
		}

		// parse ammo
		if (MSG_ReadBits(msg, 1)) {
			int i, j, k;

			for (j=0; j<4; j++) {
				if (MSG_ReadBits(msg, 1)) {
					idx = MSG_ReadBits(msg, 16);

					if (msg->readcount > msg->cursize)
						idx = -1;

					for (i=0; i<16; i++) {
						if ( (idx >> i) & 1 ) {
							k = MSG_ReadBits(msg, 16);
							if (msg->readcount > msg->cursize)
								k = -1;
							to->ammo[i+16*j] = k;
						}
					}
				}
			}
		}

		// parse ammoclip
		{
			int idx2;
			int i, j, k;

			for (j=0; j<4; j++) {
				idx = MSG_ReadBits(msg, 1);
				if (idx) {
					idx2 = MSG_ReadBits(msg, 16);

					if (msg->readcount > msg->cursize)
						idx2 = -1;

					for (i=0; i<16; i++) {
						if ( (idx2 >> i) & 1 ) {
							k = MSG_ReadBits(msg, 16);
							if (msg->readcount > msg->cursize)
								k = -1;
							to->ammoclip[i+16*j] = k;
						}
					}
				}
			}
		}
	}
}

void MSG_WriteData( msg_t *buf, const void *data, int length )
{
	int i;
	for(i=0;i<length;i++) {
		MSG_WriteByte(buf, ((byte *)data)[i]);
	}
}

void MSG_WriteLong( msg_t *sb, int c )
{
	MSG_WriteBits( sb, c, 32 );
}

void MSG_WriteByte( msg_t *sb, int c )
{
	MSG_WriteBits( sb, c, 8 );
}

void MSG_WriteShort( msg_t *sb, int c )
{
	MSG_WriteBits( sb, c, 16 );
}

void MSG_WriteString( msg_t *sb, const char *s )
{
	if ( !s ) {
		MSG_WriteData (sb, "", 1);
	} else {
		int		l,i;
		char	string[MAX_STRING_CHARS];

		l = strlen( s );
		if ( l >= MAX_STRING_CHARS ) {
			LOG_WARN {emits("MSG_WriteString: MAX_STRING_CHARS");}
			MSG_WriteData (sb, "", 1);
			return;
		}
		Q_strncpyz( string, s, sizeof( string ) );

		for ( i = 0 ; i < l ; i++ ) {
			if ( ((byte *)string)[i] > 127 ) {
				string[i] = '.';
			}
		}

		MSG_WriteData (sb, string, l+1);
	}
}

void write_packet(void)
{
	msg_t		buf;
	byte		data[MAX_MSGLEN];
	int			i, j;
	usercmd_t	*cmd, *oldcmd;
	usercmd_t	nullcmd;
	int			packetNum;
	int			oldPacketNum;
	int			count, key;

	memset(&nullcmd, 0, sizeof(nullcmd));
	oldcmd = &nullcmd;

	MSG_Init(&buf, data, sizeof(data));

	MSG_Bitstream(&buf);
	// write the current serverId so the server
	// can tell if this is from the current gameState
	MSG_WriteLong(&buf, cl.serverId);

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	MSG_WriteLong(&buf, etdos.conn.serverMessageSequence);

	// write the last reliable message we received
	MSG_WriteLong(&buf, etdos.conn.serverCommandSequence);

	// write any unacknowledged clientCommands
	for ( i = etdos.conn.reliableAcknowledge + 1 ; i <= etdos.conn.reliableSequence ; i++ ) {
		MSG_WriteByte(&buf, clc_clientCommand);
		MSG_WriteLong(&buf, i);
		MSG_WriteString(&buf, etdos.conn.reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ]);
	}

	oldPacketNum = (etdos.conn.netchan.outgoingSequence - 2) & PACKET_MASK;
	count = cl.cmdNumber - cl.outPackets[ oldPacketNum ].p_cmdNumber;
	if (count > MAX_PACKET_USERCMDS)
		count = MAX_PACKET_USERCMDS;

	LOG_FLOW { emitv("%d 0x%x %i %i %i %i\n",
		cl.serverId, etdos.conn.checksumFeed, etdos.time,
  		etdos.conn.serverMessageSequence,
		etdos.conn.serverCommandSequence, count); }

	if ( count >= 1 ) {
		MSG_WriteByte (&buf, clc_moveNoDelta);

		// write the command count
		MSG_WriteByte(&buf, count);

		// use the checksum feed in the key
		key = etdos.conn.checksumFeed;
		// also use the message acknowledge
		key ^= etdos.conn.serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(etdos.conn.serverCommands[etdos.conn.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1)], 32);

		// write all the commands, including the predicted command
		for ( i = 0 ; i < count ; i++ ) {
			j = (cl.cmdNumber - count + i + 1) & CMD_MASK;
			cmd = &cl.cmds[j];
			MSG_WriteDeltaUsercmdKey(&buf, key, oldcmd, cmd);
			oldcmd = cmd;
		}
	}

	packetNum = etdos.conn.netchan.outgoingSequence & PACKET_MASK;
	cl.outPackets[packetNum].p_realtime = etdos.time;
	cl.outPackets[packetNum].p_serverTime = oldcmd->serverTime;
	cl.outPackets[packetNum].p_cmdNumber = cl.cmdNumber;
	etdos.conn.lastPacketSentTime = etdos.time;

	CL_Netchan_Transmit (&etdos.conn.netchan, &buf);
}

void Netchan_TransmitNextFragment( netchan_t *chan )
{
	msg_t		send;
	byte		send_buf[MAX_PACKETLEN];
	int			fragmentLength;

	// write the packet header
	MSG_InitOOB (&send, send_buf, sizeof(send_buf));

	MSG_WriteLong( &send, chan->outgoingSequence | FRAGMENT_BIT );

	MSG_WriteShort( &send, htons(etdos.net_port));

	// copy the reliable message to the packet first
	fragmentLength = FRAGMENT_SIZE;
	if ( chan->unsentFragmentStart  + fragmentLength > chan->unsentLength )
		fragmentLength = chan->unsentLength - chan->unsentFragmentStart;

	MSG_WriteShort( &send, chan->unsentFragmentStart );
	MSG_WriteShort( &send, fragmentLength );
	MSG_WriteData( &send, chan->unsentBuffer + chan->unsentFragmentStart, fragmentLength );

	// send the datagram
	NET_SendPacket(chan->remoteAddress, send.cursize, send.data);

	chan->unsentFragmentStart += fragmentLength;

	if ( chan->unsentFragmentStart == chan->unsentLength && fragmentLength != FRAGMENT_SIZE ) {
		chan->outgoingSequence++;
		chan->unsentFragments = qfalse;
	}
}


void Netchan_Transmit( netchan_t *chan, int length, const byte *data )
{
	msg_t		send;
	byte		send_buf[MAX_PACKETLEN];

	if ( length > MAX_MSGLEN ) {
		LOG_WARN {emitv("Netchan_Transmit: length = %i", length);}
		return;
	}
	chan->unsentFragmentStart = 0;

	// fragment large reliable messages
	if ( length >= FRAGMENT_SIZE ) {
		chan->unsentFragments = qtrue;
		chan->unsentLength = length;
		memcpy(chan->unsentBuffer, data, length);

		Netchan_TransmitNextFragment(chan);
		return;
	}

	// write the packet header
	MSG_InitOOB (&send, send_buf, sizeof(send_buf));
	MSG_WriteLong( &send, chan->outgoingSequence );

	chan->outgoingSequence++;

	MSG_WriteShort(&send, htons(etdos.net_port));

	MSG_WriteData(&send, data, length );

	// send the datagram
	NET_SendPacket(chan->remoteAddress, send.cursize, send.data);
}

void CL_Netchan_Encode( msg_t *msg )
{
	int serverId, messageAcknowledge, reliableAcknowledge;
	int i, index, srdc, sbit, soob;
	byte key, *string;

	if ( msg->cursize <= CL_ENCODE_START )
		return;

	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->bit = 0;
	msg->readcount = 0;
	msg->oob = 0;

	serverId = MSG_ReadLong(msg);
	messageAcknowledge = MSG_ReadLong(msg);
	reliableAcknowledge = MSG_ReadLong(msg);

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	string = (byte *)etdos.conn.serverCommands[reliableAcknowledge & (MAX_RELIABLE_COMMANDS-1)];
	index = 0;

	key = etdos.conn.challenge ^ serverId ^ messageAcknowledge;
	for (i = CL_ENCODE_START; i < msg->cursize; i++) {
		if (!string[index])
			index = 0;
		if (string[index] > 127 || string[index] == '%')
			key ^= '.' << (i & 1);
		else
			key ^= string[index] << (i & 1);

		index++;
		*(msg->data + i) = (*(msg->data + i)) ^ key;
	}
}

void CL_Netchan_Transmit( netchan_t *chan, msg_t* msg )
{
	MSG_WriteByte( msg, clc_EOF );

	CL_Netchan_Encode( msg );
	Netchan_Transmit( chan, msg->cursize, msg->data );
}

void sv_reliable_cmd( const char *cmd )
{
	int		index;

	LOG_FLOW { emitv("%s\n", cmd); }

	if ( etdos.conn.reliableSequence - etdos.conn.reliableAcknowledge > MAX_RELIABLE_COMMANDS ) {
		LOG_WARN { emits("overflow\n"); }
		return;
	}

	etdos.conn.reliableSequence++;
	index = etdos.conn.reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( etdos.conn.reliableCommands[index], cmd, sizeof(etdos.conn.reliableCommands[index]));
}

