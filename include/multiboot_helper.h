#ifndef MULTIBOOT_HELPER_H
#define MULTIBOOT_HELPER_H

#include "multiboot.h"

void print_multiboot_usable_memory_map(multiboot_uint32_t mmap_length, multiboot_uint32_t mmap_addr); // print nicly the multiboot usable (free to use) memory map
void print_multiboot_memory_map_t(multiboot_memory_map_t * st); // print nicly an instance of print_multiboot_memory_map_t

#endif // MULTIBOOT_HELPER_H