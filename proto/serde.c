#include "serde.h"
#include "packet_node.h"
#include "error_handling.h"
#include "constants.h"

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


NameSpaceSerde* get_namespace(VersionSerde* version, const char* name){
    for (int i = 0; i < MAX_NAMESPACES; i++){
        if (strcmp(version->namespaces[i]->name, name) == 0){
            return version->namespaces[i];
        }
    }
    SET_ERROR_STATE(ERROR_API_USAGE, "Namespace not found: %s! Please refer to current in use proto file for a list of namespaces.", name);
    return NULL;
}

static char process_namespace_version_info(struct ProtoNode* key, struct ProtoNode* value, VersionSerde** state){
    if (key->type != PNT_str){
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Expected string, got %d", key->type);
        exit_on_error();
    }
    if (0==strcmp(key->raw_data, "protocol_number")) {
        struct ResultingNumber number =  proto_node_number(value);
        if (number.is_float) {
            SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Protocol number must be a int");
            exit_on_error();
        }
        (*state)->protocol_number = number.ll;
        printf("Protocol number: %d\n", number.ll);
    }else {
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Unexpected version info entry", key->raw_data);
        exit_on_error();
    }
    return 0;
}

struct GlobalObjState {
    VersionSerde* version;
    int current_ns;
};
static char process_packet_declaration(struct ProtoNode* node, NameSpaceSerde* namespace) {
    struct ProtoNode* id = get_argument_of_type(node, 0, PNT_num);
    struct ProtoNode* name = get_argument_of_type(node, 1, PNT_str);
    struct ResultingNumber n =  proto_node_number(id);
    if (n.is_float) {
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Packet id must not be a float");
        exit_on_error();
    }
    if (namespace->packets[n.ll].name) {
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Duplicate packet ID found on %s with ids %llu\n", namespace->name, n.ll);
        exit_on_error();
    }
    namespace->packets[n.ll].name = id->raw_data;
    namespace->packets[n.ll].definition = node->object.attached_list;

    return 0;
}

static char process_proto_global_object(struct ProtoNode* node, struct GlobalObjState* state){
    if (node->type != PNT_obj){
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Expected object, got %d", node->type);
        exit_on_error();
    }
    if (node->object.name_hash == OBJ_version_info){
        proto_dict_foreach(node->object.attached_dict, (DictCallback)process_namespace_version_info, (void**)&state->version);
    }else if (node->object.name_hash == OBJ_namespace) {
        NameSpaceSerde* namespace = calloc(sizeof(NameSpaceSerde), 1);
        struct ProtoNode* name = get_argument_of_type(node, 0, PNT_str);

        int len = strlen(name->raw_data);

        if (len >= sizeof(namespace->name)) {
            SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Namespace too long");
            exit_on_error();
        }
        memcpy(namespace->name, name->raw_data, len);
        proto_list_foreach(node->object.attached_list, (ListCallback)process_packet_declaration, (void**)namespace);
        state->version->namespaces[state->current_ns++] = namespace;
    }

    return 0;
}

VersionSerde* create_version_serde(const char* proto_file_contents){
    VersionSerde* version = calloc(1, sizeof(VersionSerde));
    version->master_proto_file = parse_proto_file(proto_file_contents);
    proto_list_foreach(version->master_proto_file, (ListCallback)process_proto_global_object, (void**)&((struct GlobalObjState){
        .version = version,
        .current_ns = 0,
    }));

    
    return version;
}