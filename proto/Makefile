# proto/Makefile

# Compiler and flags
CC = ccache gcc
CFLAGS = -g -I. -lz -I../libs -march=native

# Source files and target
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = libproto.a

# Default target
all: $(TARGET)

$(TARGET): $(OBJ)
	ar rcs $@ $^

%.o: %.c FORCE
	$(CC) $(CFLAGS) -c $< -o $@


FORCE:

clean:
	rm -f $(OBJ) $(TARGET)


