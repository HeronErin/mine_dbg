#include "serde.h"
#include "proto_file.h"

#include <stdlib.h>
#include <assert.h>


PacketSerde* PacketSerde_from_proto(const char* proto) {

    PacketSerde* serde = (PacketSerde*)calloc(1, sizeof(PacketSerde));

    struct ProtoList* raw_proto = parse_proto_file(proto);


    for (struct ProtoList* curr = raw_proto; curr != NULL; curr = curr->next) {
        for (int i = 0; i < PROTO_LIST_SEGMENT_SIZE && curr->contents[i]; i++) {
            struct ProtoNode* node = curr->contents[i];

            assert_proto_node_object_name(node, "namespace");
//			get_argument_of_type(node, 0, PNT_str)
        }
    }

    return serde;
}
