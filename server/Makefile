# server/Makefile

# Compiler and flags
CC = gcc
CFLAGS = -g -I../proto

# Source files and target
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = server

# Proto library
PROTO_DIR = ../proto
PROTO_LIB = $(PROTO_DIR)/libproto.a

# Rules
all: $(TARGET)

$(TARGET): $(OBJ) $(PROTO_LIB)
	$(CC) $(CFLAGS) -o $@ $^ -L$(PROTO_DIR) -lproto

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(PROTO_LIB):
	$(MAKE) -C $(PROTO_DIR)

clean:
	rm -f $(OBJ) $(TARGET)
	$(MAKE) -C $(PROTO_DIR) clean

clean_run: clean all
	./server

.PHONY: all clean
