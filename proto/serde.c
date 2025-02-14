#include "serde.h"

#include <errno.h>
#include <endian.h>

#include "constants.h"
#include "datatypes.h"
#include "error_handling.h"
#include "packet_node.h"

#define MAX_PACKET_NESTING 32

// Returns: non zero for error(must set error state on error)
// unpacks and sets value of items onto the head
static int deserialize_item(struct ProtoNode* item, PacketNode* head, PacketNode** parents, int depth, const char **buffer, const char *maxBuffer){
	if (item->type != PNT_obj){
		SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Packets definitions cannot contain anything other than objects, got of type: %d", item->type);
        return -1;
	}
    struct ProtoNode* datatype_name = item->object.arguments->contents[0];

    #define _FORCE_NAME() \
            if (datatype_name == NULL || datatype_name->type != PNT_str) { \
                SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Packet datatypes MUST have their first argument be an identifiable name! Found in %s", item->object.name); \
                return -1;\
            }

    char* name = NULL;
    uint64_t name_hash = 0;
    if (datatype_name != NULL && datatype_name->type == PNT_str) {
        name = datatype_name->raw_data;
        name_hash = PN_str_hash(name);
    }

    #define _MEM_ERROR_CHECK(ADD_TO_BUFF, NAME) if (*buffer + (ADD_TO_BUFF) >= maxBuffer){\
        SET_ERROR_STATE(ERROR_INVALID_PACKET, "Size is too small for " NAME "\n");\
        return -1;\
        }


    // Endianness does not affect single bytes
    #define be8toh(B) (B)
    #define _CASE_PRIMITIVE(BITS, NAME, HASH) case HASH: {\
        _FORCE_NAME();\
        _MEM_ERROR_CHECK(BITS / 8, #NAME);\
        _PNB_set_with_hash_##NAME(\
            head, name, name_hash, be##BITS##toh(  *(uint##BITS##_t*)(*buffer)   )\
        );\
        *buffer += (BITS / 8);\
        break;\
    }
    switch (item->object.name_hash) {
        _CASE_PRIMITIVE(8, boolean, OBJ_boolean)
        _CASE_PRIMITIVE(8, byte, OBJ_byte)
        _CASE_PRIMITIVE(8, ubyte, OBJ_Ubyte)
        _CASE_PRIMITIVE(16, short, OBJ_short)
        _CASE_PRIMITIVE(16, ushort, OBJ_Ushort)
        _CASE_PRIMITIVE(32, int, OBJ_int)
        _CASE_PRIMITIVE(32, uint, OBJ_Uint)
        _CASE_PRIMITIVE(64, long, OBJ_long)
        _CASE_PRIMITIVE(64, ulong, OBJ_Ulong)
        case OBJ_uuid: {
            _FORCE_NAME();
            _MEM_ERROR_CHECK(128 / 8, "uuid");
            uint64_t uuid_p1 = be64toh( *(uint64_t *) *buffer);
            *buffer += 64 / 8;
            uint64_t uuid_p2 = be64toh( *(uint64_t *) *buffer);
            *buffer += 64 / 8;

            _PNB_set_with_hash_uuid(head, name, name_hash, (struct MC_uuid){
                .uuid_low = uuid_p1,
                .uuid_high = uuid_p2
            });
            break;
        }
        case OBJ_varint: {
            _FORCE_NAME();
            uint32_t val = (uint32_t) readVarStyle(buffer, maxBuffer, 32);
            if (errno) {
                SET_ERROR_STATE(ERROR_INVALID_PACKET, "Varint memory error");
                return -1;
            }
            _PNB_set_with_hash_varint(head, name, name_hash, val);
            break;
        }
        case OBJ_varlong: {
            _FORCE_NAME();
            uint64_t val = (uint64_t) readVarStyle(buffer, maxBuffer, 64);
            if (errno) {
                SET_ERROR_STATE(ERROR_INVALID_PACKET, "Varint memory error");
                return -1;
            }
            _PNB_set_with_hash_varlong(head, name, name_hash, val);
            break;
        }
        case OBJ_string: {
            _FORCE_NAME();
            uint32_t size = (uint32_t) readVarStyle(buffer, maxBuffer, 32);
            if (errno) {
                SET_ERROR_STATE(ERROR_INVALID_PACKET, "Varint memory error");
                return -1;
            }
            // Second string arg is for the length of the string
            if (item->object.arguments->contents[1] != NULL) {
                if (item->object.arguments->contents[1]->type != PNT_num) {
                    NOT_INT:
                    SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "The string datatypes second argument CAN ONLY BE AND INT");
                    return -1;
                }
                if (item->object.arguments->contents[1]->parsed_number.is_float)
                    goto NOT_INT;
                if (size > item->object.arguments->contents[1]->parsed_number.ll) {
                    SET_ERROR_STATE(ERROR_INVALID_PACKET, "String of size max size %llu had size of %d", item->object.arguments->contents[1]->parsed_number.ll, size);
                    return -1;
                }
            }

            struct PacketBufferContents* contents = malloc(1 + size + sizeof(struct PacketBufferContents));
            contents->size = size;

            _MEM_ERROR_CHECK(size, "string");

            memcpy(contents->data, *buffer, size);
            contents->data[size] = '\0';

            *buffer += size;
            _PNB_set_with_hash_string_raw(head, name, name_hash, contents);
            break;
        }
        case OBJ_prefixed_byte_array: {
            _FORCE_NAME();
            uint32_t size = (uint32_t) readVarStyle(buffer, maxBuffer, 32);
            if (errno) {
                SET_ERROR_STATE(ERROR_INVALID_PACKET, "Varint memory error");
                return -1;
            }
            struct PacketBufferContents* contents = malloc(size + sizeof(struct PacketBufferContents));
            contents->size = size;

            _MEM_ERROR_CHECK(size, "prefixed byte array");

            memcpy(contents->data, *buffer, size);
            *buffer += size;
            _PNB_set_with_hash_byte_array_raw(head, name, name_hash, contents);
            break;
        }
        case OBJ_prefixed_optional: {
            // Two modes exist for a prefixed option. As a modifier to a single
            // data item, or as a container. The two are distinguished by using
            // arguments vs an attached list
            _MEM_ERROR_CHECK(1, "prefixed optional");
            char is_present = **buffer;
            (*buffer)++;

            // Skip contents if non existent
            if (!is_present) break;

            // If it has a list, assume it is a list style
            if (item->object.attached_list) {
                _FORCE_NAME();

                if (depth + 1 > MAX_PACKET_NESTING) {
                    SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Max depth reached!");
                    return -1;
                }
                parents[depth] = head;
                PacketNode* contents =  _deserialize_packet(parents, depth + 1, item->object.attached_list, buffer, maxBuffer);
                PN_rename(contents, name);
                PNB_set(head, contents);
            }else {
                // Otherwise assume it is a singular object
                struct ProtoNode* opt = item->object.arguments->contents[0];
                if (NULL == opt || opt->type != PNT_obj) {
                    SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "prefixed_optional MUST, either have an attached list, or a singular object!");
                    return -1;
                }
                // Shares same context
                if (deserialize_item(opt, head, parents, depth, buffer, maxBuffer))
                    return -1;
            }

            break;
        }
        default:
            SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Unknown packet definition datatype: %s", item->object.name);
            return -1;
    }
    return 0;
}


