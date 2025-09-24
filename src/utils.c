#include "utils.h"

void memset(void* dest, uint8_t val, uint32_t len)
{
    uint8_t* dst = (uint8_t*)dest;
    while(len-- > 0)
        *dst++ = (uint8_t)val;
}