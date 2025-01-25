#pragma once

#include <stddef.h>


typedef char (*EncodeFunc)(void** buffer, size_t* buffer_size, void* data);


// This is only to be defined in constants.
// And lays you how serde will handle integral datatypes.
struct DatatypePrimative{
    size_t size;
    const char* data_type_name;
    const EncodeFunc encode_into_buffer;
};




