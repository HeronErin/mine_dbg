// See constants/ for more info
// This file just adds all the string values

// Don't make _constants.h do anything
#define GENERATE_CONSTANTS
#define HAS_GENERATED_CONSTANTS


// Just define the variables
#define STRING_CONSTANT(name, value) const char *name##_str = value;
#include "constants.h"
