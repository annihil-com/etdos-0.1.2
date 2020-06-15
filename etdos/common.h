#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include "protocol.h"
#include "shared.h"

extern char __buf[4096];
extern clctrl_t ctrl;
extern FILE* dbg_fp;

extern int verbosity;

#define emits(f)		{ fprintf(dbg_fp, "%s:%i %s(): %s", __FILE__, __LINE__, __PRETTY_FUNCTION__, f); fflush(dbg_fp); }
#define emitv(f, ...) 	{ sprintf(__buf, "%s:%i %s(): %s", __FILE__, __LINE__, __PRETTY_FUNCTION__, f); \
						fprintf (dbg_fp, __buf, __VA_ARGS__); fflush(dbg_fp); }

// verbosity log levels
#define LOG_WARN		if (verbosity >= 1)
#define LOG_FLOW		if (verbosity >= 2)
#define LOG_DATA		if (verbosity >= 3)
#define	LOG_LOWLEVEL	if (verbosity >= 4)

/*
	NOTE: msglen is increased to 32768 in ET - very important !!
*/
#define	MAX_MSGLEN 32768
#define	MAX_INFO_STRING 1024
#define	MAX_INFO_KEY 1024
#define	MAX_INFO_VALUE 1024
#define	BIG_INFO_STRING 8192
#define	BIG_INFO_KEY 8192
#define	BIG_INFO_VALUE 8192

#define LittleShort
#define LittleLong
#define LittleFloat
#define BigShort(x) ShortSwap(x)
#define BigLong(x) LongSwap(x)
#define BigFloat(x) FloatSwap(&x)

char *Info_ValueForKey( const char *s, const char *key );
void Info_RemoveKey( char *s, const char *key );
void Info_SetValueForKey( char *s, const char *key, const char *value );
int Sys_Milliseconds (void);
char *va( char *format, ... );

int Q_strncmp (const char *s1, const char *s2, int n);
int Q_stricmp (const char *s1, const char *s2);
int Q_stricmpn (const char *s1, const char *s2, int n);
void Q_strncpyz( char *dest, const char *src, int destsize );

void Cmd_TokenizeString( const char *text_in );
char *Cmd_Args( void );
void Cmd_ArgvBuffer( int arg, char *buffer, int bufferLength );
char *Cmd_Argv( int arg );
int Cmd_Argc( void );
char *Cmd_ArgsFrom(int arg);
int Com_HashKey(char *string, int maxlen);

void emit_data(void *buf, int len);
void real_seed();
int randomi(int min, int max);

// base 64
char *encode_mem(void *inb, int mlen);
void *decode_mem(char *bin, int *olen);

#endif
