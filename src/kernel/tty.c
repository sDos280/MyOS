#include "kernel/tty.h"
#include "kernel/screen.h"
#include "kernel/panic.h"
#include "utils/utils.h"

void tty_init(tty_t *tty)
{
    terminal_init(&tty->terminal);
    event_init_event_handler(&tty->event_handler);
}

void tty_clean_terminal_buffers(tty_t * tty) 
{
    terminal_clean_buffers(&tty->terminal);
}

void tty_write_string(tty_t *tty, char *str)
{
    terminal_write_string(&tty->terminal, str);
}

void tty_handle_event(tty_t *tty)
{
    if (event_is_events_queue_empty(&tty->event_handler))
        return;

    event_t e;
    ring_queue_status_t st = event_pop_event(&tty->event_handler, &e);

    if (st != RING_QUEUE_OK) PANIC("Event handler got event pop not ok");
    
    /* check for not key press */
    if (!(e.type == KEY_RELEASE || e.type == KEY_PRESS))
        return; /* not implemented (yet) */

    if (e.code == KEY_LSHIFT || e.code == KEY_RSHIFT)
    {
        tty->shift_pressed = (e.type == KEY_PRESS) ? 1 : 0;
        return;
    }

    /* from this point all events are to be key press */
    if (e.type != KEY_PRESS)
        return;

    switch (e.code) {
        /* Digits (top row) */
        case KEY_0:
        case KEY_1:
        case KEY_2:
        case KEY_3:
        case KEY_4:
        case KEY_5:
        case KEY_6:
        case KEY_7:
        case KEY_8:
        case KEY_9:
        /* Letters */
        case KEY_A:
        case KEY_B:
        case KEY_C:
        case KEY_D:
        case KEY_E:
        case KEY_F:
        case KEY_G:
        case KEY_H:
        case KEY_I:
        case KEY_J:
        case KEY_K:
        case KEY_L:
        case KEY_M:
        case KEY_N:
        case KEY_O:
        case KEY_P:
        case KEY_Q:
        case KEY_R:
        case KEY_S:
        case KEY_T:
        case KEY_U:
        case KEY_V:
        case KEY_W:
        case KEY_X:
        case KEY_Y:
        case KEY_Z:
            terminal_write_char(&tty->terminal, key_to_ascii(e.code, tty->shift_pressed));
            break;

        /* Control and special keys */
        case KEY_SPACE:
            terminal_write_char(&tty->terminal, ' ');
            break;
        case KEY_TAB: /* a tab is 4 spaces */
            terminal_write_char(&tty->terminal, ' ');
            terminal_write_char(&tty->terminal, ' ');
            terminal_write_char(&tty->terminal, ' ');
            terminal_write_char(&tty->terminal, ' ');
            break;
        case KEY_ENTER:
            terminal_write_char(&tty->terminal, '\n');
            break;
        
        /* TODO: Add all the other keys functionalities */

        case KEY_PAGEUP:
            terminal_set_screen_row(&tty->terminal, tty->terminal.screen_row-1);
            break;

        case KEY_PAGEDOWN:
            terminal_set_screen_row(&tty->terminal, tty->terminal.screen_row+1);
            break;
        
        case KEY_LCTRL:
        case KEY_RCTRL:
            terminal_set_anker_state(&tty->terminal, !tty->terminal.ankered);
            break;

        default:
            break;
    }
}