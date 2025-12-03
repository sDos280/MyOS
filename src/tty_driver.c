#include "tty_driver.h"
#include "screen.h"
#include "utils.h"

void tty_initialize(tty_t * tty) {
    tty_clean_buffer(tty);
}

void tty_clean_buffer(tty_t * tty) {
    /* clear the buffer */
    tty->column = 0;
    tty->row = 0;
    tty->screen_row = 0;
    memset(tty->buffer, 0, SCREEN_BUFFER_ROWS * SCREEN_BUFFER_COLUMNS);
}

void tty_write_char(tty_t * tty, char c) {
    uint32_t relative_row = (tty->row - tty->screen_row) % SCREEN_BUFFER_ROWS;
    uint8_t print_char_to_screen = (relative_row < SCREEN_ROWS);
    uint8_t flush_screen = 0;
    uint32_t column_copy = tty->column;
    
    /* check if next char to write is over the screen */
    if (relative_row >= SCREEN_ROWS) {
        tty->screen_row = (tty->screen_row + 1) % SCREEN_BUFFER_ROWS;
        flush_screen = 1;
    }

    switch (c)
    {
    case '\n':
        tty->column = 0;
        tty->row++;
        print_char_to_screen = 0;
        break;
    
    default:
        tty->buffer[tty->row][tty->column] = c;
        tty->column++;
        break;
    }

    if (tty->column >= SCREEN_COLUMNS) { 
        tty->column = 0;
        tty->row++;
    }

    if (tty->row >= SCREEN_BUFFER_ROWS) {  /* next writing will overflow the buffer, so clean the buffer complitly */
        tty->row = 0;
        tty_clean_buffer(tty);
        flush_screen = 1;
    }

    if (print_char_to_screen) screen_print_char(c, relative_row, column_copy);
    if (flush_screen == 1) screen_flush_tty(tty);

}