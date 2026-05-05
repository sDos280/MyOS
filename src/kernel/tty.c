#include "kernel/tty.h"
#include "kernel/screen.h"
#include "utils/utils.h"

static const char *tty_parse_ansi_escape(tty_t *tty, const char *s);

static const char *tty_parse_ansi_escape(tty_t *tty, const char *s)
{
    char light = 0;
    char real_color;
    char temp;

    // s points to ESC
    s += 2; // skip 'ESC' '['

    int code = 0;

    while (*s >= '0' && *s <= '9')
    {
        code = code * 10 + (*s - '0');
        s++;
    }

    if (*s != 'm')
    {
        return s; // unsupported command
    }

    if (code == 0)
    {
        tty_set_foreground_colour(tty, LIGHT_GRAY_COLOUR);
        tty_set_background_colour(tty, BLACK_COLOUR);
        goto end;
    }

    temp = code % 10;

    /* check if code not defined */
    if (temp >= 0 && code <= 7)
        goto end;

    switch (temp)
    {
    case 0:
        real_color = BLACK_COLOUR;
        break;
    case 1:
        real_color = RED_COLOUR;
        break;
    case 2:
        real_color = GREEN_COLOUR;
        break;
    case 3:
        real_color = BROWN_COLOUR;
        break;
    case 4:
        real_color = BLUE_COLOUR;
        break;
    case 5:
        real_color = MEGENTA_COLOUR;
        break;
    case 6:
        real_color = CYAN_COLOUR;
        break;
    case 7:
        real_color = LIGHT_GRAY_COLOUR;
        break;

    default:
        break;
    }

    if (code >= 30 && code <= 37)
    {
        tty_set_foreground_colour(tty, real_color);
    }
    else if (code >= 40 && code <= 47)
    {
        tty_set_background_colour(tty, real_color);
    }
    else if (code >= 90 && code <= 97)
    {
        tty_set_foreground_colour(tty, real_color | LIGHT_COLOUR);
    }
    else if (code >= 100 && code <= 107)
    {
        tty_set_background_colour(tty, real_color | LIGHT_COLOUR);
    }

end:
    return s + 1; // skip final 'm'
}

void tty_init(tty_t *tty)
{
    tty_set_foreground_colour(tty, LIGHT_GRAY_COLOUR);
    tty_set_background_colour(tty, BLACK_COLOUR);
    tty_clean_buffer(tty);
    event_init_event_handler(&tty->event_handler);
    tty->ankered = TTY_ANKERED;
}

void tty_set_foreground_colour(tty_t *tty, uint8_t colour)
{
    tty->foregroup_colour = colour;
}

void tty_set_background_colour(tty_t *tty, uint8_t colour)
{
    tty->backgroup_colour = colour << 4;
}

void tty_set_anker_state(tty_t *tty, uint8_t state)
{
    tty->ankered = state;

    if (state == TTY_ANKERED)
    { /* there is a need to reset screen row */
        if (tty->row < SCREEN_ROWS)
        {
            tty->screen_row = 0;
        }
        else if (tty->row == SCREEN_ROWS)
        {
            tty->screen_row = tty->row - (tty->column != 0);
        }
        else
        { // tty->row > SCREEN_ROWS
            tty->screen_row = tty->row - SCREEN_ROWS + (tty->column != 0);
        }

        screen_flush_tty(tty); /* update the screen */
    }
}

void tty_set_screen_row(tty_t *tty, int32_t row)
{
    if (row < 0)
        row = 0;
    else if (row >= 0 && row < SCREEN_BUFFER_ROWS - SCREEN_ROWS)
        tty->screen_row = row;
    else
        tty->screen_row = SCREEN_BUFFER_ROWS - SCREEN_ROWS;

    screen_flush_tty(tty); /* update the screen */
}

void tty_clean_buffer(tty_t *tty)
{
    /* clear the buffer */
    tty->column = 0;
    tty->row = 0;
    tty->screen_row = 0;
    memset(tty->screen_text_buffer, 0, SCREEN_BUFFER_ROWS * SCREEN_BUFFER_COLUMNS);
    memset(tty->screen_colour_buffer, (BLACK_COLOUR << 4) | LIGHT_GRAY_COLOUR,
           SCREEN_BUFFER_ROWS * SCREEN_BUFFER_COLUMNS);

    screen_flush_tty(tty); /* update the screen */
}

void tty_write_char(tty_t *tty, char c)
{
    uint32_t relative_row = (tty->row - tty->screen_row) % SCREEN_BUFFER_ROWS;
    uint8_t print_char_to_screen = (relative_row < SCREEN_ROWS);
    uint8_t flush_screen = 0;
    uint32_t column_copy = tty->column;
    char colour = (BLACK_COLOUR << 4) | LIGHT_GRAY_COLOUR;

    /* check if next char to write is over the screen */
    if (relative_row >= SCREEN_ROWS)
    {
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
        tty->screen_text_buffer[tty->row][tty->column] = c;
        colour = tty->backgroup_colour | tty->foregroup_colour;
        tty->screen_colour_buffer[tty->row][tty->column] = colour;
        tty->column++;
        break;
    }

    if (tty->column >= SCREEN_COLUMNS)
    {
        tty->column = 0;
        tty->row++;
    }

    if (tty->row >= SCREEN_BUFFER_ROWS)
    { /* next writing will overflow the buffer, so clean the buffer complitly */
        tty->row = 0;
        tty_clean_buffer(tty);
        flush_screen = 1;
    }

    if (print_char_to_screen)
        screen_print_char(c, colour, relative_row, column_copy);
    if (flush_screen == 1)
        screen_flush_tty(tty);
}

void tty_write_string(tty_t *tty, char *str)
{
    for (size_t i = 0; i < TTY_MAX_STRING_PRINT; i++)
    {
        if (str[i] == '\0')
            break;

        /* check if command starts here */
        if (str[i] == '\033' && i + 1 < TTY_MAX_STRING_PRINT && str[i + 1] == '[')
        {
            char *new_str = tty_parse_ansi_escape(tty, &str[i]);

            /* jump over the command */
            if (new_str != &str[i])
            {
                i += (new_str - &str[i]) - 1; /* -1 because after the continue will do i++ */
                continue;
            }
        }

        tty_write_char(tty, str[i]);
    }
}

void tty_handle_event(tty_t *tty)
{
    if (event_is_events_queue_empty(&tty->event_handler))
        return;

    event_t e;
    ring_queue_status_t st = event_pop_event(&tty->event_handler, &e);

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
        /* regular alpha */
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
            tty_write_char(tty, key_to_ascii(e.code, tty->shift_pressed));
            break;
        
        /* TODO: Add all the other keys functionalities */

        case KEY_PAGEUP:
            tty_set_screen_row(tty, tty->screen_row-1);
            break;

        case KEY_PAGEDOWN:
            tty_set_screen_row(tty, tty->screen_row+1);
            break;
        
        case KEY_LCTRL:
        case KEY_RCTRL:
            tty->ankered = !tty->ankered;
            break;

        default:
            break;
    }
}