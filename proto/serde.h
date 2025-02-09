#pragma once
#include "packet_node.h"
#include "proto_file.h"
#include <stdint.h>

struct PacketDeclaration {
    const char* name;
    struct ProtoList* definition;
};

// Assumes that you provide the correct packet deffinition,
// the entire packet is presant, uncompressed, and unencrypted
// see: error_handling.h for what null means
PacketNode* deserialize_packet(struct ProtoList* packets_def, const uint8_t* buffer, size_t size);

typedef struct {
    char name[64];

    // Packet in a namespace by Packet ID (as defined by the wiki and mincreafts protocol)
    struct PacketDeclaration packets[256];


    
} NameSpaceSerde;

#define MAX_NAMESPACES 16
typedef struct {
    int protocol_number;
    struct ProtoList* master_proto_file;

    // With the small amount of namespaces, it makes no sense to make them a hashmap
    // so just strcmp them!
    NameSpaceSerde* namespaces[MAX_NAMESPACES];
} VersionSerde;

// On error will exit the program. It can only fail if the proto file is malformed
VersionSerde* create_version_serde(const char* proto_file_contents);


// Returns NULL if not found, and sets error state
NameSpaceSerde* get_namespace(VersionSerde* version, const char* name);