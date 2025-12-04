#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"
#include "tty.h"

void screen_flush_tty(tty_t * tty);
void screen_print_char(char c, uint32_t row, uint32_t column);

#endif // SCREEN_H