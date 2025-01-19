#include "proto_file.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct ProtoList* proto_list_parse(const char** input, char list_mode);

const char* skip_whitespace(const char* str) {
    while (*str == '#' || (*str && isspace(*str))) {
        // Comment skipping
        if (*str == '#')
            while (*str && *str != '\n') str++;
        else
            str++;
    }
    return str;
}

static __always_inline enum ProtoNodeType assess_proto_node_type(const char* str) {
    if (*str == '\"') return PNT_str;
    if (isdigit(*str)) return PNT_num;
    if (isalpha(*str)) return PNT_obj;
    return PNT_UNKNOWN;
}
// No fact checking, just assume all valid
static const char* absorb_number(const char* str) {
    while (*str && (isdigit(*str) || isalnum(*str))) str++;
    return str;
}

static const char* absorb_name(const char* str) {
    while (*str && (isalpha(*str) || isdigit(*str) || *str == '_')) str++;
    return str;
}

static struct ProtoDict* proto_dict_parse(const char** input);


struct ProtoNode* assess_and_parse_singular_object(const char** input) {
    const char* str = *input;
    enum ProtoNodeType type = assess_proto_node_type(str);
    if (type == PNT_UNKNOWN) {
        perror("Unknown node type\n");
        exit(1);
    }

    struct ProtoNode* ret = calloc(1, sizeof(struct ProtoNode));
    ret->type = type;

    if (type == PNT_num) {
        const char* after = absorb_number(str);

        size_t len = after - str;

        if (len >= sizeof(ret->raw_data)) {
            perror("Number too long\n");
            exit(1);
        }
        memcpy(ret->raw_data, str, len);
        ret->raw_data[len] = 0;

        str = after;
    }
    else if (ret->type == PNT_str) {
        bool is_escaped = false;

        str++;
        const char* initial = str;

        // Skip over all content until an unescaped double quote is found
        while (*str && (*str != '\"' || is_escaped)) {
            is_escaped = (*str == '\\') ? !is_escaped : false;
            str++;
        }
        size_t len = str - initial;

        // Skip over the trailing double quote
        str += !!*str;

        if (len >= sizeof(ret->raw_data)) {
            perror("String too long\n");
            exit(1);
        }
        memcpy(ret->raw_data, initial, len);
    }
    else if (ret->type == PNT_obj) {
        const char* after_name = absorb_name(str);
        size_t len = after_name - str;
        if (len >= sizeof(ret->object.name)) {
            perror("Object name too long\n");
            exit(1);
        }
        memcpy(ret->object.name, str, len);
        ret->object.name[len] = 0;

        str = skip_whitespace(after_name);

        while (*str == '(' || *str == '[' || *str == '{') {
            str++; // Skip opener
            char list_mode = 0;
            struct ProtoList** output = &ret->object.attached_list;
            switch (str[-1]) {
                case '(':
                    list_mode = 2;
                    output = &ret->object.arguments;

                case '[':
                    if (*output != NULL) {
                        perror("Object cannot have two sets of arguments or lists\n");
                        exit(1);
                    }

                    *output = proto_list_parse(&str, list_mode);
                    break;
                case '{':
                    if (ret->object.attached_dict != NULL) {
                        perror("Object cannot have two sets of attached dicts\n");
                        exit(1);
                    }
                    ret->object.attached_dict = proto_dict_parse(&str);
                default: ;
            }
        }
        if (ret->object.arguments == NULL) {
            perror("Object must have arguments after them, although this argument list maybe empty.\n");
            exit(1);
        }
    }
    *input = str;
    return ret;
}


// Assumes we are on the char past the opener, sets the input one after the closer
static struct ProtoDict* proto_dict_parse(const char** input) {
    int index = 0;
    struct ProtoDict* dict = calloc(1, sizeof(struct ProtoDict));
    struct ProtoDict* current_dict = dict;
    const char* str = skip_whitespace(*input);
    while (*str) {
        if (*str == '}') break;
        struct ProtoNode* key = assess_and_parse_singular_object(&str);
        str = skip_whitespace(str);
        if (*str != ':') {
            perror("Syntax error: dict is lacking a colon to indicate a key-value pair\n");
            exit(1);
        }
        str++; // Skip colon

        str = skip_whitespace(str);
        struct ProtoNode* value = assess_and_parse_singular_object(&str);
        str = skip_whitespace(str);

        if (index >= 64) {
            current_dict->next = calloc(1, sizeof(struct ProtoDict));
            current_dict = current_dict->next;
            index %= 64;
        }

        current_dict->keys[index] = key;
        current_dict->values[index] = value;
        index++;


        str += *str == ','; // Skip comma if present
        str = skip_whitespace(str);
    }


