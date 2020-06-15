#include <stdio.h>

int main(int argc, char **argv)
{
	char line1[65355];
	char line2[65355];

	if (argc < 3) {
		printf("./prog <base> <extras>\n");
		return 1;
	}

	FILE *base = fopen(argv[1], "r");
	if (!base)
		return 1;
	FILE *extra = fopen(argv[2], "r");
	if (!extra)
		return 1;

	int fnd = 0;
	while (fgets(line1, sizeof(line1), extra)) {
		fnd = 0;
		fseek(base, 0, SEEK_SET);
		while(fgets(line2, sizeof(line2), base)) {
			char *q = strchr(line2, ' ')+1;
			if (strstr(line1, q)) {
				fnd = 1;
				break;
			}
		}

		if (!fnd)
			printf("%s", line1);
	}
	return 0;
}
