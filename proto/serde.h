#pragma once
#include "zlib.h"

#define NAMESPACE_HASH_SIZE 64
#define MAX_PACKET_AMOUNT 1024


enum PacketFieldType { PFT_UNKNOWN = 0 };

struct PacketField {
    enum PacketFieldType type;
    char *packet_id;
    union {
        // bool boolean;
        // TODO: Everything else
    };
};
struct Packet {
    struct PacketField fields[128];
};

struct PacketDefinition {
    // TODO
};


typedef struct {
    char name[512];
    uLong check_sum;

    struct PacketDefinition *packets[MAX_PACKET_AMOUNT];
} NamespaceSerde;


typedef struct {
    NamespaceSerde *namespaces[NAMESPACE_HASH_SIZE]; // Basic shitty hashmap using zlib's crc32.
} PacketSerde;


PacketSerde *PacketSerde_from_proto(const char *proto);
