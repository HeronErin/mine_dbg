#include <stdio.h>

#include <stdlib.h>
#include "string.h"
#include "xxhash.h"


#define HAS_GENERATED_CONSTANTS
#include "constants.h"


void write_hash(FILE *file, const char *name, const char *input) {
    int len = strlen(input);
    if (len > MAX_STRING_CONST_SIZE) {
        fprintf(stderr, "String constant too large for hash! Please increase MAX_STRING_CONST_SIZE: \"%s\"\n", input);
        exit(-1);
    }

    char temp[MAX_STRING_CONST_SIZE] = {0};
    memcpy(temp, input, len);
    uint64_t value = XXH64(temp, MAX_STRING_CONST_SIZE, 0);
    //    fwrite(&value, sizeof(uint64_t), 1, file);

    fprintf(file, "#define %s %luull\n", name, value);
}

int main() {
    FILE *file = fopen("generated_constants.h", "w");
    fprintf(file, "#pragma once\n"
                  " // DO NOT EDIT!!!\n // This file is automatically generated by constant_gen.c\n\n");
#undef STRING_CONSTANT
#define STRING_CONSTANT(name, value) write_hash(file, #name, value);
#include "constants.h"


    fclose(file);
}
