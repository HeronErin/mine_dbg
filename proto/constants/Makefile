
CC = gcc
CFLAGS = -g -I. -lz -I.. -march=native

all: constants.bin

gen.a : constant_gen.c constants.h
	$(CC) -c ../xxhash.c -o xxhash.o
	$(CC) constant_gen.c xxhash.o -o gen.a

constants.bin : gen.a
	./gen.a

clean:
	rm -f *.o *.a *.bin