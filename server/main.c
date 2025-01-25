#include <stdio.h>
#include <stdlib.h>
#include "proto_file.h"

#include "datatypes.h"
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
    struct EncodeDataSegment *root = makeEncodeDataSegmentRoot();
    struct EncodeDataSegment *head = root;


    int i = -2147483648;
    writeVarStyle(&head, *(unsigned int *) &i);

    struct CombinedDataSegment *dat = combineSegments(root);

    fwrite(dat->data, dat->size, 1, stdout);

    freeEncodeDataSegment(root);
    free(buffer);
    free(dat);
    return 0;
}
