#include <stdio.h>
#include <stdlib.h>
#include "proto_file.h"

#include "datatypes.h"


#include "error_handling.h"
#include "assert.h"


#include "constants/constants.h"
#include "packet_node.h"

int main() {
    FILE *fp = fopen("/home/mitchell/Documents/mc_dbg/proto/packets/1.21.4.proto", "r");

	assert(fp != NULL);
    printf(": %llu\n", *foo_c);
    printf(": %llu\n", PN_str_hash(foo_c_str));
    return 0;
}
