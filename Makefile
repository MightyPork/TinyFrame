build: tf.bin

run: tf.bin
	./tf.bin

debug: tf.bin
	gdb -q -ex run ./tf.bin

tf.bin: test.c TinyFrame.c TinyFrame.h
	gcc -Os --std=gnu89 -Wall -Wno-main -Wno-unused -Wextra test.c TinyFrame.c -I. -o tf.bin
