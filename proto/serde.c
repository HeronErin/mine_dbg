#include "serde.h"
#include "packet_node.h"
#include "error_handling.h"

#define MAX_PACKET_NESTING 32

// Returns: non zero for error(must set error state on error)
// unpacks and sets value of items onto the head
static inline int deserialize_item(struct ProtoNode* item, PacketNode* head, PacketNode** parents, int depth, const uint8_t* buffer, size_t size){
	if (item->type != PNT_obj){
		SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Packets definitions cannot contain anything other than objects, got of type: %d", item->type);
        return -1;
	}
    // TODO: MAKE THIS FAST!
    if (strcmp(item->object.name, "PacketNode") == 0){

    }else{
 		SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Unknown packet definition datatype: %s", item->object.name);
        return -1;
    }
}


PacketNode* _deserialize_packet(PacketNode** parents, int packet_deph, struct ProtoList* packets_def, const uint8_t* buffer, size_t size){
    if (packet_deph >= MAX_PACKET_NESTING) {
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Packet too deeply nested to continue. Please increase MAX_PACKET_NESTING");
        return NULL;
    }
    PacketNode *head = PN_new_bundle();


    PACKET_PARSE_LOOP:
    for (int i = 0; i < PROTO_LIST_SEGMENT_SIZE && packets_def->contents[i]; i++){
		struct ProtoNode* element = packets_def->contents[i];
        if (deserialize_item(element, head, parents, packet_deph, buffer, size))
            return NULL;

    }
	if (packets_def->next){
        packets_def = packets_def->next;
        goto PACKET_PARSE_LOOP;
	}


    return head;
}

PacketNode* deserialize_packet(struct ProtoList* packets_def, const uint8_t* buffer, size_t size){
    PacketNode* parents[MAX_PACKET_NESTING] = {0};

    return _deserialize_packet(parents, 0, packets_def, buffer, size);
}

