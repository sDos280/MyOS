#include "kernel/tty.h"
#include "kernel/screen.h"
#include "kernel/panic.h"
#include "utils/utils.h"

void tty_init(tty_t *tty,  void (*event_handler_func)(event_t))
{
    console_init(&tty->console);
    event_init_event_handler(&tty->event_handler);
    tty->event_handler_func = event_handler_func;
}

void tty_write_string(tty_t *tty, char *str)
{
    console_write_string(&tty->console, str);
}

void tty_handle_event(tty_t *tty)
{
    if (event_is_events_queue_empty(&tty->event_handler))
        return;

    event_t e;
    ring_queue_status_t st = event_pop_event(&tty->event_handler, &e);

    if (st != RING_QUEUE_OK) PANIC("Event handler got event pop not ok");
    
    tty->event_handler_func(e);
}