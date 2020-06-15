CF=-Os
BUILDDIR=../build
#STR = strip -s -x etdos

all:
	yasm -f elf32 pure_chksum.asm -o $(BUILDDIR)/pure.o
	gcc $(CF) -c common.c -o $(BUILDDIR)/common.o
	gcc $(CF) -c huffman.c -o $(BUILDDIR)/huffman.o
	gcc $(CF) -c msg.c -o $(BUILDDIR)/msg.o
	gcc $(CF) -c net.c -o $(BUILDDIR)/net.o
	gcc $(CF) -c cvar.c -o $(BUILDDIR)/cvar.o
	gcc $(CF) -c b64.c -o $(BUILDDIR)/b64.o
	ar r etdos.a \
		$(BUILDDIR)/b64.o \
		$(BUILDDIR)/common.o \
		$(BUILDDIR)/huffman.o \
		$(BUILDDIR)/msg.o \
		$(BUILDDIR)/net.o \
		$(BUILDDIR)/cvar.o \
		$(BUILDDIR)/pure.o \
	$(STR)

clean:
	rm -rf etdos.a $(BUILDDIR)/*
