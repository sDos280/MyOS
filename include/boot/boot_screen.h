#ifndef BOOT_SCREEN_H
#define BOOT_SCREEN_H

#include "types.h"

void boot_initialize_screen() __attribute__ ((section(".boot.text")));
void boot_clear_screen() __attribute__ ((section(".boot.text")));
void boot_print_char(char c) __attribute__ ((section(".boot.text")));
void boot_printf(const char* format, ...) __attribute__ ((section(".boot.text")));
void boot_print_const_string(const char* str) __attribute__ ((section(".boot.text")));
void boot_print_int_padded(uint32_t num, int width, char pad_char) __attribute__ ((section(".boot.text")));
void boot_print_hex_padded(uint32_t num, int width, char pad_char) __attribute__ ((section(".boot.text")));
void boot_print_hexdump(const void *data, size_t size) __attribute__ ((section(".boot.text"))); // print hexdump of something

#endif // BOOT_SCREEN_H