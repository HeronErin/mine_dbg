#include <stdio.h>
#include <stdlib.h>
#include "proto_file.h"

#include "serde.h"


int main() {
    FILE *fp = fopen("/home/mitchell/Documents/mc_dbg/proto/packets/1.21.4.proto", "r");

    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = malloc(len + 1);
    fread(buffer, len, 1, fp);
    buffer[len] = '\0';
    fclose(fp);
    PacketSerde_from_proto(buffer);

    // printf("%s\n", unescape_string("This is a test: \\X6a!"));


    free(buffer);
    return 0;
}
