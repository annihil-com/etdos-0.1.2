CF=-I.. -Os -D_USE_MMAP -fomit-frame-pointer -Wall -Wstrict-prototypes

all:
	gcc $(CF) -c -o zip.o zip.c
	gcc $(CF) -c -o write.o write.c
	gcc $(CF) -c -o stat.o stat.c
	gcc $(CF) -c -o memdisk.o memdisk.c
	gcc $(CF) -c -o info.o info.c
	gcc $(CF) -c -o fseeko.o fseeko.c
	gcc $(CF) -c -o plugin.o plugin.c
	gcc $(CF) -c -o file.o file.c
	gcc $(CF) -c -o fetch.o fetch.c
	gcc $(CF) -c -o err.o err.c
	gcc $(CF) -c -o dir.o dir.c
	ar r zip.a \
		zip.o \
		write.o \
		stat.o \
		memdisk.o \
		info.o \
		fseeko.o \
		file.o \
		fetch.o \
		plugin.o \
		err.o \
		dir.o


clean:
	rm -rf *.o zip.a
