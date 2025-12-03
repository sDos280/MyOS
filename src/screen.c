#include "screen.h"
#include "types.h"
#include "utils.h"

#define BACKGROUND_COLOR 0x07

static volatile char *video = (volatile char*)0xB8000;

void screen_flush_tty(tty_t * tty) {
    for (uint32_t y = 0; y < SCREEN_ROWS; y++) {
        for (uint32_t x = 0; x < SCREEN_COLUMNS; x++) {
            video[(y * SCREEN_COLUMNS + x) * 2] = tty->buffer[(y + tty->screen_row) % SCREEN_BUFFER_ROWS][x];
            video[(y * SCREEN_COLUMNS + x) * 2 + 1] = BACKGROUND_COLOR;
        }
    }
}

void screen_print_char(char c, uint32_t row, uint32_t column) {
    video[(row * SCREEN_COLUMNS + column) * 2] = c;
    video[(row * SCREEN_COLUMNS + column) * 2 + 1] = BACKGROUND_COLOR;
}