#pragma once
/*
  .proto files are my own format for handling the ever changing minecraft
  protocol. It is a recursive format for storing data. As it is made to
  be simple, I will just keep to specification contained in this header.
 
 Data Types in .proto Files
  1. Numbers:
     - Supported formats: decimal, hexadecimal, and binary.
       - Hexadecimal: prefixed with `0x`.
       - Binary: prefixed with `0b`.
       - Decimal: the default base-10 format, supports integer and floating-point values.
  
  2. Strings:
     - Enclosed in double quotation marks (`""`).
     - Supports C-style escape sequences but does not include more complex features.
  
  3. Objects:
     - The primary and most common data type, resembling function calls.
       - Example: `Ushort("port")`.
     - Arguments can be of two types:
       - Function-style arguments: 
         Implementation-defined, passed within parentheses.
       - Composite arguments: 
         Can use either List or Dict modes. 
 
 Composite Argument Modes:
  - List Mode:
    - Enclosed in square brackets (`[]`).
    - Elements are separated by commas.
    - Example: `foo()[1, 2, 3]`.
  
  - Dict Mode:
    - Enclosed in curly brackets (`{}`).
    - Key-value pairs are separated by colons (`:`).
    - Entries are separated by commas.
    - Example: `bar(){1: 2, 3: 4}`.
 
 Nodes:
  Dict mode allows keys and values to be any defined data type.
  List mode elements can also be any defined data type.
  Single line comments are supported, and begin with `#`
*/

enum ProtoNodeType{
	PNT_UNKNOWN = 0,
	PNT_num,
	PNT_str,
	PNT_obj
};
struct ProtoList{
	struct ProtoNode* contents[64];
	
	// Null unless size > 64
	struct ProtoList* next;
};
struct ProtoDict{
	struct ProtoNode* keys[64];
	struct ProtoNode* values[64];

	// Null unless size > 64
	struct ProtoDict* next;
};


struct ProtoObject{
	char name[64];
	struct ProtoList* arguments; // Never null
	
	struct ProtoList* attached_list; // Null if not set
	struct ProtoDict* attached_dict; // Null if not set
};

// A proto node can NEVER contain Lists or Dicts, as these cannot be on their own
struct ProtoNode{
	enum ProtoNodeType type;
	union{
		char raw_data[512]; // Used in num and str
		struct ProtoObject object; // Used in PNT_obj
	};
};



struct ProtoList* parse_proto_file(const char* str);
struct ProtoNode* assess_and_parse_singular_object(const char** input);
void free_proto_node(struct ProtoNode* node);
void free_proto_list(struct ProtoList* list);
void free_proto_dict(struct ProtoDict* dict);

void debug_print_proto_dict(struct ProtoDict* list, int level);
void debug_print_proto_list(struct ProtoList* list, int level);
void debug_print_proto_node(struct ProtoNode* node, int level);