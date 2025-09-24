#include "kheap.h"


extern uint32_t end;
uint32_t placement_address = (uint32_t)&end; // the last unused address

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