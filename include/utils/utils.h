#ifndef UTILS_H
#define UTILS_H

#include "types.h"

void memset(void* dest, uint8_t val, uint32_t len);
void *memcpy(void *dest, const void *src, size_t n);
size_t strlen(const char *s);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dest, const char *src, size_t n);

#endif // UTILS_H