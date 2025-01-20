#pragma once
#include "zlib.h"

#define NAMESPACE_HASH_SIZE 64
#define MAX_PACKET_AMOUNT 1024

struct MinecraftPacket {
    // TODO
};


typedef struct {
    char name[512];
    uLong check_sum;

    struct MinecraftPacket *packets[MAX_PACKET_AMOUNT];
} NamespaceSerde;


typedef struct {
    NamespaceSerde *namespaces[NAMESPACE_HASH_SIZE]; // Basic shitty hashmap.
} PacketSerde;


PacketSerde *PacketSerde_from_proto(const char *proto);
