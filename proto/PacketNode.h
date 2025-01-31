#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum NodeType {
    NT_NAMESAPCE,
    NT_PACKET_LIST,

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

    // Binary container type values
    NT_STRING,
    NT_NBT,

    // Custom MC objects
    NT_POSITION,
    NT_ANGLE,
    NT_UUID

};
struct PacketBufferContents {
    size_t size;
    char data[];
};
union __PacketNodeData {
    // For list type nodes:
    // NT_NAMESAPCE, NT_PACKET_LIST, TODO
    struct PacketNode_ *children[128];


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

struct PacketNode_ {
    char name[32];
    enum NodeType type;

    //  ONLY WHAT YOU NEED IS PRESENT!
    union __PacketNodeData __data[];
};

typedef struct PacketNode_ PacketNode;

static inline PacketNode *_PacketNode_alloc(size_t data_size) { return calloc(1, sizeof(PacketNode) + data_size); }


#define _PACKET_NODE_INIT(FUNCTION_NAME_ADDON, ELEMENT_NAME, ELEMENT_TYPE)                                                                                                         \
    static inline PacketNode *PacketNode_from_##FUNCTION_NAME_ADDON(ELEMENT_TYPE arg) {                                                                                            \
        PacketNode *ret = _PacketNode_alloc(sizeof(ret->__data->ELEMENT_NAME));                                                                                                    \
        ret->__data->ELEMENT_NAME = arg;                                                                                                                                           \
        return ret;                                                                                                                                                                \
    }

_PACKET_NODE_INIT(boolean, boolean, uint8_t)
_PACKET_NODE_INIT(byte, byte_, int8_t)
_PACKET_NODE_INIT(ubyte, Ubyte_, uint8_t)
_PACKET_NODE_INIT(short, short_, int16_t)
_PACKET_NODE_INIT(ushort, Ushort_, uint16_t)
_PACKET_NODE_INIT(int, int_, int32_t)
_PACKET_NODE_INIT(uint, Uint_, int32_t)
_PACKET_NODE_INIT(varint, varint, int32_t)
_PACKET_NODE_INIT(long, long_, int64_t)
_PACKET_NODE_INIT(ulong, Ulong_, uint64_t)
_PACKET_NODE_INIT(varlong, varlong, uint64_t)
_PACKET_NODE_INIT(float, float_, float)
_PACKET_NODE_INIT(double, double_, double)
