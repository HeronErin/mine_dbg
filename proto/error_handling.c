#include "error_handling.h"
#include <stdio.h>

__thread struct GlobalErrorState* global_error_state = NULL;


void exit_on_error(){
	if (!global_error_state) return;
	fprintf(stderr, "An unrecoverable error has occured (code: %d) in file %s on line %d:\n%s\n",
		global_error_state->type,
		global_error_state->file_name,
		global_error_state->line,
		global_error_state->message
	);
	exit(-1);

}