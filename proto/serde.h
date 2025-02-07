#pragma once
#include "packet_node.h"
#include "proto_file.h"
#include <stdint.h>


// Assumes that you provide the correct packet deffinition,
// the entire packet is presant, uncompressed, and unencrypted
// see: error_handling.h for what null means
PacketNode* deserialize_packet(struct ProtoList* packets_def, const uint8_t* buffer, size_t size);

