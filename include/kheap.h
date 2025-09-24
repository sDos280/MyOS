#ifndef KHEAP_H
#define KHEAP_H

#include "types.h"

/*#define KHEAP_START         0xC0000000
#define KHEAP_INITIAL_SIZE  0x100000

 typedef struct heap_chunk_struct {
    heap_chunk_t * previous;
    heap_chunk_t * next;
    size_t size; // the size of the user data (not including the header)
    uint8_t is_used; // 0 - not used, 1 - used
} heap_chunk_t; */

uint32_t alloc_unfreable_phys(size_t size, uint8_t align); // allocate a non freable type of memory, 0 - not align, 1 - align

#endif