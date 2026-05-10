#ifndef MULTIBOOT_HELPER_H
#define MULTIBOOT_HELPER_H

#include "multiboot.h"
#include "types.h"

void multiboot_print_info(uint32_t magic, const multiboot_info_t *mbi);
void multiboot_helper_invalidate_unavailable_memory(multiboot_info_t *mbi);  // invalidate (by setting as used in pmm) unavailable memory 

#endif // MULTIBOOT_HELPER_H