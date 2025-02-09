# proto/Makefile

# Compiler and flags
CC = ccache gcc
CFLAGS = -g -I. -lz -I../libs -Iconstants -march=native

# Source files and target
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = libproto.a

# Default target
all: constants/generated_constants.h $(TARGET)

$(TARGET): $(OBJ)
	ar rcs $@ $^

%.o: %.c FORCE
	$(CC) $(CFLAGS) -c $< -o $@

constants/generated_constants.h : constants/constants.h
	$(MAKE) -C constants

FORCE:

clean:
	rm -f $(OBJ) $(TARGET)
	$(MAKE) -C constants clean


