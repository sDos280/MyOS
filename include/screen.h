#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"

void initialize_screen();
void clear_screen();
void print_char(char c);
void printf(const char* format, ...);
void print_const_string(const char* str);
void print_int_padded(uint32_t num, int width, char pad_char);
void print_hex_padded(uint32_t num, int width, char pad_char);
void print_hexdump(const void *data, size_t size); // print hexdump of something

#endif // SCREEN_H