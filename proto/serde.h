#pragma once
#include "zlib.h"




typedef struct {
    char name[512];
    uLong check_sum;
} NamespaceSerde;

typedef struct {
    NamespaceSerde* namespaces[64]; // Basic shitty hashmap.
} PacketSerde;


PacketSerde* PacketSerde_from_proto(const char* proto);
