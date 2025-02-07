# proto/Makefile

# Compiler and flags
CC = ccache gcc
CFLAGS = -g -I. -lz -I../libs -Iconstants -march=native

# Source files and target
SRC = $(wildcard *.c) constants/constants.c
OBJ = $(SRC:.c=.o)
TARGET = libproto.a

# Default target
all: constants/constants.bin $(TARGET)

$(TARGET): $(OBJ)
	ar rcs $@ $^

%.o: %.c FORCE
	$(CC) $(CFLAGS) -c $< -o $@

constants/constants.bin : constants/constants.h
	$(MAKE) -C constants

FORCE:

clean:
	rm -f $(OBJ) $(TARGET)
	$(MAKE) -C constants clean


