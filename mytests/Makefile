
CC=powerpc-apple-macos-gcc
#CC=powerpc-apple-macos-gcc
#CC=m68k-apple-macos-gcc
#CC=m68k-amigaos-gcc -mcrt=nix20

CFLAGS=-I../SDL-main/include
#CFLAGS=`sdl2-config --cflags`

LD=powerpc-apple-macos-g++
#LD=powerpc-apple-macos-g++
#LD=m68k-apple-macos-g++
#LD=m68k-amigaos-g++ -mcrt=nix20

#LDFLAGS=-L.. -lSDL2main -lSDL2
LDFLAGS=-L../SDL-main -lSDL3 -lThreadsLib
#LDFLAGS=-L../SDL-main -lSDL2 -lSDL2main

CONV=$(HOME)/sdl3macos9/xcoff2carb.sh
#CONV=$(HOME)/sdl3macos9/xcoff2app.sh
#CONV=$(HOME)/sdl3macos9/bin2app.sh
#CONV=echo

#EXT=.code.bin

.c.o:
	$(CC) $(CFLAGS) -c -o$*.o $*.c

.cpp.o:
	$(CC) $(CFLAGS) -c -o$*.o $*.cpp


all:

clean:
	rm -f *.o *.pef *.dsk *.APPL *.bin *.gdb
	rm -f sdl3test


sdl3test: sdl3test.o
	$(LD) -o sdl3test$(EXT) sdl3test.o $(LDFLAGS)
	$(CONV) sdl3test

