#include "kernel/print.h"
#include "mm/kheap.h"
#include "mm/paging.h"
#include "mm/pmm.h"

// Defined in the linker
extern uint32_t __heap_start;  // end is defined in the linker scrip
extern uint32_t __heap_end;  // heap_start is defined in the linker script
heap_t kernel_heap;   


void print_heap_status() {
    printf("--- Kernel Heap Status ---\n");
    
    if (kernel_heap.heap_first == NULL) {
        printf("Heap is empty or uninitialized.\n");
        return;
    }

    heap_chunk_t *current_chunk = kernel_heap.heap_first;
    uint32_t chunk_count = 0;

    while (current_chunk != NULL) {
        chunk_count++;

        printf("Chunk #%d at address: %p\n", chunk_count, (uint32_t)current_chunk);

        printf("  Status: ");
        if (current_chunk->is_used == CHUNK_IN_US) {
            printf("USED");
        } else {
            printf("FREE");
        }
        printf("\n");

        printf("  User Data Size: 0x%x bytes\n", current_chunk->size);

        printf("  Previous Chunk: 0x%p\n", (uint32_t)current_chunk->previous);

        printf("  Next Chunk: 0x%p\n", (uint32_t)current_chunk->next);
        
        printf("\n");

        // Move to the next chunk in the linked list
        current_chunk = current_chunk->next;
    }

    printf("--- End of Heap Status (Total Chunks: %d) ---\n", chunk_count);
}

void heap_init(){
    /* map the heap somewere */
    void * paddr;

    for (uint32_t vaddr = (uint32_t)&__heap_start; vaddr < (uint32_t)&__heap_end; vaddr += PAGE_SIZE) {
        paddr = pmm_alloc_frame();
        paging_map_page(vaddr, (uint32_t)paddr, PG_WRITABLE | PG_PRESENT);
    }

    heap_chunk_t * first_chunk = (heap_chunk_t *)(&__heap_start);
    first_chunk->is_used = CHUNK_NOT_IN_US;
    first_chunk->size = KHEAP_INITIAL_SIZE - sizeof(heap_chunk_t);
    first_chunk->previous = NULL;
    first_chunk->next = NULL;
    kernel_heap.heap_first = first_chunk;
}

void* kalloc(size_t size){
    heap_chunk_t * current = kernel_heap.heap_first;

    while (current != NULL)
    {
        if (current->is_used == CHUNK_NOT_IN_US && (current->size > size + sizeof(heap_chunk_t) /* the > in here may be replaced with more rigorous condition */)){
            // calculate returned chunk position in memory
            void * position_after_chunk = (void *)current + current->size + sizeof(heap_chunk_t);
            heap_chunk_t * new = (heap_chunk_t *)(position_after_chunk - size - sizeof(heap_chunk_t));
            
            // NOTE: next = current->next

            // set list's pointers
            // set pointers for next
            if (current->next != NULL) {
                current->next->previous = new;
            }

            // set pointers of new chunk
            new->next = current->next;
            new->previous = current;

            // set current chunk pointers
            current->next = new;

            // initialize chunk
            new->is_used = CHUNK_IN_US;
            new->size = size;

            // update current chunk size
            current->size -= size + sizeof(heap_chunk_t);

            return (void *)new + sizeof(heap_chunk_t);
        }

        current = current->next;
    }
}

void kfree(void * user_pointer) {
    if (user_pointer == NULL) return;
    
    heap_chunk_t * chunk = (heap_chunk_t *)(user_pointer - sizeof(heap_chunk_t));

    // update chunk status
    chunk->is_used = CHUNK_NOT_IN_US;
    
    // assimilate next chunk to the current chunk if he is free
    if (chunk->next != NULL && chunk->next->is_used == CHUNK_NOT_IN_US) {
        // change the prespective to the next chunk
        heap_chunk_t * next = chunk->next->next;
        heap_chunk_t * current = chunk->next;
        heap_chunk_t * previous = chunk;

        // update pointers of next
        if (next != NULL) {
            next->previous = previous;
        }
        
        // there isn't a need to update the pointers of current since it will be assimilated to previous
        // update pointers of previous
        previous->next = next;

        // update the previous (in the current prespective) chunk size
        previous->size += sizeof(heap_chunk_t) + current->size;
    }

    // assimilate current chunk to the previous chunk if he is free
    if (chunk->previous != NULL && chunk->previous->is_used == CHUNK_NOT_IN_US) {
        // preserve the current prespective
        heap_chunk_t * next = chunk->next;
        heap_chunk_t * current = chunk;
        heap_chunk_t * previous = chunk->previous;

        // update pointers of next
        if (next != NULL) {
            next->previous = previous;
        }
        
        // there isn't a need to update the pointers of current since it will be assimilated to previous
        // update pointers of previous
        previous->next = next;

        // update the previous (in the current prespective) chunk size
        previous->size += sizeof(heap_chunk_t) + current->size;
    }
}