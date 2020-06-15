/*
	base64 memory [d]e[n]coder
	
	KoPyRight kobject_
	stripped down & modified version of base64 encoding
	original by Bob Trower 08/04/01
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

void encodeblock( unsigned char in[3], unsigned char out[4], int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

void decodeblock( unsigned char in[4], unsigned char out[3] )
{
    out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
    out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
    out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}


char *encode_mem(void *inb, int mlen)
{
	unsigned char in[3], out[4];
	int j, i, len;
	char *of = NULL;
	int ofz = 0;

	j = 0;
	while(j < mlen) {
		len = 0;
		for( i = 0; i < 3; i++ ) {
			if( j < mlen )
				len++;
			if (j < mlen)
				in[i] = *(unsigned char*)(inb+j);
			else
				in[i] = 0;
			j++;
		}
		if( len ) {
			encodeblock( in, out, len );

			ofz++;
			of = realloc(of, ofz*4);
			memset(of+(ofz-1)*4, 0, 4);

			for( i = 0; i < 4; i++ )
				of[4*(ofz-1)+i] = out[i];
		}
	}

	of = realloc(of, (ofz+1)*4+1);
	of[ofz*4]=0;

	return of;
}

void *decode_mem(char *bin, int *olen)
{
    unsigned char in[4], out[3], v;
    int i,j, len;

	int ilen = strlen(bin);
	unsigned char *obuf = NULL;
	int nobuf = 0;
	
	j = 0;
    while( j<ilen )
	{
        for( len = 0, i = 0; i < 4 && (j<ilen); i++ ) {
            v = 0;
            while( j<ilen && v == 0 ) {
                v = (unsigned char)bin[j];
                v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
                if( v )
                    v = (unsigned char) ((v == '$') ? 0 : v - 61);
				j++;
            }
            if( j < ilen ) {
                len++;
                if( v )
                    in[i] = (unsigned char) (v - 1);
            } else
                in[i] = 0;
        }
        if (len) {
            decodeblock(in, out);

			int mobuf = nobuf;
			nobuf += len-1;
			
			obuf = realloc(obuf, nobuf);
			
            for( i = 0; i < len - 1; i++ )
				obuf[mobuf+i] = out[i];
        }
    }

	*olen = nobuf;
	return (void*)obuf;
}
