#include "proto_file.h"
#include <stdio.h>
#include <stdlib.h>


int main(){
	FILE *fp = fopen("/home/mitchell/Documents/mc_dbg/proto/packets/1.21.4.proto", "r");

	fseek(fp, 0, SEEK_END);
	size_t len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *buffer = malloc(len + 1);
	fread(buffer, len, 1, fp);
	buffer[len] = '\0';
	fclose(fp);


	struct ProtoList* list =  parse_proto_file(buffer);
	debug_print_proto_list(list, 0);
	free_proto_list(list);


	free(buffer);
	return 0;
}
