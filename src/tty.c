#include "tty.h"
#include "screen.h"
#include "utils.h"

void tty_initialize(tty_t * tty) {
    tty_clean_buffer(tty);
    memset(tty->in_char_queue, 0, TTY_IN_CHAR_QUEUE_SIZE);
    tty->head = -1;
    tty->tail = -1;
    tty->ankered = TTY_ANKERED;
}

void tty_set_anker_state(tty_t * tty, uint8_t state) {
    tty->ankered = state;

    if (state == TTY_ANKERED) { /* there is a need to reset screen row*/
        if (tty->row < SCREEN_ROWS) {
            tty->screen_row = 0;
        } else if (tty->row == SCREEN_ROWS) {
            tty->screen_row = tty->row - (tty->column != 0);
        } else { // tty->row > SCREEN_ROWS
            tty->screen_row = tty->row - SCREEN_ROWS + (tty->column != 0);
        }
    }
}

void tty_set_screen_row(tty_t * tty, uint32_t row) {
    tty->screen_row = row % SCREEN_BUFFER_ROWS;
}

void tty_clean_buffer(tty_t * tty) {
    /* clear the buffer */
    tty->column = 0;
    tty->row = 0;
    tty->screen_row = 0;
    memset(tty->screeb_buffer, 0, SCREEN_BUFFER_ROWS * SCREEN_BUFFER_COLUMNS);
}

void tty_write_char(tty_t * tty, char c) {
    uint32_t relative_row = (tty->row - tty->screen_row) % SCREEN_BUFFER_ROWS;
    uint8_t print_char_to_screen = (relative_row < SCREEN_ROWS);
    uint8_t flush_screen = 0;
    uint32_t column_copy = tty->column;
    
    /* check if next char to write is over the screen */
    if (relative_row >= SCREEN_ROWS) {
        if (tty->ankered == TTY_ANKERED)
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
        tty->screeb_buffer[tty->row][tty->column] = c;
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

static uint8_t is_in_char_queue_empty(tty_t * tty) {
    return tty->head == -1;
}

static uint8_t is_in_char_queue_full(tty_t * tty) {
    return (tty->tail + 1) % TTY_IN_CHAR_QUEUE_SIZE == tty->head;
}

void tty_putc(tty_t * tty, char c) {
    if (is_in_char_queue_full(tty))
        tty_getc(tty);  // the quere is full, so remove one element to clear spot

    if (is_in_char_queue_empty(tty)) {
        tty->head = 0; // Initialize front for the first element
    }
    
    tty->tail = (tty->tail + 1) % TTY_IN_CHAR_QUEUE_SIZE;
    tty->in_char_queue[tty->tail] = c;
}

char tty_getc(tty_t * tty) {
    while (is_in_char_queue_empty(tty));  /* do nothing if key is empty */

    char data = tty->in_char_queue[tty->head];

    if (tty->head == tty->tail) { 
        // If queue becomes empty after getting this element, reset both pointers to -1
        tty->head = -1;
        tty->tail = -1;
    } else {
        // Use modulo to wrap the head index around
        tty->head = (tty->head + 1) % TTY_IN_CHAR_QUEUE_SIZE;
    }

    return data;
}