#ifndef PRINT_H
#define PRINT_H

#include "types.h"
#include "tty.h"

void print_set_tty(tty_t * tty);
void print_clean_screen();
void putchar(char c);
char getchar();
void put_key_press(uint8_t key);
uint8_t get_key_press();

void print_char(char c);
void printf(const char* format, ...);
void print_const_string(const char* str);
void print_int_padded(uint32_t num, int width, char pad_char);
void print_hex_padded(uint32_t num, int width, char pad_char);
void print_hexdump(const void *data, size_t size); // print hexdump of something


#endif // PRINT_H