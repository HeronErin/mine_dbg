#include "error_handling.h"
#include <stdio.h>

__thread struct GlobalErrorState* global_error_state = NULL;