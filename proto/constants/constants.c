#define NO_HASH_CONSTANTS
#include "constants.h"
#include "assert.h"
#include <stddef.h>
extern const uint64_t constant_hash_blob_start[];
extern const uint64_t constant_hash_blob_end[];

__asm__(
    ".section .data\n"
    "constant_hash_blob_start:\n"
    ".incbin \"constants.bin\"\n"
    "constant_hash_blob_end:\n"
);

#undef STRING_CONSTANT
#define STRING_CONSTANT(name, value)                                         \
    const uint64_t* name = constant_hash_blob_start + __CONST_OFFSET_##name; \
    const char* name##_str = value;

#include "constants.h"
