#include "datatypes.h"
#include <endian.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void writeBulkDataToBuffer(struct EncodeDataSegment** head_, const void* data, size_t size){
    struct EncodeDataSegment* head = *head_;
    if (head->size + size > head->alloc){
        size_t alloc = DEFAULT_SEGMENT_ALLOC > size ? DEFAULT_SEGMENT_ALLOC : size;
        head->next = malloc(sizeof(struct EncodeDataSegment) + alloc);
        *head_ = head->next;
        head = *head_;

        head->alloc = alloc;
        head->size = 0;
        head->next = NULL;
    }
    assert(head->next == NULL);

    memcpy(head->data + head->size, data, size);
    head->size += size;
}
void writeByteToBuffer(struct EncodeDataSegment** head_, char byte){
    struct EncodeDataSegment* head = *head_;
    if (head->size + 1 > head->alloc){
        head->next = malloc(sizeof(struct EncodeDataSegment) + DEFAULT_SEGMENT_ALLOC);
        *head_ = head->next;
        head = *head_;

        head->alloc = DEFAULT_SEGMENT_ALLOC;
        head->size = 0;
        head->next = NULL;
    }
    assert(head->next == NULL);
    head->data[head->size] = byte;
    head->size += 1;
}
struct CombinedDataSegment* combineSegments(struct EncodeDataSegment* root){
    assert(root != NULL);

    struct EncodeDataSegment* head = root;
    size_t size = 0;

    while (head != NULL){
        size += head->size;
        head = head->next;
    }
    struct CombinedDataSegment* combined = malloc(sizeof(struct CombinedDataSegment) + size);
    combined->size = size;
    head = root;
    size = 0;
    while (head != NULL){
        memcpy(combined->data + size, head->data, head->size);
        size += head->size;
        head = head->next;
    }

    return combined;
}





// Adapted from https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Protocol#VarInt_and_VarLong
static char SEGMENT_BITS = 0x7F;
static char CONTINUE_BIT = 0x80;

// Reads an abrbitray sized varint from a buffer.
// Sets errno on error. ENOMEM for out of bounds read, EINVAL for going past max bits
unsigned long readVarStyle(char** buffer_, char* maxBuffer, char maxBits){
    unsigned long value = 0;
    int position = 0;

    char* buffer = *buffer_;

    while (1) {
        char current = *(buffer++);
        value |= (current & SEGMENT_BITS) << position;

        if ((current & CONTINUE_BIT) == 0) break;
        if (buffer >= maxBuffer){
            errno = ENOMEM;
            return -1;
        }

        position += 7;

        if (position >= maxBits){
            errno = EINVAL;
            return -1;
        }
    }
    *buffer_ = buffer;
    return value;
}
void writeVarStyle(struct EncodeDataSegment** head_, unsigned long value) {
    while (1) {
        if ((value & ~SEGMENT_BITS) == 0) {
            writeByteToBuffer(head_, value);
            return;
        }
        writeByteToBuffer(head_, (value & SEGMENT_BITS) | CONTINUE_BIT);
        value >>= 7;
    }
}