PacketNode* _deserialize_packet(PacketNode** parents, int packet_deph, struct ProtoList* packets_def, const char** buffer, const char* max_buffer){
    if (packet_deph >= MAX_PACKET_NESTING) {
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Packet too deeply nested to continue. Please increase MAX_PACKET_NESTING");
        return NULL;
    }
    PacketNode *head = PN_new_bundle();



    PACKET_PARSE_LOOP:
    for (int i = 0; i < PROTO_LIST_SEGMENT_SIZE && packets_def->contents[i]; i++){
		struct ProtoNode* element = packets_def->contents[i];
        if (deserialize_item(element, head, parents, packet_deph, buffer, max_buffer))
            return NULL;


        if (*buffer >= max_buffer) {
            SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Packet too short");
            return NULL;
        }
    }
	if (packets_def->next){
        packets_def = packets_def->next;
        goto PACKET_PARSE_LOOP;
	}


    return head;
}

PacketNode* deserialize_packet(struct ProtoList* packets_def, const char* buffer, size_t size){
    PacketNode* parents[MAX_PACKET_NESTING] = {0};

    return _deserialize_packet(parents, 0, packets_def, &buffer, buffer + size);
}


NameSpaceSerde* get_namespace(VersionSerde* version, const char* name){
    for (int i = 0; i < MAX_NAMESPACES && version->namespaces[i]->name; i++){
        if (strcmp(version->namespaces[i]->name, name) == 0){
            return version->namespaces[i];
        }
    }
    SET_ERROR_STATE(ERROR_API_USAGE, "Namespace not found: %s! Please refer to current in use proto file for a list of namespaces.", name);
    return NULL;
}

static char process_namespace_version_info(struct ProtoNode* key, struct ProtoNode* value, VersionSerde* state){
    if (key->type != PNT_str){
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Expected string, got %d", key->type);
        exit_on_error();
    }
    if (0==strcmp(key->raw_data, "protocol_number")) {
        if (value->parsed_number.is_float) {
            SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Protocol number must be a int");
            exit_on_error();
        }
        state->protocol_number = value->parsed_number.ll;
        printf("Protocol number: %llu\n", value->parsed_number.ll);
    }else {
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Unexpected version info entry: %s", key->raw_data);
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

    if (id->parsed_number.is_float) {
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Packet id must not be a float");
        exit_on_error();
    }
    if (namespace->packets[id->parsed_number.ll].name) {
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Duplicate packet ID found on %s with ids %llu\n", namespace->name, id->parsed_number.ll);
        exit_on_error();
    }
    namespace->packets[id->parsed_number.ll].name = id->raw_data;
    namespace->packets[id->parsed_number.ll].definition = node->object.attached_list;

    return 0;
}

static char process_enum_registration(struct ProtoNode* key, struct ProtoNode* value, VersionSerde* vserde) {
    if (key->type != PNT_str) {
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Enum id must a string!");
        exit_on_error();
    }
    if (value->type != PNT_obj || value->object.name_hash != OBJ_enum) {
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Enum values must be declared using the enum(){INT : STRING} format!!");
        exit_on_error();
    }
    uint64_t hash = PN_str_hash(key->escaped_string);
    struct EnumRegistryEntry** write_too = &vserde->enum_registry[hash % ENUM_REGISTRY_SIZE];
    while (*write_too) {
        if ((*write_too)->name_hash == hash) {
            SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Duplicate enum contents in registry! %s", key->escaped_string);
            exit_on_error();
        }
        write_too = &((*write_too)->next);
    }






    return 0;
}
static char process_proto_global_object(struct ProtoNode* node, struct GlobalObjState* state){
    if (node->type != PNT_obj){
        SET_ERROR_STATE(ERROR_INVALID_PACKET_FORMAT, "Expected object, got %d", node->type);
        exit_on_error();
    }
    if (node->object.name_hash == OBJ_version_info){
        proto_dict_foreach(node->object.attached_dict, (DictCallback)process_namespace_version_info, (void**)state->version);
    } else if (node->object.name_hash == OBJ_enums) {
        proto_dict_foreach(node->object.attached_dict, (DictCallback)process_enum_registration,  (void**)state->version);
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