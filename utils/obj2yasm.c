/*
	convert objdump asm output to quasi-usefull yasm input

	usefull to shamelessly rip code out of a binary...

	(call addresses still need manual care)
	Copyright kobject_
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	char line[128];
	int jmptarg;
	int adr;
	int jmp;
	int jmplbl;
	int i_jmptrg;
} ins_t;

void replace(char *in, const char *pat, const char *to)
{
	char *st = strstr(in, pat);
	if (!st)
		return;

	int l = strlen(pat);
	int m = strlen(to);
	int k = strlen(in);

	memmove(st+m, st+l, k-l);
	memcpy(st, to, m);
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return 1;

	FILE *fp = fopen(argv[1], "r");
	if (!fp) {
		printf("bad file\n");
		return 1;
	}

	char buf[1024];
	char s[1024];
	char *p;
	int i,j;
	int nins = 0;
	ins_t *ins = NULL;
	ins_t *ti;


	while (fgets(buf, 1024, fp)) {
		strcpy(s, buf+32);

		nins++;
		ins = realloc(ins, nins*sizeof(ins_t));
		ti = &ins[nins-1];
		memset(ti, 0, sizeof(ins_t));

		sscanf(buf, "%x", &ti->adr);

		// check for jmps, ex:
		//jne    8192156 <_Unwind_Find_FDE+0x2c6>
		if (s[0] == 'j') {
			sscanf(s+7, "%x", &ti->jmp);
			*strchr(s, '<') = '\0';
		}

		replace(s, "DWORD PTR ", "");
		replace(s, "BYTE PTR", "byte");
		replace(s, "WORD PTR", "word");

		strcpy(ti->line, s);
	}

	// first pass - resolve jmp targets
	for(i=0; i<nins; i++) {
		ti = &ins[i];
		if (ti->jmp) {
			ti->i_jmptrg = -1;
			for(j=0; j<nins; j++) {
				if (ins[j].adr == ti->jmp){
					ins[j].jmptarg = 1;
					ti->i_jmptrg = j;
					break;
				}
			}
		}
	}

	// second pass - label jump targets
	int njmps = 0;
	for(i=0; i<nins; i++) {
		ti = &ins[i];
		if (ti->jmptarg)
			ti->jmplbl = ++njmps;
	}

	// print results
	for(i=0; i<nins; i++) {
		ti = &ins[i];
		if (ti->jmplbl)
			printf("\nL%i\n", ti->jmplbl);

		if (ti->jmp) {
			if (ti->i_jmptrg >= 0) {
				char jt[10];
				ti->line[7] = 0;
				sprintf(jt, "L%i\n", ins[ti->i_jmptrg].jmplbl);
				strcat(ti->line, jt);
			} else
				strcat(ti->line, "\t\t;unresolved");
		}
		printf("\t%s", ti->line);
	}

	return 0;
}
