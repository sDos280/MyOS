#ifndef MULTIBOOT_INFO_H
#define MULTIBOOT_INFO_H

#include "multiboot.h"
#include "types.h"

void multiboot_info_print(uint32_t magic, const multiboot_info_t *mbi);
void multiboot_info_invalidate_unavailable_memory(multiboot_info_t *mbi);  // invalidate (by setting as used in pmm) unavailable memory 

#endif // MULTIBOOT_INFO_H