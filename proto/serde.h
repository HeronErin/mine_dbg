#pragma once
#include "PacketNode.h"
#include "proto_file.h"



// Assumes that you provide the correct packet deffinition,
// the entire packet is presant, uncompressed, and unencrypted
PacketNode* deserialize_packet(struct ProtoObject packet_def, const uint8_t* buffer, size_t size);

