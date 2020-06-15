/*
	pk3 raw checksum grabber :)

	CoPyRiGhT kobject_
*/

#include <stdio.h>
#include<libgen.h>
#include <zzip/lib.h>

extern char *encode_mem(void *inb, int mlen);
extern unsigned int purechecksum(void *msg, int len, unsigned int chk);

#define ISOPT(x) (argv[c][0] == '-')
#define GETOPT(x) (ISOPT(x) ? argv[c][1] : 0)

unsigned int chksum = 0x12345678;

void get_checksum(char *file)
{
	ZZIP_DIR* dir = zzip_dir_open(file, 0);
	if (dir) {
		int ncrc = 0;
		unsigned int *crcs = NULL;

		ZZIP_DIRENT dirent;
		struct zzip_dir_hdr * hdr = dir->hdr0;
		while (zzip_dir_read(dir,&dirent)) {
			if (hdr->d_crc32) {
				ncrc++;
				crcs = (unsigned int*)realloc(crcs, 4*ncrc);
				crcs[ncrc-1] = hdr->d_crc32;
			}
			hdr = (struct zzip_dir_hdr *) ((char *)hdr + hdr->d_reclen)	;
		}
		zzip_dir_close(dir);

		if (ncrc) {
			char *b = encode_mem(crcs, ncrc*4);
			printf("%s %s\n", file, b);
			free(b);
		}
	}
}

void get_checksum_raw(char *file)
{
	ZZIP_DIR* dir = zzip_dir_open(file, 0);
	if (dir) {
		int ncrc = 0;
		unsigned int *crcs = NULL;

		ZZIP_DIRENT dirent;
		struct zzip_dir_hdr * hdr = dir->hdr0;
		while (zzip_dir_read(dir,&dirent)) {
			if (hdr->d_crc32) {
				ncrc++;
				crcs = (unsigned int*)realloc(crcs, 4*ncrc);
				crcs[ncrc-1] = hdr->d_crc32;
			}
			hdr = (struct zzip_dir_hdr *) ((char *)hdr + hdr->d_reclen)	;
		}
		zzip_dir_close(dir);

		fwrite(crcs, 4, ncrc, stdout);
	}
}

void print_pk3(char *file)
{
	ZZIP_DIR* dir = zzip_dir_open(file, 0);
	if (dir) {
		ZZIP_DIRENT dirent;
		struct zzip_dir_hdr * hdr = dir->hdr0;
		while (zzip_dir_read(dir,&dirent)) {
			if (hdr->d_crc32)
				printf("%s\t\t0x%x\n", dirent.d_name, hdr->d_crc32);
			hdr = (struct zzip_dir_hdr *) ((char *)hdr + hdr->d_reclen)	;
		}
		zzip_dir_close(dir);
	}
}

void print_pure_checksum(char *file)
{
	ZZIP_DIR* dir = zzip_dir_open(file, 0);
	if (dir) {
		int ncrc = 0;
		unsigned int *crcs = NULL;

		ZZIP_DIRENT dirent;
		struct zzip_dir_hdr * hdr = dir->hdr0;
		while (zzip_dir_read(dir,&dirent)) {
			if (hdr->d_crc32) {
				ncrc++;
				crcs = (unsigned int*)realloc(crcs, 4*ncrc);
				crcs[ncrc-1] = hdr->d_crc32;
			}
			hdr = (struct zzip_dir_hdr *) ((char *)hdr + hdr->d_reclen)	;
		}
		zzip_dir_close(dir);

		if (ncrc) {
			unsigned int pure = purechecksum(crcs, 4*ncrc, chksum);
			printf("checksum feed: 0x%x, pure checksum: 0x%x %i\n", chksum, pure, (int)pure);
		}
	}
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		printf("usage: pk3 -c/-v/-t <file>\n");
		return 1;
	}

	int c;
	for (c=1; c<argc; c++) {
		switch(GETOPT(c)) {
			case 'c':
				get_checksum(argv[2]);
				break;
			case 'r':
				get_checksum_raw(argv[2]);
				break;
			case 'v':
				print_pk3(argv[2]);
				break;
			case 't':
				sscanf(argv[2], "0x%x", &chksum);
				print_pure_checksum(argv[3]);
				break;
		}
	}

	return 0;
}
