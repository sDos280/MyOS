#ifndef TTY_DRIVER_H
#define TTY_DRIVER_H

#include "drivers/keys.h"
#include "drivers/event_driver.h"
#include "kernel/terminal.h"
#include "screen.h"
#include "types.h"

#define TTY_MAX_STRING_PRINT 1000

typedef struct tty_struct {
    terminal_t terminal;            /* the terminal buffer of the tty */
    event_handler_t event_handler;  /* the event handler of the tty */
    uint8_t shift_pressed;          /* 1 for pressed shift, else 0 */
} tty_t;

void tty_init(tty_t * tty);
void tty_write_string(tty_t * tty, char * str); /* write a string, max number of chars to print is TTY_MAX_STRING_PRINT */
void tty_handle_event(tty_t * tty);

#endif // TTY_DRIVER_H