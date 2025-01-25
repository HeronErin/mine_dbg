#pragma once

#include <stddef.h>
#include <stdlib.h>


#define DEFAULT_SEGMENT_ALLOC 1024

struct EncodeDataSegment {
    // Most of the time DEFAULT_SEGMENT_ALLOC
    // but can be larger
    size_t alloc;
    size_t size;

    struct EncodeDataSegment *next;

    char data[];
};
struct CombinedDataSegment {
    size_t size;
    char data[];
};
void writeBulkDataToBuffer(struct EncodeDataSegment **head, const void *data, size_t size);
void writeByteToBuffer(struct EncodeDataSegment **head_, char byte);

static inline struct EncodeDataSegment *makeEncodeDataSegmentRoot() {
    struct EncodeDataSegment *root = malloc(sizeof(struct EncodeDataSegment) + DEFAULT_SEGMENT_ALLOC);
    root->alloc = DEFAULT_SEGMENT_ALLOC;
    root->size = 0;
    root->next = NULL;

    return root;
}
static inline void freeEncodeDataSegment(struct EncodeDataSegment *root) {
    while (root) {
        struct EncodeDataSegment *next = root->next;
        free(root);
        root = next;
    }
}


struct CombinedDataSegment *combineSegments(struct EncodeDataSegment *root);


unsigned long readVarStyle(char **buffer, char *maxBuffer, char maxBits);
void writeVarStyle(struct EncodeDataSegment **head_, unsigned long value);
