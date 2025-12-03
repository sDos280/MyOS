#include "multiboot_helper.h"
#include "print.h"

void print_multiboot_usable_memory_map(multiboot_uint32_t mmap_length, multiboot_uint32_t mmap_addr) {
    multiboot_memory_map_t * entry = (multiboot_memory_map_t *)mmap_addr;
    uint32_t end = mmap_addr + mmap_length;

    while ((uint32_t)entry < end) {
        if (entry->type == 2)
            print_multiboot_memory_map_t(entry);

        // Move to next entry:
        entry = (multiboot_memory_map_t *)((void *)entry + entry->size + sizeof(entry->size));
    }
}

void print_multiboot_memory_map_t(multiboot_memory_map_t * st) {
    printf("Multiboot Memory Map Entry:\n");
    printf("    Size: %d\n", st->size);
    printf("    Addr: %p\n", st->addr);
    printf("    Len: %x\n", st->len);

    switch (st->type)
    {
    case 1:
        printf("    Type: Usable RAM\n");
        break;
    case 2:
        printf("    Type: Reserved\n");
        break;
    case 3:
        printf("    Type: ACPI Reclaimable\n");
        break;
    case 4:
        printf("    Type: ACPI NVS\n");
        break;
    case 5:
        printf("    Type: Bad memory\n");
        break;
    
    default:
        break;
    }
}