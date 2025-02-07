#include <stdint.h>


//  During the initial pass nothing will be done
#ifndef GENERATE_CONSTANTS

// Act as a nop
#define STRING_CONSTANT(name, value)

#else
#ifndef HAS_GENERATED_CONSTANTS
// Prevent recurisive imports
#define HAS_GENERATED_CONSTANTS



// Keep track of the offsets used for the hashes index into the bin
enum __constant_offsets{

#undef STRING_CONSTANT
#define STRING_CONSTANT(name, value) __CONST_OFFSET_##name,
#include "constants.h"

};

#ifndef NO_HASH_CONSTANTS

#undef STRING_CONSTANT

// User-facing variables, see constants.bin
#define STRING_CONSTANT(name, value) \
    extern const uint64_t* name;\
    extern const char* name##_str;

#include "constants.h"

#endif
#endif
#endif