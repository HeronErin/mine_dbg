
CC = gcc
CFLAGS = -g -I. -lz -I.. -march=native

all: generated_constants.h

gen.a : constant_gen.c constants.h
	$(CC) -c ../xxhash.c -o xxhash.o
	$(CC) constant_gen.c xxhash.o -o gen.a

generated_constants.h : gen.a
	./gen.a

clean:
	rm -f *.o *.a *.bin