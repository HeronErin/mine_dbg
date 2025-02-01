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

    // printf("%llu\n", H_KEY("FOO"));

    PacketNode *root = PN_new_bundle();
    PacketNode *x = PN_from_byte('Q');
    PN_rename(x, "x");
    // PN_rename(x, "X");
    PN_bundle_set_element(root, x);
    printf("%d\n", x == PN_bundle_get_element_hash(root, PN_str_hash("x")));


    return 0;
}
