all:
	yasm -f elf32 pure_chksum.asm -o pure.o
	gcc -g -ggdb -c b64.c -o b64.o
	gcc -g -ggdb -c pk3cksgrabber.c -o pk3.o
	gcc pure.o b64.o pk3.o -o pk3grab -lzzip

clean:
	rm pure.o b64.o pk3.o pk3grab
