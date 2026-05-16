#ifndef TTY_DRIVER_H
#define TTY_DRIVER_H

#include "drivers/keys.h"
#include "drivers/event_driver.h"
#include "drivers/console_driver.h"
#include "kernel/terminal.h"
#include "screen.h"
#include "types.h"

#define TTY_MAX_STRING_PRINT 1000

typedef struct tty_struct {
    event_handler_t event_handler;  /* the event handler of the tty */
    console_t console;              /* the tty console (used only for screen print really), fix: we should seperate tty and consle more */
    void (*event_handler_func)(event_t);  /* the tty's event handler function, called on each event */
} tty_t;

void tty_init(tty_t * tty,  void (*event_handler_func)(event_t));
void tty_clean_terminal_buffers(tty_t * tty);
void tty_write_string(tty_t * tty, char * str); /* write a string, max number of chars to print is TTY_MAX_STRING_PRINT */
void tty_handle_event(tty_t * tty);

#endif // TTY_DRIVER_H