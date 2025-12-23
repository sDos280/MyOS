#include "boot/boot_utils.h"

void boot_memset(void* dest, uint8_t val, uint32_t len) {
    uint8_t* dst = (uint8_t*)dest;
    while(len-- > 0)
        *dst++ = (uint8_t)val;
}