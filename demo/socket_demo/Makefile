CFILES=../demo.c ../utils.c ../../TinyFrame.c
INCLDIRS=-I. -I.. -I../..
CFLAGS=-O0 -ggdb --std=gnu99 -Wno-main -Wall -Wno-unused -Wextra $(CFILES) $(INCLDIRS)

build: master.bin slave.bin

master: master.bin
	./master.bin

slave: slave.bin
	./slave.bin

master.bin: master.c $(CFILES)
	gcc master.c $(CFLAGS) -o master.bin

slave.bin: slave.c $(CFILES)
	gcc slave.c $(CFLAGS) -o slave.bin
