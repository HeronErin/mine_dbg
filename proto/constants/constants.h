// Provides a way to precompute constants of quick string comparison where it matters
//
// Usage of constants.h:
// For each STRING_CONSTANT you see bellow it defines the following
//   NAME = a uint64 of the xxhash of the value
//   NAME_str = the value



#ifndef CONSTANTS_H
#define MAX_STRING_CONST_SIZE 64

#define CONSTANTS_H
#endif


#include "_constants.h"

// Proto file object names
STRING_CONSTANT(OBJ_version_info, "version_info")
STRING_CONSTANT(OBJ_namespace, "namespace")
STRING_CONSTANT(OBJ_packet, "packet")
STRING_CONSTANT(OBJ_enum, "enum")
STRING_CONSTANT(OBJ_enums, "enums")
// Proto file datatype object names
STRING_CONSTANT(OBJ_varint, "varint")
STRING_CONSTANT(OBJ_varlong, "varlong")
STRING_CONSTANT(OBJ_varint_enum, "varint_enum")

STRING_CONSTANT(OBJ_string, "string")

STRING_CONSTANT(OBJ_byte, "byte")
STRING_CONSTANT(OBJ_Ubyte, "Ubyte")

STRING_CONSTANT(OBJ_short, "short")
STRING_CONSTANT(OBJ_Ushort, "Ushort")

STRING_CONSTANT(OBJ_int, "int")
STRING_CONSTANT(OBJ_Uint, "Uint")

STRING_CONSTANT(OBJ_Ulong, "Ulong")
STRING_CONSTANT(OBJ_long, "long")
STRING_CONSTANT(OBJ_boolean, "boolean")

STRING_CONSTANT(OBJ_uuid, "uuid")
STRING_CONSTANT(OBJ_prefixed_byte_array, "prefixed_byte_array")
STRING_CONSTANT(OBJ_prefixed_array, "prefixed_array")
STRING_CONSTANT(OBJ_prefixed_optional, "prefixed_optional")
STRING_CONSTANT(OBJ_byte_array, "byte_array")
STRING_CONSTANT(OBJ_CONTEXT, "CONTEXT")
STRING_CONSTANT(OBJ_REMAINING_BYTES, "REMAINING_BYTES")



#define GENERATE_CONSTANTS
#include "_constants.h"