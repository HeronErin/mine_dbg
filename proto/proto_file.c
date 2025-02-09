#include "proto_file.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xxhash.h"

// Always *should* be set to the current string
// being parsed, as such more detailed error info
// can be derived.
static const char *ERROR_STRING = NULL;

__attribute__((noreturn)) static void parsing_error(const char *error_loc, const char *error) {
    // If somebody(me) forgot to set ERROR_STRING or error_loc is NULL
    if (error_loc < ERROR_STRING || ERROR_STRING == NULL || error_loc == NULL || strlen(ERROR_STRING) + ERROR_STRING < error_loc) {
        char error_ext[32] = {0};
        strncpy(error_ext, error, 32 - 1);

        fprintf(stderr, "\nAn error has occurred during proto file parsing.\nError details: %s\n Context: ```%s```", error, error_ext);
        exit(2);
    }

    // Now we know they overlap, we can generate betting error details
    const char *itr = ERROR_STRING;
    int linecount = 0;
    int columncount = 0;
    const char *lastline = ERROR_STRING;
    // This is safe as we know error_loc is inside of ERROR_STRING
    while (itr != error_loc) {
        if (*itr == '\n') {
            linecount++;
            columncount = 0;
            lastline = itr;
        } else
            columncount++;
        itr++;
    }
    while (*itr && *itr != '\n')
        itr++;


    fprintf(stderr, "\nFile parsing error: line %d, column %d\nError details: %s\nError context: ", linecount, columncount, error);
    if (lastline) {
        fwrite(lastline, error_loc - lastline, 1, stderr);
        fprintf(stderr, "\x1b[1;31m%c\x1b[0m", *error_loc);
        fwrite(error_loc + 1, itr - error_loc - 1, 1, stderr);
        fprintf(stderr, "\n");
    }


    exit(1);
}


static struct ProtoList *proto_list_parse(const char **input, char list_mode);

const char *skip_whitespace(const char *str) {
    while (*str == '#' || (*str && isspace(*str))) {
        // Comment skipping
        if (*str == '#')
            while (*str && *str != '\n')
                str++;
        else
            str++;
    }
    return str;
}

static __always_inline enum ProtoNodeType assess_proto_node_type(const char *str) {
    if (*str == '\"')
        return PNT_str;
    if (isdigit(*str) || *str == '-')
        return PNT_num;
    if (isalpha(*str))
        return PNT_obj;
    return PNT_UNKNOWN;
}
// No fact checking, just assume all valid
static const char *absorb_number(const char *str) {
    while (*str && (isdigit(*str) || isalnum(*str) || *str == '-' || *str == '.'))
        str++;
    return str;
}

static const char *absorb_name(const char *str) {
    while (*str && (isalpha(*str) || isdigit(*str) || *str == '_'))
        str++;
    return str;
}

static struct ProtoDict *proto_dict_parse(const char **input);


static struct ProtoNode *assess_and_parse_singular_object(const char **input) {
    const char *str = *input;
    enum ProtoNodeType type = assess_proto_node_type(str);
    if (type == PNT_UNKNOWN)
        parsing_error(*input, "Unable to parse data type");

    struct ProtoNode *ret = calloc(1, sizeof(struct ProtoNode));
    ret->type = type;

    if (type == PNT_num) {
        const char *after = absorb_number(str);

        size_t len = after - str;

        if (len >= sizeof(ret->raw_data))
            parsing_error(str, "Number too long");

        memcpy(ret->raw_data, str, len);
        ret->raw_data[len] = 0;

        str = after;
    } else if (ret->type == PNT_str) {
        bool is_escaped = false;

        str++;
        const char *initial = str;

        // Skip over all content until an unescaped double quote is found
        while (*str && (*str != '\"' || is_escaped)) {
            is_escaped = (*str == '\\') ? !is_escaped : false;
            str++;
        }
        size_t len = str - initial;

        // Skip over the trailing double quote
        str += !!*str;

        if (len >= sizeof(ret->raw_data))
            parsing_error(initial, "String too long");
        memcpy(ret->raw_data, initial, len);
    } else if (ret->type == PNT_obj) {
        const char *after_name = absorb_name(str);
        size_t len = after_name - str;
        if (len >= sizeof(ret->object.name))
            parsing_error(str, "Object name too long");
        memcpy(ret->object.name, str, len);
        ret->object.name[len] = 0;
        ret->object.name_hash = PN_str_hash(ret->object.name);

        str = skip_whitespace(after_name);

        while (*str == '(' || *str == '[' || *str == '{') {
            str++; // Skip opener
            char list_mode = 0;
            struct ProtoList **output = &ret->object.attached_list;
            switch (str[-1]) {
                case '(':
                    list_mode = 2;
                    output = &ret->object.arguments;

                case '[':
                    if (*output != NULL)
                        parsing_error(str, "Object cannot have two sets of arguments or lists");

                    *output = proto_list_parse(&str, list_mode);
                    break;
                case '{':
                    if (ret->object.attached_dict != NULL)
                        parsing_error(str, "Object cannot have two sets of attached dicts");

                    ret->object.attached_dict = proto_dict_parse(&str);
                default:;
            }
        }
        if (ret->object.arguments == NULL)
            parsing_error(str, "Object must have arguments after them, although this argument list maybe empty");
    }
    *input = str;
    return ret;
}


