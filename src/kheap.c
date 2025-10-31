#include "kheap.h"
#include "screen.h"

extern uint32_t end;
uint32_t placement_address = (uint32_t)&end; // the last unused address
heap_t kernel_heap;

uint32_t alloc_unfreable_phys(size_t size, uint8_t align) {
    uint32_t temp;

    if (align == 1) {
        // ensure placement_address is page-aligned
        if ((placement_address & 0xFFFFF000) != placement_address) 
            placement_address = (placement_address + 0x1000) & 0xFFFFF000; // align placement
    }

    temp = placement_address;
    placement_address += size;
    return temp;
}

void print_heap_status() {
    clear_screen();
    print("--- Kernel Heap Status ---\n");
    
    if (kernel_heap.heap_first == NULL) {
        print("Heap is empty or uninitialized.\n");
        return;
    }

    heap_chunk_t *current_chunk = kernel_heap.heap_first;
    uint32_t chunk_count = 0;

    while (current_chunk != NULL) {
        chunk_count++;

        print("Chunk #");
        print_int(chunk_count);
        print(" at address: ");
        // Print the address of the actual chunk structure
        print_hex((uint32_t)current_chunk); 
        print("\n");

        print("  Status: ");
        if (current_chunk->is_used == CHUNK_IN_US) {
            print("USED");
        } else {
            print("FREE");
        }
        print("\n");

        print("  User Data Size: ");
        print_int(current_chunk->size);
        print(" bytes\n");

        print("  Previous Chunk: ");
        print_hex((uint32_t)current_chunk->previous);
        print("\n");

        print("  Next Chunk: ");
        print_hex((uint32_t)current_chunk->next);
        print("\n");
        
        print("\n");

        // Move to the next chunk in the linked list
        current_chunk = current_chunk->next;
    }

    print("--- End of Heap Status (Total Chunks: ");
    print_int(chunk_count);
    print(") ---\n");
}

void initialize_heap(){
    heap_chunk_t * first_chunk = (heap_chunk_t *)KHEAP_START;
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