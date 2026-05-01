#ifndef SCREEN_H
#define SCREEN_H

#include "kernel/tty.h"
#include "types.h"

#define SCREEN_BLACK_COLOUR          0x00
#define SCREEN_BLUE_COLOUR           0x01
#define SCREEN_GREEN_COLOUR          0x02
#define SCREEN_CYAN_COLOUR           0x03
#define SCREEN_RED_COLOUR            0x04
#define SCREEN_MEGENTA_COLOUR        0x05
#define SCREEN_BROWN_COLOUR          0x06
#define SCREEN_LIGHT_GRAY_COLOUR     0x07
#define SCREEN_DARK_GRAY_COLOUR      0x08
#define SCREEN_LIGHT_BLUE_COLOUR     0x09
#define SCREEN_LIGHT_GREEN_COLOUR    0x0A
#define SCREEN_LIGHT_CYAN_COLOUR     0x0B
#define SCREEN_LIGHT_RED_COLOUR      0x0C
#define SCREEN_LIGHT_MEGENTA_COLOUR  0x0D
#define SCREEN_YELOW_COLOUR          0x0E
#define SCREEN_WHITE_GRAY_COLOUR     0x0F

#define SCREEN_LIGHT_COLOUR          0b1000

void screen_set_foreground_colour(uint8_t colour);
void screen_set_background_colour(uint8_t colour);
void screen_flush_tty(tty_t * tty);
void screen_print_char(char c, uint32_t row, uint32_t column);

#endif // SCREEN_H