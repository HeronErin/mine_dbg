#include <stdio.h>

#include "xxhash.h"
#include "string.h"
#include <stdlib.h>


#define DO_NOT_MAP_BIN
#include "constants.h"


void write_hash(FILE *file, const char *input) {
    int len = strlen(input);
    if (len > MAX_STRING_CONST_SIZE) {
        fprintf(stderr, "String constant too large for hash! Please increase MAX_STRING_CONST_SIZE: \"%s\"\n", input);
        exit(-1);
    }
    char temp[MAX_STRING_CONST_SIZE] = {0};
    memcpy(temp, input, len);
    uint64_t value =  XXH64(temp, MAX_STRING_CONST_SIZE, 0);
    fwrite(&value, sizeof(uint64_t), 1, file);

}

int main() {
    FILE* file = fopen("constants.bin", "w");

    #undef STRING_CONSTANT
    #define STRING_CONSTANT(name, value) write_hash(file, value);
    #include "constants.h"



    fclose(file);

}