#pragma once
#include <stdlib.h>

// Only set if something has gone severly wrong
extern __thread struct GlobalErrorState* global_error_state;


enum ErrorType {
    ERROR_TYPE_UNKNOWN = 0,
    ERROR_INVALID_PACKET,
    ERROR_INVALID_PACKET_FORMAT
};

#define RESET_ERROR_STATE() if (global_error_state) {free(global_error_state); global_error_state = NULL;}

#define SET_ERROR_STATE(ERR_TYPE, ERR_STR, ...) {\
    RESET_ERROR_STATE();\
    global_error_state = calloc(1, sizeof(struct GlobalErrorState));\
    global_error_state->type = ERR_TYPE;\
    global_error_state->file_name = __FILE__;\
    global_error_state->line = __LINE__;\
    snprintf(global_error_state->message, sizeof(global_error_state->message) - 1, ERR_STR, ##__VA_ARGS__);\
}


void exit_on_error();



struct GlobalErrorState{
    char message[1024];

    const char* file_name;
    int line;

    enum ErrorType type;
};