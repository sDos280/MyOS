#ifndef CONSOLE_DRIVER_H
#define CONSOLE_DRIVER_H

#include "drivers/keys.h"
#include "drivers/event_driver.h"
#include "kernel/terminal.h"
#include "types.h"

typedef struct console_struct {
    terminal_t terminal;            /* the terminal buffer of the tty */
    uint8_t shift_pressed;          /* 1 for pressed shift, else 0 */
} console_t;

void console_init(console_t *con);
void console_clean_terminal_buffers(console_t *con);
void console_write_string(console_t *con, char * str); /* write a string, max number of chars to print is TTY_MAX_STRING_PRINT */
void console_handle_event(console_t *con, event_t e);

#endif // CONSOLE_DRIVER_H