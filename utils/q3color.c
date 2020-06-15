/* convert q_math.c color codes to html */

#include <stdio.h>


typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

vec4_t	g_color_table[32] =
	{
		{ 0.0,	0.0,	0.0,	1.0 },	// 0 - black		0
		{ 1.0,	0.0,	0.0,	1.0 },	// 1 - red			1
		{ 0.0,	1.0,	0.0,	1.0 },	// 2 - green		2
		{ 1.0,	1.0,	0.0,	1.0 },	// 3 - yellow		3
		{ 0.0,	0.0,	1.0,	1.0 },	// 4 - blue			4
		{ 0.0,	1.0,	1.0,	1.0 },	// 5 - cyan			5
		{ 1.0,	0.0,	1.0,	1.0 },	// 6 - purple		6
		{ 1.0,	1.0,	1.0,	1.0 },	// 7 - white		7
		{ 1.0,	0.5,	0.0,	1.0 },	// 8 - orange		8
		{ 0.5,	0.5,	0.5,	1.0 },	// 9 - md.grey		9
		{ 0.75,	0.75,	0.75,	1.0 },	// : - lt.grey		10		// lt grey for names
		{ 0.75, 0.75,	0.75,	1.0 },	// ; - lt.grey		11
		{ 0.0,	0.5,	0.0,	1.0 },	// < - md.green		12
		{ 0.5,	0.5,	0.0,	1.0 },	// = - md.yellow	13
		{ 0.0,	0.0,	0.5,	1.0 },	// > - md.blue		14
		{ 0.5,	0.0,	0.0,	1.0 },	// ? - md.red		15
		{ 0.5,	0.25,	0.0,	1.0 },	// @ - md.orange	16
		{ 1.0,	0.6f,	0.1f,	1.0 },	// A - lt.orange	17
		{ 0.0,	0.5,	0.5,	1.0 },	// B - md.cyan		18
		{ 0.5,	0.0,	0.5,	1.0 },	// C - md.purple	19
		{ 0.0,	0.5,	1.0,	1.0 },	// D				20
		{ 0.5,	0.0,	1.0,	1.0 },	// E				21
		{ 0.2f,	0.6f,	0.8f,	1.0 },	// F				22
		{ 0.8f,	1.0,	0.8f,	1.0 },	// G				23
		{ 0.0,	0.4,	0.2f,	1.0 },	// H				24
		{ 1.0,	0.0,	0.2f,	1.0 },	// I				25
		{ 0.7f,	0.1f,	0.1f,	1.0 },	// J				26
		{ 0.6f,	0.2f,	0.0,	1.0 },	// K				27
		{ 0.8f,	0.6f,	0.2f,	1.0 },	// L				28
		{ 0.6f,	0.6f,	0.2f,	1.0 },	// M				29
		{ 1.0,	1.0,	0.75,	1.0 },	// N				30
		{ 1.0,	1.0,	0.5,	1.0 },	// O				31
	};

int main()
{
	int i = 0;
	char hex[64];
	printf("static const char *q3colors[] = {\n");
	for(i=0; i<32; i++) {
		sprintf(hex, "\"#%02x%02x%02x\",",
			(int)(g_color_table[i][0]*255.) & 0xff,
			(int)(g_color_table[i][1]*255.) & 0xff,
			(int)(g_color_table[i][2]*255.) & 0xff);
		printf("%s\n", hex);
	}
	printf("};\n");
}
