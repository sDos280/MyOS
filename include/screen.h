#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"

#define SCREEN_COLUMNS 80
#define SCREEN_ROWS 25
#define ROW_FACTOR 4
#define SCREEN_BUFFER_COLUMNS 80
#define SCREEN_BUFFER_ROWS (SCREEN_ROWS * ROW_FACTOR)

typedef struct screen_handler_struct {
    uint32_t screen_start_row;  // the starting row the screen will be presented as
    uint32_t row; // the row next to be writen
    uint32_t column; // the column next to be writen
    char screen_buffer[SCREEN_BUFFER_ROWS * SCREEN_BUFFER_COLUMNS * 2];
} screen_handler_t;

void assign_screen_handler(screen_handler_t * handler);  // assign a new screen handler
void initialize_screen_handler();  // initialize the current assigned screen handler
void clear_screen();
void print_char(char c);
void printf(const char* format, ...);
void print_const_string(const char* str);
void print_int_padded(uint32_t num, int width, char pad_char);
void print_hex_padded(uint32_t num, int width, char pad_char);
void print_hexdump(const void *data, size_t size); // print hexdump of something

#endif // SCREEN_H