    // Add one if not null
    *input = str + !!*str;
    return dict;
}


// list_mode of 0 is square bracket mode
// list_mode of 1 is root mode (null terminated input)
// list_mode of 2 is function arg mode (parenthesis terminated)
//
// Assumes we are on the char past the opener, sets the input one after the closer
static struct ProtoList* proto_list_parse(const char** input, char list_mode) {
    int index = 0;
    struct ProtoList* list = calloc(1, sizeof(struct ProtoList));
    struct ProtoList* current_list = list;
    const char* str = skip_whitespace(*input);

    while (*str) {
        if (*str == ']') {
            if (list_mode == 0) break;

            perror("List open brackets != list close bracket count\n");
            exit(1);
        }
        else if (*str == ')') {
            if (list_mode == 2) break;

            perror("List open parenthesis != list close parenthesis count\n");
            exit(1);
        }
        struct ProtoNode* element = assess_and_parse_singular_object(&str);
        str = skip_whitespace(str);
        if (*str && (*str != ')' && *str != ']' && *str != ',')) {
            // debug_print_proto_node(element, 1);
            fprintf(stderr, "Unknown separator found in list: %c\n", *str);
            exit(1);
        }
        // At this point if MUST be `,` '[' or '{'

        if (index >= 64) {
            current_list->next = calloc(1, sizeof(struct ProtoList));
            current_list = current_list->next;
            index %= 64;
        }
        current_list->contents[index] = element;
        index++;


        str += *str == ',';
        str = skip_whitespace(str);
    }
    // Add one if not null
    *input = str + !!*str;
    return list;
}

struct ProtoList* parse_proto_file(const char* str) {
    // All files by default are in list mode
    return proto_list_parse(&str, 1);
}


void free_proto_node(struct ProtoNode* node) {
    switch (node->type) {
        case PNT_num:
        case PNT_str:
            break;
        case PNT_obj:
            free_proto_list(node->object.arguments);
            if (node->object.attached_list) free_proto_list(node->object.attached_list);
            if (node->object.attached_dict) free_proto_dict(node->object.attached_dict);
            break;
        default:
            perror("Node type not supported\n");
            exit(2);
    }
    free(node);
}

void free_proto_list(struct ProtoList* list) {
    for (int i = 0; i < 64 && list->contents[i]; i++) {
        free_proto_node(list->contents[i]);
    }
    if (list->next) free_proto_list(list->next);
    free(list);
}
void free_proto_dict(struct ProtoDict* dict) {
    for (int i = 0; i < 64 && dict->keys[i]; i++) {
        free_proto_node(dict->keys[i]);
        free_proto_node(dict->values[i]);
    }
    if (dict->next) free_proto_dict(dict->next);
    free(dict);
}

static void _print_level(int level) {
    while (level--)
        printf("\t");
}

void debug_print_proto_node(struct ProtoNode* node, int level) {
    _print_level(level);
    switch (node->type) {
        case PNT_num:
        case PNT_str:
            printf("%s\n", node->raw_data);
            break;
        case PNT_obj:
            printf("%s:\n", node->object.name);

            _print_level(level+1);
            printf("Args:\n");
            debug_print_proto_list(node->object.arguments, level+2);


            if (node->object.attached_list) {
                _print_level(level+1);
                printf("List:\n");
                debug_print_proto_list(node->object.attached_list, level+2);
            }
            if (node->object.attached_dict) {
                _print_level(level+1);
                printf("Dict:\n");
                debug_print_proto_dict(node->object.attached_dict, level+2);
            }
            break;
        default: ;
    }
}
void debug_print_proto_dict(struct ProtoDict* dict, int level) {
    for (int i = 0; i < 64 && dict->keys[i]; i++) {
        _print_level(level);
        printf("Key:\n");
        debug_print_proto_node(dict->keys[i], level+1);
        _print_level(level);
        printf("Value:\n");
        debug_print_proto_node(dict->values[i], level+1);
    }
    if (dict->next) debug_print_proto_dict(dict->next, level);
}
void debug_print_proto_list(struct ProtoList* list, int level) {
    for (int i = 0; i < 64 && list->contents[i]; i++) {
        debug_print_proto_node(list->contents[i], level);
    }
    if (list->next) debug_print_proto_list(list->next, level);
}
