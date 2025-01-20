#include "serde.h"
#include "proto_file.h"

#include <stdlib.h>
#include <assert.h>
#include "zlib.h"


static char create_packet_from_packet_protonode(struct ProtoNode * node, NamespaceSerde** ns_){
    NamespaceSerde* ns = *ns_;
	assert_proto_node_object_name(node, "packet");

	struct ResultingNumber number = proto_node_number(get_argument_of_type(node, 0, PNT_num));
    assert(!number.is_float);
	long long pid = number.ll;

    if (pid >= MAX_PACKET_AMOUNT || pid < 0) {
        fprintf(stderr, "Unsupported packet id: %lld\n", pid);
        exit(1);
    }


    return 0;
}

static char create_namespace_from_namespace_protonode(struct ProtoNode * node, PacketSerde** serde_){
    PacketSerde * serde = *serde_;

    assert_proto_node_object_name(node, "namespace");
	char* namespace_name = unescape_string(get_argument_of_type(node, 0, PNT_str)->raw_data);

    NamespaceSerde* namespace_serde = (NamespaceSerde*)calloc(1, sizeof(NamespaceSerde));

    strncpy(namespace_serde->name, namespace_name, 512 - 1);
    free(namespace_name);

    namespace_serde->check_sum = crc32(0L, (unsigned char*) namespace_serde->name, 512);
    int ns_loc = namespace_serde->check_sum % NAMESPACE_HASH_SIZE;
    if (serde->namespaces[ns_loc]){
        fprintf(stderr, "Duplicate namespace found: %s with %s\n", namespace_serde->name, serde->namespaces[ns_loc]->name);
        exit(1);
    }
    serde->namespaces[ns_loc] = namespace_serde;
	if (node->object.attached_list)
        proto_list_foreach(node->object.attached_list, (ListCallback)&create_packet_from_packet_protonode, (void**)&namespace_serde);

    return 0;
}

PacketSerde* PacketSerde_from_proto(const char* proto) {
    PacketSerde* serde = (PacketSerde*)calloc(1, sizeof(PacketSerde));

    struct ProtoList* raw_proto = parse_proto_file(proto);

	proto_list_foreach(raw_proto, (ListCallback)&create_namespace_from_namespace_protonode, (void**)&serde);
    return serde;
}
