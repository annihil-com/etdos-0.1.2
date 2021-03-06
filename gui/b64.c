/*
	base64 encode
	
	KoPyRight kobject_
	stripped down & modified version of base64 encoding
	original by Bob Trower 08/04/01
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void encodeblock( unsigned char in[3], unsigned char out[4], int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
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
