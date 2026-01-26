#ifndef PRINT_H
#define PRINT_H

#include "kernel/tty.h"
#include "types.h"

tty_t * print_get_tty();
void print_set_tty(tty_t * tty);
void print_clean_screen();
void putchar(char c);
char getchar();
void put_key_press(uint8_t key);
uint8_t get_key_press();

void printf(const char* format, ...);
void print_hexdump(const void *data, size_t size); // print hexdump of something


#endif // PRINT_H