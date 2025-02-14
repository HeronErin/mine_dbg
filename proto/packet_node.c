#include "packet_node.h"

// Print all the elements of a node tree
// completely GPT generated as this is the boring part
void PN_tree_(const PacketNode *node, int indent) {
    // Print indentation
    for (int i = 0; i < indent; i++)
        printf("  ");

    // Print node header with name and type
    printf("Node: name='%s', type=", node->name);
    switch (node->type) {
        case NT_BUNDLE:
            printf("BUNDLE");
            break;
        case NT_LIST:
            printf("LIST");
            break;
        case NT_BOOLEAN:
            printf("BOOLEAN: %d", node->__data->boolean);
            break;
        case NT_BYTE:
            printf("BYTE: %d", node->__data->byte_);
            break;
        case NT_UBYTE:
            printf("UBYTE: %d", node->__data->Ubyte_);
            break;
        case NT_SHORT:
            printf("SHORT: %d", node->__data->short_);
            break;
        case NT_USHORT:
            printf("USHORT: %u", node->__data->Ushort_);
            break;
        case NT_INT:
            printf("INT: %d", node->__data->int_);
            break;
        case NT_UINT:
            printf("UINT: %u", node->__data->Uint_);
            break;
        case NT_VARINT:
            printf("VARINT: %d", node->__data->varint);
            break;
        case NT_LONG:
            printf("LONG: %lld", (long long) node->__data->long_);
            break;
        case NT_ULONG:
            printf("ULONG: %llu", (unsigned long long) node->__data->Ulong_);
            break;
        case NT_VARLONG:
            printf("VARLONG: %llu", (unsigned long long) node->__data->varlong);
            break;
        case NT_FLOAT:
            printf("FLOAT: %f", node->__data->float_);
            break;
        case NT_DOUBLE:
            printf("DOUBLE: %f", node->__data->double_);
            break;
        case NT_STRING:
            printf("STRING: \"%s\"", node->__data->contents->data);
            break;
        case NT_NBT:
            printf("NBT: [binary, size=%zu]", node->__data->contents->size);
            break;
        case NT_POSITION:
            printf("POSITION: (%d, %d, %d)", node->__data->x, node->__data->y, node->__data->z);
            break;
        case NT_ANGLE:
            printf("ANGLE: %u", node->__data->_raw_angle);
            break;
        case NT_UUID:
            printf("UUID: (%llu, %llu)", (unsigned long long) node->__data->uuid.uuid_high, (unsigned long long) node->__data->uuid.uuid_low);
            break;
        default:
            printf("UNKNOWN");
            break;
    }
    printf("\n");

    // Recurse for composite types
    if (node->type == NT_BUNDLE) {
        for (int i = 0; i < PACKET_NODE_COLLECTION_SIZE; i++) {
            PacketNode *child = node->__data->hashmap[i];
            while (child) {
                PN_tree_(child, indent + 1);
                child = child->_hashmap_next;
            }
        }
    } else if (node->type == NT_LIST) {
        for (int i = 0; i < node->list_size; i++) {
            if (node->__data->children[i])
                PN_tree_(node->__data->children[i], indent + 1);
        }
    }
}
