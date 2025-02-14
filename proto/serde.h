#pragma once
#include <stdint.h>
#include "packet_node.h"
#include "proto_file.h"

#define ENUM_REGISTRY_SIZE 1024

struct PacketDeclaration {
    const char *name;
    struct ProtoList *definition;
};

struct EnumRegistryEntry {
    char *name;
    uint64_t name_hash;

    // Hashmaps need this
    struct EnumRegistryEntry *next;
};


// Assumes that you provide the correct packet deffinition,
// the entire packet is presant, uncompressed, and unencrypted
// see: error_handling.h for what null means
PacketNode *deserialize_packet(struct ProtoList *packets_def, const char *buffer, size_t size);


// Simular to deserialize_packet, but allowing for multiple layers down
PacketNode *_deserialize_packet(PacketNode **parents, int packet_deph, struct ProtoList *packets_def, const char **buffer,
                                const char *max_buffer);

typedef struct {
    char name[64];

    // Packet in a namespace by Packet ID (as defined by the wiki and mincreafts protocol)
    struct PacketDeclaration packets[256];
} NameSpaceSerde;

#define MAX_NAMESPACES 16
typedef struct {
    int protocol_number;
    struct ProtoList *master_proto_file;

    // With the small amount of namespaces, it makes no sense to make them a hashmap
    // so just strcmp them!
    NameSpaceSerde *namespaces[MAX_NAMESPACES];

    // Hashmap, by enum name, of all the registered enums
    // in the proto file.
    struct EnumRegistryEntry *enum_registry[ENUM_REGISTRY_SIZE];

} VersionSerde;

// On error will exit the program. It can only fail if the proto file is malformed
VersionSerde *create_version_serde(const char *proto_file_contents);


// Returns NULL if not found, and sets error state
NameSpaceSerde *get_namespace(VersionSerde *version, const char *name);
