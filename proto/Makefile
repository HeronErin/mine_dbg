# proto/Makefile

# Compiler and flags
CC = gcc
CFLAGS = -g -I.

# Source files and target
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = libproto.a

# Rules
all: $(TARGET)

$(TARGET): $(OBJ)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
