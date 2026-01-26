#ifndef UTILS_H
#define UTILS_H

#include "types.h"

void memset(void* dest, uint8_t val, uint32_t len);
void *memcpy(void *dest, const void *src, size_t n);
size_t strlen(const char *s);

#endif // UTILS_H