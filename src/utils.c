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

size_t strlen(const char *s) {
    const char *p = s;
    
    while (*p != '\0') {
        p++;
    }

    return p - s;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    const unsigned char *us1 = (const unsigned char *)s1;
    const unsigned char *us2 = (const unsigned char *)s2;

    // Loop while 'n' is greater than 0
    while (n > 0) {
        // If characters differ, return the difference
        if (*us1 != *us2) {
            return (*us1 > *us2) ? 1 : -1; // Standard mandates sign of difference, not necessarily the exact difference
        }
        // If a null terminator is reached, strings are equal so far; return 0
        if (*us1 == '\0') {
            return 0;
        }

        // Move to the next character and decrement count
        us1++;
        us2++;
        n--;
    }

    // If 'n' characters were compared and no difference was found, the strings are equal
    return 0;
}