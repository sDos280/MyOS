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

    while (current_chunk != 0) {
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
    heap_chunk_t * copy = kernel_heap.heap_first;

    while (copy != NULL)
    {
        if (copy->is_used == CHUNK_NOT_IN_US && (copy->size > size + sizeof(heap_chunk_t) /* the > in here may be replaced with more rigorous condition */)){
            // calculate returned chunk position in memory
            void * position_after_chunk = (void *)copy + copy->size + sizeof(heap_chunk_t);
            heap_chunk_t * to_be_returned = (heap_chunk_t *)(position_after_chunk - size - sizeof(heap_chunk_t));

            // set list's pointers
            to_be_returned->next = copy->next;
            copy->next = to_be_returned;
            to_be_returned->previous = to_be_returned;

            // initialize chunk
            to_be_returned->is_used = CHUNK_IN_US;
            to_be_returned->size = size;

            // update current chunk size
            copy->size -= size + sizeof(heap_chunk_t);

            return (void *)to_be_returned + sizeof(heap_chunk_t);
        }

        copy = copy->next;
    }
}