/*
	Common Q3 stuff included for convience
	Copyright ID software I guess...
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

char __buf[4096];
int verbosity = 0;
FILE* dbg_fp;

char *Info_ValueForKey( const char *s, const char *key )
{
	char	pkey[BIG_INFO_KEY];
	static	char value[2][BIG_INFO_VALUE];
	static	int	valueindex = 0;
	char	*o;

	if ( !s || !key )
		return "";

	if ( strlen( s ) >= BIG_INFO_STRING )
		LOG_WARN { emits("oversize infostring") }

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp (key, pkey) )
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}

void Info_RemoveKey( char *s, const char *key ) {
	char	*start;
	char	pkey[MAX_INFO_KEY];
	char	value[MAX_INFO_VALUE];
	char	*o;

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		LOG_WARN {emits("Info_RemoveKey: oversize infostring" );}
		return;
	}

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			strcpy (start, s);
			return;
		}

		if (!*s)
			return;
	}

}

void Info_SetValueForKey( char *s, const char *key, const char *value )
{
	char	newi[MAX_INFO_STRING];
	const char* blacklist = "\\;\"";

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		LOG_WARN {emits("Info_SetValueForKey: oversize infostring" );}
		return;
	}

	for(; *blacklist; ++blacklist)
	{
		if (strchr (key, *blacklist) || strchr (value, *blacklist))
		{
			LOG_WARN {emitv("Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value);}
			return;
		}
	}

	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	snprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) >= MAX_INFO_STRING)
	{
		LOG_WARN {emits("Info string length exceeded\n");}
		return;
	}

	strcat (newi, s);
	strcpy (s, newi);
}

// milliseconds since start of main loop
int Sys_Milliseconds (void)
{
	static unsigned int sys_timeBase = 0;

	struct timeval tp;

	gettimeofday(&tp, NULL);

	if (!sys_timeBase)
	{
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	int curtime = (tp.tv_sec - sys_timeBase)*1000 + tp.tv_usec/1000;

	return curtime;
}

char *va( char *format, ... )
{
	va_list		argptr;
	static char		string[2][32000];
	static int		index = 0;
	char	*buf;

	buf = string[index & 1];
	index++;

	va_start (argptr, format);
	vsprintf (buf, format,argptr);
	va_end (argptr);

	return buf;
}

int Q_strncmp (const char *s1, const char *s2, int n)
{
	int		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;

		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
	} while (c1);

	return 0;
}

int Q_stricmp (const char *s1, const char *s2)
{
	return (s1 && s2) ? Q_stricmpn (s1, s2, 99999) : -1;
}

int Q_stricmpn (const char *s1, const char *s2, int n)
{
	int		c1, c2;

	if ( s1 == NULL ) {
		if ( s2 == NULL )
			return 0;
		else
			return -1;
	}
	else if ( s2==NULL )
		return 1;


	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;
		}

		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);

	return 0;
}

void Q_strncpyz( char *dest, const char *src, int destsize )
{
	if ( !dest )
		LOG_WARN {emits("Q_strncpyz: NULL dest" );}

	if ( !src )
		LOG_WARN {emits("Q_strncpyz: NULL src" );}

	if ( destsize < 1 )
		LOG_WARN {emits("Q_strncpyz: destsize < 1");}

	strncpy( dest, src, destsize-1 );
	dest[destsize-1] = 0;
}

static	int cmd_argc;
static	char *cmd_argv[MAX_STRING_TOKENS];
static	char cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];
static	char cmd_cmd[BIG_INFO_STRING];

int Cmd_Argc( void )
{
	return cmd_argc;
}

char *Cmd_Argv( int arg )
{
	if ( (unsigned)arg >= cmd_argc ) {
		return "";
	}
	return cmd_argv[arg];
}

void Cmd_ArgvBuffer( int arg, char *buffer, int bufferLength )
{
	Q_strncpyz( buffer, Cmd_Argv( arg ), bufferLength );
}

char *Cmd_Args( void )
{
	static	char cmd_args[MAX_STRING_CHARS];
	int		i;

	cmd_args[0] = 0;
	for ( i = 1 ; i < cmd_argc ; i++ ) {
		strcat( cmd_args, cmd_argv[i] );
		if ( i != cmd_argc-1 ) {
			strcat( cmd_args, " " );
		}
	}

	return cmd_args;
}

void Cmd_TokenizeString2( const char *text_in, qboolean ignoreQuotes )
{
	const char	*text;
	char	*textOut;

	cmd_argc = 0;

	if ( !text_in ) {
		return;
	}

	Q_strncpyz( cmd_cmd, text_in, sizeof(cmd_cmd) );

	text = text_in;
	textOut = cmd_tokenized;

	while ( 1 ) {
		if ( cmd_argc == MAX_STRING_TOKENS ) {
			return;
		}

		while ( 1 ) {
			while ( *text && *text <= ' ' ) {
				text++;
			}
			if ( !*text ) {
				return;
			}

			if ( text[0] == '/' && text[1] == '/' ) {
				return;
			}

			if ( text[0] == '/' && text[1] =='*' ) {
				while ( *text && ( text[0] != '*' || text[1] != '/' ) ) {
					text++;
				}
				if ( !*text ) {
					return;
				}
				text += 2;
			} else {
				break;
			}
		}

		if ( !ignoreQuotes && *text == '"' ) {
			cmd_argv[cmd_argc] = textOut;
			cmd_argc++;
			text++;
			while ( *text && *text != '"' ) {
				*textOut++ = *text++;
			}
			*textOut++ = 0;
			if ( !*text ) {
				return;
			}
			text++;
			continue;
		}

		cmd_argv[cmd_argc] = textOut;
		cmd_argc++;

		while ( *text > ' ' ) {
			if ( !ignoreQuotes && text[0] == '"' ) {
				break;
			}

			if ( text[0] == '/' && text[1] == '/' ) {
				break;
			}

			if ( text[0] == '/' && text[1] =='*' ) {
				break;
			}

			*textOut++ = *text++;
		}

		*textOut++ = 0;

		if ( !*text ) {
			return;
		}
	}

}

void Cmd_TokenizeString( const char *text_in )
{
	Cmd_TokenizeString2( text_in, qfalse );
}

void emit_data(void *buf, int len)
{
#define stride 16
	int i = 0;
	int j = 0;

	fprintf(dbg_fp, "raw datagram, size %i (0x%x) bytes\n", len, len);
	while (i < len)
	{
		for (j=0; j<stride; j++) {
			fprintf(dbg_fp, "%02x ", *(byte*)(buf+j+i));
			if (j+i == len) {
				fprintf(dbg_fp, "\n");
				return;
			}
		}
		i+=16;
		fprintf(dbg_fp, "\n");
	}
}

char *Cmd_ArgsFrom(int arg)
{
	static	char cmd_args[BIG_INFO_STRING];
	int i;

	cmd_args[0] = 0;
	if (arg < 0)
		arg = 0;
	for ( i = arg ; i < cmd_argc ; i++ ) {
		strcat( cmd_args, cmd_argv[i] );
		if ( i != cmd_argc-1 ) {
			strcat( cmd_args, " " );
		}
	}

	return cmd_args;
}

int Com_HashKey(char *string, int maxlen)
{
	int register hash, i;

	hash = 0;
	for (i = 0; i < maxlen && string[i] != '\0'; i++) {
		hash += string[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return hash;
}

void real_seed()
{
	unsigned long long result;
	asm volatile ("rdtsc" : "=A" (result) );

	unsigned int res32 = result & 0xfffffff;
	srand(result);
}

int randomi(int min, int max)
{
	int j = min + (int) ((float)(max-min) * (rand() / (RAND_MAX + 1.0)));
	return j;
}
