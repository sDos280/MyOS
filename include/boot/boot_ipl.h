#ifndef BOOT_IPL_H
#define BOOT_IPL_H

#include "mm/paging.h"
#include "multiboot.h"
#include "types.h"

void boot_ipl (multiboot_info_t* multiboot_info_structure, uint32_t multiboot_magic) __attribute__ ((section(".boot.text")));
uint8_t boot_map_page(page_directory_t* d, void* vaddr, void* paddr, uint32_t page_flags) __attribute__ ((section(".boot.text")));

#endif // BOOT_IPL_H