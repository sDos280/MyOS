#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"

void clear_screen();
void print_char(char c);
void print(const char* str);
void print_int(uint32_t num);
void print_hex(uint32_t num);

#endif // SCREEN_H