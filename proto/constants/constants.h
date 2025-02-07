// Provides a way to precompute constants of quick string comparison where it matters

// This is NOT intended for users to redefine!
// only change this value here!
#ifndef MAX_STRING_CONST_SIZE
#define MAX_STRING_CONST_SIZE 64
#endif

#include "_constants.h"

STRING_CONSTANT(foo_c, "bar")


#define GENERATE_CONSTANTS
#include "_constants.h"