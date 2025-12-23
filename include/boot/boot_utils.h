#ifndef BOOT_UTILS_H
#define BOOT_UTILS_H

#include "types.h"

void boot_memset(void* dest, uint8_t val, uint32_t len) __attribute__ ((section(".boot.text")));

#endif // BOOT_UTILS_H