#include <stdio.h>
#include <stdlib.h>
#include "proto_file.h"

#include "datatypes.h"

#include "PacketNode.h"


int main() {
    FILE *fp = fopen("/home/mitchell/Documents/mc_dbg/proto/packets/1.21.4.proto", "r");

    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = malloc(len + 1);
    fread(buffer, len, 1, fp);
    buffer[len] = '\0';
    fclose(fp);
    // PacketSerde_from_proto(buffer);
    free(buffer);

    printf("%llu\n", H_KEY("FOO"));

    PacketNode *boo = PN_from_boolean(39);
    PN_rename(boo, "boo");
    printf(": %d\n", PN_get_boolean(boo));
    printf(": %s\n", boo->name);
    PN_set_boolean(boo, 1);
    printf(": %d\n", PN_get_boolean(boo));

    return 0;
}
