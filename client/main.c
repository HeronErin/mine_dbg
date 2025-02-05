#include <stdio.h>
#include <stdlib.h>
#include "proto_file.h"

#include "datatypes.h"

#include "PacketNode.h"
#include "error_handling.h"


int main() {
    FILE *fp = fopen("/home/mitchell/Documents/mc_dbg/proto/packets/1.21.4.proto", "r");
    SET_ERROR_STATE(ERROR_INVALID_PACKET, "%d");

    return 0;
}
