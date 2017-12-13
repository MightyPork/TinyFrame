CFILES=../utils.c ../../TinyFrame.c
INCLDIRS=-I. -I.. -I../..
CFLAGS=-O0 -ggdb --std=gnu99 -Wno-main -Wno-unused -Wall -Wextra $(CFILES) $(INCLDIRS)

run: test.bin
	./test.bin

build: test.bin

test.bin: test.c $(CFILES)
	gcc test.c $(CFLAGS) -o test.bin
