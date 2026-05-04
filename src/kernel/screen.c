#include "kernel/screen.h"
#include "types.h"
#include "utils/utils.h"

static volatile char *video = (volatile char*)0xFFFFF000;

void screen_flush_tty(tty_t * tty) {
    for (uint32_t y = 0; y < SCREEN_ROWS; y++) {
        for (uint32_t x = 0; x < SCREEN_COLUMNS; x++) {
            video[(y * SCREEN_COLUMNS + x) * 2] = tty->screen_text_buffer[(y + tty->screen_row) % SCREEN_BUFFER_ROWS][x];
            video[(y * SCREEN_COLUMNS + x) * 2 + 1] = tty->screen_colour_buffer[(y + tty->screen_row) % SCREEN_BUFFER_ROWS][x];
        }
    }
}

void screen_print_char(char c, char colour, uint32_t row, uint32_t column) {
    video[(row * SCREEN_COLUMNS + column) * 2] = c;
    video[(row * SCREEN_COLUMNS + column) * 2 + 1] = colour;
}