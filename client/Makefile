# server/Makefile

# Compiler and flags
CC = ccache gcc
CFLAGS = -g -I../proto -I../proto/constants -lz

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

%.o: %.c FORCE
	$(CC) $(CFLAGS) -c $< -o $@
FORCE:

$(PROTO_LIB): FORCE
	$(MAKE) -C $(PROTO_DIR)

clean:
	rm -f $(OBJ) $(TARGET)
	$(MAKE) -C $(PROTO_DIR) clean

run:  all
	./$(TARGET)

.PHONY: all