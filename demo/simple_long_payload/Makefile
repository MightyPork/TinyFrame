CFILES=../utils.c ../../TinyFrame.c
INCLDIRS=-I. -I.. -I../..
CFLAGS=-O0 -ggdb --std=gnu99 -Wno-main -Wall -Wextra $(CFILES) $(INCLDIRS)


build: test.bin

run: test.bin
	./test.bin

test.bin: test.c $(CFILES)
	gcc test.c $(CFLAGS) -o test.bin
