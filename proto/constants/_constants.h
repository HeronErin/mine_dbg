#include <stdint.h>


//  During the initial pass nothing will be done
#ifndef GENERATE_CONSTANTS

// Act as a nop
#define STRING_CONSTANT(name, value)

#else
#ifndef HAS_GENERATED_CONSTANTS
// Prevent recurisive imports
#define HAS_GENERATED_CONSTANTS


#undef STRING_CONSTANT
#define STRING_CONSTANT(name, value) extern const char* name##_str;
#include "constants.h"

#include "generated_constants.h"

#endif
#endif