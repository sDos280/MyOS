#include "drivers/console_driver.h"

void console_init(console_t *con) 
{
    terminal_init(&con->terminal);
}

void console_clean_terminal_buffers(console_t *con) 
{
    terminal_clean_buffers(&con->terminal);
}

void console_write_string(console_t *con, char * str)
{
    terminal_write_string(&con->terminal, str);
}

void console_handle_event(console_t *con, event_t e) {
    /* check for not key press */
    if (!(e.type == KEY_RELEASE || e.type == KEY_PRESS))
        return; /* not implemented (yet) */

    if (e.code == KEY_LSHIFT || e.code == KEY_RSHIFT)
    {
        con->shift_pressed = (e.type == KEY_PRESS) ? 1 : 0;
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
            terminal_write_char(&con->terminal, key_to_ascii(e.code, con->shift_pressed));
            break;

        /* Control and special keys */
        case KEY_SPACE:
            terminal_write_char(&con->terminal, ' ');
            break;
        case KEY_TAB: /* a tab is 4 spaces */
            terminal_write_char(&con->terminal, ' ');
            terminal_write_char(&con->terminal, ' ');
            terminal_write_char(&con->terminal, ' ');
            terminal_write_char(&con->terminal, ' ');
            break;
        case KEY_ENTER:
            terminal_write_char(&con->terminal, '\n');
            break;
        
        /* TODO: Add all the other keys functionalities */

        case KEY_PAGEUP:
            terminal_set_screen_row(&con->terminal, con->terminal.screen_row-1);
            break;

        case KEY_PAGEDOWN:
            terminal_set_screen_row(&con->terminal, con->terminal.screen_row+1);
            break;
        
        case KEY_LCTRL:
        case KEY_RCTRL:
            terminal_set_anker_state(&con->terminal, !con->terminal.ankered);
            break;

        default:
            break;
    }
}