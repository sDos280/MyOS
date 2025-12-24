#ifndef BOOT_IPL_H
#define BOOT_IPL_H

#include "mm/paging.h"
#include "multiboot.h"
#include "types.h"

/**
 * @brief Initialize paging and enable it.
 * @param mb_info   Multiboot information structure.
 * @param mb_magic  Multiboot magic value.
 */
void boot_ipl (multiboot_info_t* multiboot_info_structure, uint32_t multiboot_magic) 
    __attribute__ ((section(".boot.text")));

/**
 * @brief Map a single 4KB page.
 * @param pd     Page directory.
 * @param vaddr  Virtual address.
 * @param paddr  Physical address.
 * @param flags  Page flags.
 */
uint8_t boot_map_page(page_directory_t* d, void* vaddr, void* paddr, uint32_t page_flags) 
    __attribute__ ((section(".boot.text")));

#endif // BOOT_IPL_H