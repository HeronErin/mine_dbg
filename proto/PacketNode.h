#pragma once
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xxhash.h"

enum NodeType {
    NT_NAMESAPCE,

    // Hashmap style dictionary
    NT_BUNDLE,

    // Native MC datatypes (in native machine format):
    NT_BOOLEAN,
    NT_BYTE,
    NT_UBYTE,
    NT_SHORT,
    NT_USHORT,
    NT_INT,
    NT_UINT,
    NT_LONG,
    NT_ULONG,
    NT_FLOAT,
    NT_DOUBLE,

    NT_VARINT,
    NT_VARLONG,

    // Binary container type values
    NT_STRING,
    NT_NBT,

    // Custom MC objects
    NT_POSITION,
    NT_ANGLE,
    NT_UUID

};
#define PACKET_NODE_HASHMAP_SIZE 1024
struct PacketBufferContents {
    size_t size;
    char data[];
};

// Not malloced by itself
union __PacketNodeData {
    // For list type nodes:
    // NT_NAMESAPCE, NT_PACKET_LIST, TODO
    struct PacketNode_ *children[128];

    // Used for: NT_BUNDLE
    struct PacketNode_ *hashmap[PACKET_NODE_HASHMAP_SIZE];

    uint8_t boolean;
    int8_t byte_;
    int8_t Ubyte_;

    int16_t short_;
    uint16_t Ushort_;

    int32_t int_;
    int32_t varint;
    uint32_t Uint_;

    int64_t long_;
    int64_t varlong;
    uint64_t Ulong_;

    float float_;
    double double_;

    // For binary container type nodes:
    // NT_STRING, NT_NBT
    struct PacketBufferContents *contents;

    // MC special position format NT_POSITION
    struct {
        int32_t x;
        int32_t y;
        int32_t z;
    };

    // NT_ANGLE
    struct {
        // 1/256 of a full rotation
        uint8_t _raw_angle;
    };

    // NT_UUID
    struct {
        uint64_t uuid_high;
        uint64_t uuid_low;
    };
};

// Needs to be calloc-ed, otherwise name cannot be properly hashed
struct PacketNode_ {
    char name[32];
    enum NodeType type;


    // ONLY WHAT YOU NEED IS PRESENT!
    // Holds the raw data for nodes
    union __PacketNodeData __data[];
};

typedef struct PacketNode_ PacketNode;

static inline PacketNode *_PN_alloc(size_t data_size) { return calloc(1, sizeof(PacketNode) + data_size); }


static inline PacketNode *PN_rename(PacketNode *node, const char *name) {
    size_t size = strlen(name) + 1;
    if (size >= sizeof(node->name)) {
        fprintf(stderr, "ERROR: PacketNode name too long: \"%s\"\n", name);
        exit(1);
    }
    memcpy(node->name, name, size);
    return node;
}


static inline PacketNode *PN_from_string(const char *name) {
    size_t size = strlen(name) + 1;
    struct PacketBufferContents *contents = malloc(size + sizeof(struct PacketBufferContents));
    PacketNode *ret = _PN_alloc(sizeof(size_t));
    ret->type = NT_STRING;
    ret->__data->contents = contents;

    contents->size = size;
    memcpy(contents->data, name, size);
    return ret;
}
static inline char *PN_get_string(PacketNode *node) {
    assert(node->type == NT_STRING);
    return node->__data->contents->data;
}
static inline void PN_set_string(PacketNode *node, const char *name) {
    assert(node->type == NT_STRING);
    size_t old_size = node->__data->contents->size;
    size_t new_size = strlen(name) + 1;

    if (old_size >= new_size) {
        // Technically this leaves a bit of memory unused, so what!
        node->__data->contents->size = new_size;
        memcpy(node->__data->contents->data, name, new_size);
        return;
    }

    free(node->__data->contents);
    struct PacketBufferContents *contents = malloc(new_size + sizeof(struct PacketBufferContents));
    contents->size = new_size;
    memcpy(contents->data, name, new_size);
    node->__data->contents = contents;
}


#define H_KEY(NAME) XXH64((NAME), 32, 0xFEEDBEEF69)

static inline uint64_t PN_get_hashcode(PacketNode *node) {
    if (!node->name[0]) {
        perror("ERROR: Ilegal attempt as adding at hashing an un-named node!");
        exit(1);
    }
    return H_KEY(node->name);
}
static inline PacketNode *PN_new_bundle() {
    PacketNode *ret = _PN_alloc(sizeof(ret->__data->hashmap));
    ret->type = NT_BUNDLE;

    return ret;
}


#define _PACKET_NODE_INIT(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                                                                        \
    static inline PacketNode *PN_from_##FUNCTION_NAME_ADDON(ELEMENT_TYPE arg) {                                                                                                    \
        PacketNode *ret = _PN_alloc(sizeof(ret->__data->ELEMENT_NAME));                                                                                                            \
        ret->type = ELEMENT_TYPE_ID;                                                                                                                                               \
        ret->__data->ELEMENT_NAME = arg;                                                                                                                                           \
        return ret;                                                                                                                                                                \
    }

#define _PACKET_NODE_GETTER(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                                                                      \
    static inline ELEMENT_TYPE PN_get_##FUNCTION_NAME_ADDON(PacketNode *node) {                                                                                                    \
        assert(node->type == ELEMENT_TYPE_ID);                                                                                                                                     \
        return node->__data->ELEMENT_NAME;                                                                                                                                         \
    }
#define _PACKET_NODE_SETTER(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                                                                      \
    static inline void PN_set_##FUNCTION_NAME_ADDON(PacketNode *node, ELEMENT_TYPE value) {                                                                                        \
        assert(node->type == ELEMENT_TYPE_ID);                                                                                                                                     \
        node->__data->ELEMENT_NAME = value;                                                                                                                                        \
    }
#define _PACKET_NODE_GEN_FUNCS(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                                                                   \
    _PACKET_NODE_INIT(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                                                                            \
    _PACKET_NODE_GETTER(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                                                                          \
    _PACKET_NODE_SETTER(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)


_PACKET_NODE_GEN_FUNCS(boolean, boolean, int8_t, NT_BOOLEAN)
_PACKET_NODE_GEN_FUNCS(byte, byte_, int8_t, NT_BYTE)
_PACKET_NODE_GEN_FUNCS(ubyte, Ubyte_, uint8_t, NT_UBYTE)
_PACKET_NODE_GEN_FUNCS(short, short_, int16_t, NT_SHORT)
_PACKET_NODE_GEN_FUNCS(ushort, Ushort_, uint16_t, NT_USHORT)
_PACKET_NODE_GEN_FUNCS(int, int_, int32_t, NT_INT)
_PACKET_NODE_GEN_FUNCS(uint, Uint_, int32_t, NT_UINT)
_PACKET_NODE_GEN_FUNCS(varint, varint, int32_t, NT_VARINT)
_PACKET_NODE_GEN_FUNCS(long, long_, int64_t, NT_LONG)
_PACKET_NODE_GEN_FUNCS(ulong, Ulong_, uint64_t, NT_ULONG)
_PACKET_NODE_GEN_FUNCS(varlong, varlong, uint64_t, NT_VARLONG)
_PACKET_NODE_GEN_FUNCS(float, float_, float, NT_FLOAT)
_PACKET_NODE_GEN_FUNCS(double, double_, double, NT_DOUBLE)

_PACKET_NODE_GEN_FUNCS(string_raw, contents, struct PacketBufferContents *, NT_STRING)
_PACKET_NODE_GEN_FUNCS(NBT_raw, contents, struct PacketBufferContents *, NT_NBT)
