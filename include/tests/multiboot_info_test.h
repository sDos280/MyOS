#ifndef MULTIBOOT_INFO_TEST_H
#define MULTIBOOT_INFO_TEST_H

#include "multiboot.h"
#include "types.h"

void multiboot_print_info(uint32_t magic, const multiboot_info_t *mbi);

#endif // MULTIBOOT_INFO_TEST_H