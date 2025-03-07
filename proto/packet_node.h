#pragma once
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constants.h"

#include "xxhash.h"

enum NodeType {
    // Hashmap style dictionary
    NT_BUNDLE,

    NT_LIST,

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
    NT_BYTE_ARRAY,

    // Custom MC objects
    NT_POSITION,
    NT_ANGLE,
    NT_UUID

};


struct MC_enumValue {
    long long value;
    const char *string;
};


// Max size of both bundles AND lists
#define PACKET_NODE_COLLECTION_SIZE 1024
struct PacketBufferContents {
    size_t size;
    char data[];
};

struct MC_uuid {
    uint64_t uuid_high;
    uint64_t uuid_low;
};
// Not malloced by itself
union __PacketNodeData {
    // For list type nodes:
    //  NT_PACKET_LIST, TODO
    struct PacketNode_ *children[PACKET_NODE_COLLECTION_SIZE];

    // Used for: NT_BUNDLE
    struct PacketNode_ *hashmap[PACKET_NODE_COLLECTION_SIZE];


    uint8_t boolean;

    // Any of these _could_ be an enum
    struct {
        union {
            int8_t byte_;
            uint8_t Ubyte_;

            int16_t short_;
            uint16_t Ushort_;

            int32_t int_;
            int32_t varint;
            uint32_t Uint_;

            int64_t long_;
            int64_t varlong;
            uint64_t Ulong_;
        };
    };


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
    struct MC_uuid uuid;
};

#define PACKET_KEY_NAME_LEN MAX_STRING_CONST_SIZE

// Needs to be calloc-ed, otherwise name cannot be properly hashed
struct PacketNode_ {
    char name[PACKET_KEY_NAME_LEN];
    enum NodeType type;

    // Most of the time ignore this. It is only for hashmaps when
    // a collision occurs
    struct PacketNode_ *_hashmap_next;
    uint64_t full_hash;

    // Only used for lists
    int list_size;


    // ONLY WHAT YOU NEED IS PRESENT!
    // Holds the raw data for nodes
    union __PacketNodeData __data[];
};

typedef struct PacketNode_ PacketNode;

static __always_inline PacketNode *_PN_alloc(size_t data_size) { return calloc(1, sizeof(PacketNode) + data_size); }

static __always_inline uint64_t PN_str_hash(const char *str) {
    char temp[PACKET_KEY_NAME_LEN] = {0};
    size_t size = strlen(str) + 1;
    if (size >= PACKET_KEY_NAME_LEN) {
        fprintf(stderr, "ERROR: name too long to fit in packet node key: \"%s\"\n", str);
        exit(1);
    }
    memcpy(temp, str, size);
    return XXH64(temp, PACKET_KEY_NAME_LEN, 0);
}

static __always_inline PacketNode *PN_rename(PacketNode *node, const char *name) {
    node->full_hash = PN_str_hash(name);
    strncpy(node->name, name, PACKET_KEY_NAME_LEN - 1);
    return node;
}


static __always_inline PacketNode *PN_from_string(const char *name) {
    size_t size = strlen(name) + 1;
    struct PacketBufferContents *contents = malloc(size + sizeof(struct PacketBufferContents));
    PacketNode *ret = _PN_alloc(sizeof(size_t));
    ret->type = NT_STRING;
    ret->__data->contents = contents;

    contents->size = size;
    memcpy(contents->data, name, size);
    return ret;
}
static __always_inline char *PN_get_string(PacketNode *node) {
    assert(node->type == NT_STRING);
    return node->__data->contents->data;
}
static __always_inline void PN_set_string(PacketNode *node, const char *name) {
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

static __always_inline PacketNode *PN_new_bundle() {
    PacketNode *ret = _PN_alloc(sizeof(ret->__data->hashmap));
    ret->type = NT_BUNDLE;

    return ret;
}
static __always_inline PacketNode *PN_new_list() {
    PacketNode *ret = _PN_alloc(sizeof(ret->__data->children));
    ret->type = NT_LIST;
    ret->list_size = 0;
    return ret;
}

static __always_inline void PN_list_append(PacketNode *list, PacketNode *child) {
    assert(list->type == NT_LIST);
    assert(list->list_size < PACKET_NODE_COLLECTION_SIZE);
    list->__data->children[list->list_size++] = child;
}

static __always_inline PacketNode *PN_list_get(PacketNode *list, int index) {
    assert(list->type == NT_LIST);
    assert(index >= 0 && index < list->list_size);
    return list->__data->children[index];
}
static inline void PN_free(PacketNode *node) {
    if (node->_hashmap_next)
        PN_free(node->_hashmap_next);

    switch (node->type) {
        case NT_STRING:
        case NT_NBT:
            free(node->__data->contents);
            break;
        case NT_BUNDLE:
            for (int i = 0; i < PACKET_NODE_COLLECTION_SIZE; i++) {
                if (node->__data->hashmap[i])
                    PN_free(node->__data->hashmap[i]);
            }
            break;
        case NT_LIST:
            for (int i = 0; i < node->list_size; i++) {
                if (node->__data->children[i])
                    PN_free(node->__data->children[i]);
            }
            break;

        default:
    }


    free(node);
}

// Puts an element onto a hashmap/bundle.
// WARNING: **Takes ownership of value!**
static __always_inline void PNB_set(PacketNode *root_bundle, PacketNode *value) {
    assert(root_bundle->type == NT_BUNDLE);
    size_t hash = value->full_hash;
    size_t modhash = hash % PACKET_NODE_COLLECTION_SIZE;

    PacketNode **hashmap_element = &root_bundle->__data->hashmap[modhash];
    while (*hashmap_element) {
        // Save if key of same value was found
        // note: assuming xxhash is collision free
        if ((*hashmap_element)->full_hash == hash) {
            PN_free(*hashmap_element);
            *hashmap_element = value;
            return;
        }

        hashmap_element = &(*hashmap_element)->_hashmap_next;
    }
    *hashmap_element = value;
}
// Get the element of a bundle by the hash of a name
static __always_inline PacketNode *PNB_hget(PacketNode *root_bundle, uint64_t hash) {
    assert(root_bundle->type == NT_BUNDLE);
    size_t modhash = hash % PACKET_NODE_COLLECTION_SIZE;
    PacketNode *hashmap_element = root_bundle->__data->hashmap[modhash];

    while (hashmap_element && hashmap_element->full_hash != hash) {
        hashmap_element = hashmap_element->_hashmap_next;
    }

    if (!hashmap_element)
        return NULL;

    if (hashmap_element->full_hash != hash)
        return NULL;
    return hashmap_element;
}
// Get the element on a bundle by name
static __always_inline PacketNode *PNB_get(PacketNode *root_bundle, char *e) { return PNB_hget(root_bundle, PN_str_hash(e)); }


#define _PACKET_NODE_INIT(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                                \
    static __always_inline PacketNode *PN_from_##FUNCTION_NAME_ADDON(ELEMENT_TYPE arg) {                                                   \
        PacketNode *ret = _PN_alloc(sizeof(ret->__data->ELEMENT_NAME));                                                                    \
        ret->type = ELEMENT_TYPE_ID;                                                                                                       \
        ret->__data->ELEMENT_NAME = arg;                                                                                                   \
        return ret;                                                                                                                        \
    }

