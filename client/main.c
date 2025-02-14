#include <serde.h>
#include <stdio.h>
#include <stdlib.h>
#include "proto_file.h"

#include "datatypes.h"


#include "assert.h"
#include "error_handling.h"


#include "constants/constants.h"
#include "packet_node.h"

int main() {
    FILE *fp = fopen( "/home/mitchell/Documents/mc_dbg/proto/packets/1.21.4.proto", "r" );

    fseek( fp, 0, SEEK_END );
    size_t len = ftell( fp );
    fseek( fp, 0, SEEK_SET );

    char *buffer = malloc( len + 1 );
    fread( buffer, len, 1, fp );
    buffer[len] = '\0';
    fclose( fp );

    // printf("%s\n", buffer);

    create_version_serde( buffer );


    //    printf(": %llu\n", *OBJ_version_info);
    //    printf(": %llu\n", PN_str_hash(OBJ_version_info_str));
    return 0;
}