// Assumes we are on the char past the opener, sets the input one after the closer
static struct ProtoDict *proto_dict_parse(const char **input) {
    int index = 0;
    struct ProtoDict *dict = calloc(1, sizeof(struct ProtoDict));
    struct ProtoDict *current_dict = dict;
    const char *str = skip_whitespace(*input);
    while (*str) {
        if (*str == '}')
            break;
        struct ProtoNode *key = assess_and_parse_singular_object(&str);
        str = skip_whitespace(str);
        if (*str != ':')
            parsing_error(str, "Syntax error: dict is lacking a colon to indicate a key-value pair");
        str++; // Skip colon

        str = skip_whitespace(str);
        struct ProtoNode *value = assess_and_parse_singular_object(&str);
        str = skip_whitespace(str);

        if (index >= PROTO_LIST_SEGMENT_SIZE) {
            current_dict->next = calloc(1, sizeof(struct ProtoDict));
            current_dict = current_dict->next;
            index %= PROTO_LIST_SEGMENT_SIZE;
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


char proto_list_foreach(struct ProtoList *list, ListCallback callback, void **state) {
    for (int i = 0; i < PROTO_LIST_SEGMENT_SIZE && list->contents[i]; i++) {
        if (callback(list->contents[i], state) == 1)
            return 1;
    }
    if (list->next)
        return proto_list_foreach(list->next, callback, state);
    return 0;
}
char proto_dict_foreach(struct ProtoDict *dict, DictCallback callback, void **state) {
    for (int i = 0; i < PROTO_LIST_SEGMENT_SIZE && dict->keys[i] && dict->values[i]; i++) {
        if (callback(dict->keys[i], dict->values[i], state) == 1)
            return 0;
    }
    if (dict->next)
        return proto_dict_foreach(dict->next, callback, state);
    return 0;
}

// list_mode of 0 is square bracket mode
// list_mode of 1 is root mode (null terminated input)
// list_mode of 2 is function arg mode (parenthesis terminated)
//
// Assumes we are on the char past the opener, sets the input one after the closer
static struct ProtoList *proto_list_parse(const char **input, char list_mode) {
    int index = 0;
    struct ProtoList *list = calloc(1, sizeof(struct ProtoList));
    struct ProtoList *current_list = list;
    const char *str = skip_whitespace(*input);

    while (*str) {
        if (*str == ']') {
            if (list_mode != 0)
                parsing_error(str, "List open brackets != list close bracket count");
            break;
        } else if (*str == ')') {
            if (list_mode != 2)
                parsing_error(str, "List open parenthesis != list close parenthesis count");
            break;
        }
        struct ProtoNode *element = assess_and_parse_singular_object(&str);
        str = skip_whitespace(str);
        if (*str && (*str != ')' && *str != ']' && *str != ','))
            parsing_error(str, "Unknown separator found in list");

        // At this point if MUST be `,` '[' or '{'

        if (index >= PROTO_LIST_SEGMENT_SIZE) {
            current_list->next = calloc(1, sizeof(struct ProtoList));
            current_list = current_list->next;
            index %= PROTO_LIST_SEGMENT_SIZE;
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

struct ProtoList *parse_proto_file(const char *str) {
    ERROR_STRING = str;

    // All files by default are in list mode
    return proto_list_parse(&str, 1);
}


void free_proto_node(struct ProtoNode *node) {
    switch (node->type) {
        case PNT_num:
        case PNT_str:
            break;
        case PNT_obj:
            free_proto_list(node->object.arguments);
            if (node->object.attached_list)
                free_proto_list(node->object.attached_list);
            if (node->object.attached_dict)
                free_proto_dict(node->object.attached_dict);
            break;
        default:
            perror("Node type not supported\n");
            exit(2);
    }
    free(node);
}

void free_proto_list(struct ProtoList *list) {
    for (int i = 0; i < PROTO_LIST_SEGMENT_SIZE && list->contents[i]; i++) {
        free_proto_node(list->contents[i]);
    }
    if (list->next)
        free_proto_list(list->next);
    free(list);
}
void free_proto_dict(struct ProtoDict *dict) {
    for (int i = 0; i < PROTO_LIST_SEGMENT_SIZE && dict->keys[i]; i++) {
        free_proto_node(dict->keys[i]);
        free_proto_node(dict->values[i]);
    }
    if (dict->next)
        free_proto_dict(dict->next);
    free(dict);
}

static void _print_level(int level) {
    while (level--)
        printf("\t");
}

void debug_print_proto_node(struct ProtoNode *node, int level) {
    _print_level(level);
    switch (node->type) {
        case PNT_num:
        case PNT_str:
            printf("%s\n", node->raw_data);
            break;
        case PNT_obj:
            printf("%s:\n", node->object.name);

            _print_level(level + 1);
            printf("Args:\n");
            debug_print_proto_list(node->object.arguments, level + 2);


            if (node->object.attached_list) {
                _print_level(level + 1);
                printf("List:\n");
                debug_print_proto_list(node->object.attached_list, level + 2);
            }
            if (node->object.attached_dict) {
                _print_level(level + 1);
                printf("Dict:\n");
                debug_print_proto_dict(node->object.attached_dict, level + 2);
            }
            break;
        default:;
    }
}
void debug_print_proto_dict(const struct ProtoDict *dict, int level) {
    for (int i = 0; i < PROTO_LIST_SEGMENT_SIZE && dict->keys[i]; i++) {
        _print_level(level);
        printf("Key:\n");
        debug_print_proto_node(dict->keys[i], level + 1);
        _print_level(level);
        printf("Value:\n");
        debug_print_proto_node(dict->values[i], level + 1);
    }
    if (dict->next)
        debug_print_proto_dict(dict->next, level);
}
void debug_print_proto_list(const struct ProtoList *list, int level) {
    for (int i = 0; i < PROTO_LIST_SEGMENT_SIZE && list->contents[i]; i++) {
        debug_print_proto_node(list->contents[i], level);
    }

    if (list->next)
        debug_print_proto_list(list->next, level);
}


char *unescape_string(const char *in) {
    char *out = calloc(strlen(in) + 1, sizeof(char));
    char *out_ptr = out;
    while (*in) {
        if (*in != '\\') {
            *(out_ptr++) = *(in++);
            continue;
        }

        in++;
        switch (*in) {
            case 'a':
                *(out_ptr++) = '\a';
                break;
            case 'b':
                *(out_ptr++) = '\b';
                break;
            case 'f':
                *(out_ptr++) = '\f';
                break;
            case 'n':
                *(out_ptr++) = '\n';
                break;
            case 'r':
                *(out_ptr++) = '\r';
                break;
            case 't':
                *(out_ptr++) = '\t';
                break;
            case 'v':
                *(out_ptr++) = '\v';
                break;
            case '\\':
                *(out_ptr++) = '\\';
                break;
            case '\'':
                *(out_ptr++) = '\'';
                break;
            case '"':
                *(out_ptr++) = '"';
                break;
            case 'X':
            case 'x':
                char high = tolower(*(++in));
                char low = tolower(*(++in));
                assert(('0' <= high && high <= '9') || ('a' <= high && high <= 'f'));
                assert(('0' <= low && low <= '9') || ('a' <= low && low <= 'f'));

                char out = low <= '9' ? low - '0' : low - 'a' + 0xa;
                out |= (high <= '9' ? high - '0' : high - 'a' + 0xa) << 4;
                *(out_ptr++) = out;
                break;
            default:
                *(out_ptr++) = '\\';
                *(out_ptr++) = *in;
        }
        in++;
    }
    return out;
}

struct ResultingNumber proto_node_number(struct ProtoNode *node) {
    assert(node->type == PNT_num);

    char *number = node->raw_data;
    bool is_negative = *number == '-';
    number += is_negative;
    if (!*number)
        goto INVALID;


    bool is_decimal = false;
    for (char *itr = number; *itr; itr++)
        if (*itr == '.') {
            is_decimal = true;
            break;
        }
    if (is_decimal) {
        double d = strtod(number, NULL);
        if (errno != 0)
            goto INVALID;
        return (struct ResultingNumber) {
                .is_float = true,
                .d = is_negative ? -d : d,
        };
    }


    int base = 10;
    if (number[0] == '0' && (number[1] == 'x' || number[1] == 'X')) {
        number += 2;
        if (!*number)
            goto INVALID;
        base = 16;
    } else if (number[0] == '0' && number[1] == 'b') {
        number += 2;
        if (!*number)
            goto INVALID;
        base = 2;
    }
    errno = 0;
    long long result = strtoll(number, NULL, base);
    if (errno != 0)
        goto INVALID;

    return (struct ResultingNumber) {
            .is_float = false,
            .ll = is_negative ? -result : result,
    };
INVALID:
    fprintf(stderr, "Cannot parse number: %s\n", node->raw_data);
    exit(1);
}
