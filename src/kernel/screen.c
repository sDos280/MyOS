#include "kernel/screen.h"
#include "types.h"
#include "utils.h"

#define BACKGROUND_COLOUR_DEFAULT 0x07
#define FOREGROUND_COLOUR_DEFAULT 0x00

static volatile char *video = (volatile char*)0xFFFFF000;
static char backgroup_colour = BACKGROUND_COLOUR_DEFAULT;
static char foregroup_colour = FOREGROUND_COLOUR_DEFAULT;

void screen_set_foreground_colour(uint8_t colour) {
    foregroup_colour = colour;
}

void screen_set_background_colour(uint8_t colour) {
    backgroup_colour = colour;
}

void screen_flush_tty(tty_t * tty) {
    for (uint32_t y = 0; y < SCREEN_ROWS; y++) {
        for (uint32_t x = 0; x < SCREEN_COLUMNS; x++) {
            video[(y * SCREEN_COLUMNS + x) * 2] = tty->screeb_buffer[(y + tty->screen_row) % SCREEN_BUFFER_ROWS][x];
            video[(y * SCREEN_COLUMNS + x) * 2 + 1] = backgroup_colour | foregroup_colour;
        }
    }
}

void screen_print_char(char c, uint32_t row, uint32_t column) {
    video[(row * SCREEN_COLUMNS + column) * 2] = c;
    video[(row * SCREEN_COLUMNS + column) * 2 + 1] = backgroup_colour | foregroup_colour;
}