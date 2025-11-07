#include "utils.h"

void memset(void* dest, uint8_t val, uint32_t len)
{
    uint8_t* dst = (uint8_t*)dest;
    while(len-- > 0)
        *dst++ = (uint8_t)val;
}

void *memcpy(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}