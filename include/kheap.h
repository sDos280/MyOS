#ifndef KHEAP_H
#define KHEAP_H

#include "types.h"

#define KHEAP_INITIAL_SIZE  (0x100000)

#define CHUNK_NOT_IN_US 0
#define CHUNK_IN_US 1

typedef struct heap_node_struct {
    uint8_t is_used; // 0 - not used, 1 - used
    size_t size; // the size of the user data (not including the header)
    struct heap_node_struct * previous;
    struct heap_node_struct * next;
} heap_chunk_t;

typedef struct heap_struct {
    heap_chunk_t * heap_first;
} heap_t;

uint32_t alloc_unfreable_phys(size_t size, uint8_t align); // allocate a non freable type of memory, 0 - not align, 1 - align

void print_heap_status(); // pring the heap status
void initialize_heap();  // initiate the heap maneger 
void* kalloc(size_t size); // allocate memory
void kfree(void * chunk); // free a chunk

#endif // KHEAP_H