#define _PACKET_NODE_GETTER(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                              \
    static __always_inline ELEMENT_TYPE PN_get_##FUNCTION_NAME_ADDON(PacketNode *node) {                                                   \
        assert(node->type == ELEMENT_TYPE_ID);                                                                                             \
        return node->__data->ELEMENT_NAME;                                                                                                 \
    }
#define _PACKET_NODE_SETTER(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                              \
    static __always_inline void PN_set_##FUNCTION_NAME_ADDON(PacketNode *node, ELEMENT_TYPE value) {                                       \
        assert(node->type == ELEMENT_TYPE_ID);                                                                                             \
        node->__data->ELEMENT_NAME = value;                                                                                                \
    }
#define _PACKET_BUNDLE_QUICK_SET(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                         \
    static __always_inline void PNB_set_##FUNCTION_NAME_ADDON(PacketNode *node, char *name, ELEMENT_TYPE value) {                          \
        PacketNode *element = PN_from_##FUNCTION_NAME_ADDON(value);                                                                        \
        PN_rename(element, name);                                                                                                          \
        PNB_set(node, element);                                                                                                            \
    }                                                                                                                                      \
    static __always_inline void _PNB_set_with_hash_##FUNCTION_NAME_ADDON(PacketNode *node, char *name, uint64_t hash,                      \
                                                                         ELEMENT_TYPE value) {                                             \
        PacketNode *element = PN_from_##FUNCTION_NAME_ADDON(value);                                                                        \
        strncpy(node->name, name, PACKET_KEY_NAME_LEN - 1);                                                                                \
        node->full_hash = hash;                                                                                                            \
    }
#define _PACKET_BUNDLE_QUICK_GET(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                         \
    static __always_inline ELEMENT_TYPE PNB_get_##FUNCTION_NAME_ADDON(PacketNode *node, char *name) {                                      \
        PacketNode *element = PNB_get(node, name);                                                                                         \
        if (!element) {                                                                                                                    \
            fprintf(stderr, "Could not resolve element \"%s\" of type " #FUNCTION_NAME_ADDON "\n", name);                                  \
            exit(1);                                                                                                                       \
        }                                                                                                                                  \
        return PN_get_##FUNCTION_NAME_ADDON(element);                                                                                      \
    }

#define _PACKET_NODE_GEN_FUNCS(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                           \
    _PACKET_NODE_INIT(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                                    \
    _PACKET_NODE_GETTER(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                                  \
    _PACKET_NODE_SETTER(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                                  \
    _PACKET_BUNDLE_QUICK_SET(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)                                             \
    _PACKET_BUNDLE_QUICK_GET(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE, ELEMENT_TYPE_ID)


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
_PACKET_NODE_GEN_FUNCS(uuid, uuid, struct MC_uuid, NT_UUID)

_PACKET_NODE_GEN_FUNCS(string_raw, contents, struct PacketBufferContents *, NT_STRING)
_PACKET_NODE_GEN_FUNCS(byte_array_raw, contents, struct PacketBufferContents *, NT_BYTE_ARRAY)
_PACKET_NODE_GEN_FUNCS(NBT_raw, contents, struct PacketBufferContents *, NT_NBT)


void PN_tree_(const PacketNode *node, int indent);

static __always_inline void PN_tree(const PacketNode *node) { PN_tree_(node, 0); }
