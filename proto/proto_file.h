#pragma once
/* =======================================================================
   |                                                                     |
   |                   Basic .proto file format.                         |
   |                                                                     |
   =======================================================================


  .proto files are my own format for handling the ever-changing minecraft
  protocol. It is a recursive format for storing data. As it is made to
  be simple, I will just keep to specification contained in this header.

 Data Types in .proto Files
  1. Numbers:
     - Supported formats: decimal, hexadecimal, and binary.
       - Hexadecimal: prefixed with `0x`.
       - Binary: prefixed with `0b`.
       - Decimal: the default base-10 format, supports integer and
floating-point values.

  2. Strings:
     - Enclosed in double quotation marks (`""`).
     - Supports C-style escape sequences but does not include more complex
features.

  3. Objects:
     - The primary and most common data type, resembling function calls.
       - Example: `Ushort("port")`.
     - Arguments can be of two types:
       - Function-style arguments:
         Implementation-defined, passed within parentheses.
       - Composite arguments:
         Can use either List or Dict modes.

 Composite Argument Modes:
  - List Mode:
    - Enclosed in square brackets (`[]`).
    - Elements are separated by commas.
    - Example: `foo()[1, 2, 3]`.

  - Dict Mode:
    - Enclosed in curly brackets (`{}`).
    - Key-value pairs are separated by colons (`:`).
    - Entries are separated by commas.
    - Example: `bar(){1: 2, 3: 4}`.

 Nodes:
  Dict mode allows keys and values to be any defined data type.
  List mode elements can also be any defined data type.
  Single line comments are supported, and begin with `#`

--------------------------------------------------------------
*/
#define PROTO_LIST_SEGMENT_SIZE 64
#include <packet_node.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constants.h"
#define MAX_PROTO_OBJ_SIZE MAX_STRING_CONST_SIZE


enum ProtoNodeType { PNT_UNKNOWN = 0, PNT_num, PNT_str, PNT_obj };
enum NumberVariety { NV_INVALID, NV_INT, NV_FLOAT };

struct ProtoList {
    struct ProtoNode *contents[PROTO_LIST_SEGMENT_SIZE];

    // Null unless size > PROTO_LIST_SEGMENT_SIZE
    struct ProtoList *next;
};
struct ProtoDict {
    struct ProtoNode *keys[PROTO_LIST_SEGMENT_SIZE];
    struct ProtoNode *values[PROTO_LIST_SEGMENT_SIZE];

    // Null unless size > PROTO_LIST_SEGMENT_SIZE
    struct ProtoDict *next;
};

struct ProtoObject {
    char name[MAX_PROTO_OBJ_SIZE];
    uint64_t name_hash;

    struct ProtoList *arguments; // Never null

    struct ProtoList *attached_list; // Null if not set
    struct ProtoDict *attached_dict; // Null if not set
};

// A proto node can NEVER contain Lists or Dicts, as these cannot be on their
// own
struct ProtoNode {
    enum ProtoNodeType type;
    union {
        char raw_data[512]; // Used in num and str
        struct ProtoObject object; // Used in PNT_obj
    };
};

struct ProtoList *parse_proto_file(const char *str);
void free_proto_node(struct ProtoNode *node);
void free_proto_list(struct ProtoList *list);
void free_proto_dict(struct ProtoDict *dict);

void debug_print_proto_dict(const struct ProtoDict *dict, int level);
void debug_print_proto_list(const struct ProtoList *list, int level);
void debug_print_proto_node(struct ProtoNode *node, int level);

// Takes a string, handles escapes, makes a new unescaped string.
// You take ownership of newly created string
char *unescape_string(const char *str);
struct ResultingNumber {
    bool is_float;
    union {
        double d;
        long long ll;
    };
};
struct ResultingNumber proto_node_number(struct ProtoNode *node);

static __always_inline void assert_proto_node_type(struct ProtoNode *node, enum ProtoNodeType type) {
    if (node->type != type) {
        fprintf(stderr, "Proto element type needed: %d != actual type %d\n", node->type, type);
        exit(2);
    }
}
static __always_inline void assert_proto_node_object_name(struct ProtoNode *node, char *name) {
    if (node->type != PNT_obj) {
        fprintf(stderr, "Needed a proto node object of name \"%s\", got type %d\n", name, node->type);
        exit(2);
    }
    if (0 != strcmp(node->object.name, name)) {
        fprintf(stderr, "Needed a proto node object of name \"%s\", got \"%s\"\n", name, node->object.name);
        exit(2);
    }
}
static __always_inline struct ProtoNode *get_argument_of_type(struct ProtoNode *node, int index, enum ProtoNodeType type) {
    assert_proto_node_type(node, PNT_obj);
    struct ProtoList *arguments = node->object.arguments;
    while (index > PROTO_LIST_SEGMENT_SIZE) {
        arguments = arguments->next;
        if (arguments == NULL) {
            debug_print_proto_node(node, 0);
            fprintf(stderr, "Error: Proto node does not have enough arguments\n");
            exit(2);
        }
        index -= PROTO_LIST_SEGMENT_SIZE;
    }
    struct ProtoNode *arg = arguments->contents[index];
    if (arg == NULL) {
        debug_print_proto_node(node, 0);
        fprintf(stderr, "Error: Proto node of name \"%s\" does not have enough arguments\n", node->object.name);
        exit(2);
    }
    if (arg->type != type) {
        debug_print_proto_node(node, 0);
        fprintf(stderr, "Error: Proto node of name \"%s\" has incorrect type at index %d, needed: %d, got: %d \n", node->object.name, index, type, arg->type);
        exit(2);
    }
    return arg;
}

// Callback used for proto_list_foreach.
// Param 1: Current element
// Param 2: Any state you wish to keep during iteration
// Return: 0 for nothing, 1 to break out of loop
typedef char (*ListCallback)(struct ProtoNode *, void **);
// Loop over each element of list. Callback is called per element
// State is passed to callback
// Return: 1 if loop has been broken out of
char proto_list_foreach(struct ProtoList *list, ListCallback callback, void **state);

typedef char (*DictCallback)(struct ProtoNode *, struct ProtoNode *, void **);
char proto_dict_foreach(struct ProtoDict *dict, DictCallback callback, void **state);
