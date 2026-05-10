#include "kernel/terminal.h"
#include "utils/utils.h"

static const char *terminal_parse_ansi_escape(terminal_t *terminal, char *s);

static const char *terminal_parse_ansi_escape(terminal_t *terminal, char *s)
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
        terminal_set_foreground_colour(terminal, LIGHT_GRAY_COLOUR);
        terminal_set_background_colour(terminal, BLACK_COLOUR);
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
        terminal_set_foreground_colour(terminal, real_color);
    }
    else if (code >= 40 && code <= 47)
    {
        terminal_set_background_colour(terminal, real_color);
    }
    else if (code >= 90 && code <= 97)
    {
        terminal_set_foreground_colour(terminal, real_color | LIGHT_COLOUR);
    }
    else if (code >= 100 && code <= 107)
    {
        terminal_set_background_colour(terminal, real_color | LIGHT_COLOUR);
    }

end:
    return s + 1; // skip final 'm'
}

void terminal_init(terminal_t *terminal)
{
    terminal_set_foreground_colour(terminal, LIGHT_GRAY_COLOUR);
    terminal_set_background_colour(terminal, BLACK_COLOUR);
    terminal_clean_buffer(terminal);

    terminal->ankered = TERMINAL_ANKERED;
}

void terminal_set_foreground_colour(terminal_t *terminal, uint8_t colour)
{
    terminal->foregroup_colour = colour;
}

void terminal_set_background_colour(terminal_t *terminal, uint8_t colour)
{
    terminal->backgroup_colour = colour << 4;
}

void terminal_set_anker_state(terminal_t *terminal, uint8_t state)
{
    terminal->ankered = state;

    if (state == TERMINAL_ANKERED)
    { /* there is a need to reset screen row */
        if (terminal->row < SCREEN_ROWS)
        {
            terminal->screen_row = 0;
        }
        else if (terminal->row == SCREEN_ROWS)
        {
            terminal->screen_row = terminal->row - (terminal->column != 0);
        }
        else
        { // terminal->row > SCREEN_ROWS
            terminal->screen_row = terminal->row - SCREEN_ROWS + (terminal->column != 0);
        }

        screen_flush_text_and_colour_buffer(
            &terminal->text_buffer[0][0], 
            &terminal->colour_buffer[0][0], 
            terminal->screen_row, 
            0, 
            TERMINAL_ROWS, 
            TERMINAL_COLUMNS); /* update the screen */
    }
}

void terminal_set_screen_row(terminal_t *terminal, int32_t row)
{
    if (row < 0)
        row = 0;
    else if (row >= 0 && row < TERMINAL_ROWS - SCREEN_ROWS)
        terminal->screen_row = row;
    else
        terminal->screen_row = TERMINAL_ROWS - SCREEN_ROWS;

    screen_flush_text_and_colour_buffer(
            &terminal->text_buffer[0][0], 
            &terminal->colour_buffer[0][0], 
            terminal->screen_row, 
            0, 
            TERMINAL_ROWS, 
            TERMINAL_COLUMNS); /* update the screen */
}

void terminal_clean_buffer(terminal_t *terminal)
{
    /* clear the buffer */
    terminal->column = 0;
    terminal->row = 0;
    terminal->screen_row = 0;
    memset(terminal->text_buffer, 0, TERMINAL_ROWS * TERMINAL_COLUMNS);
    memset(terminal->colour_buffer, (BLACK_COLOUR << 4) | LIGHT_GRAY_COLOUR,
           TERMINAL_ROWS * TERMINAL_COLUMNS);

    screen_flush_text_and_colour_buffer(
            &terminal->text_buffer[0][0], 
            &terminal->colour_buffer[0][0], 
            terminal->screen_row, 
            0, 
            TERMINAL_ROWS, 
            TERMINAL_COLUMNS); /* update the screen */
}

void terminal_write_char(terminal_t *terminal, char c)
{
    uint32_t relative_row = (terminal->row - terminal->screen_row) % TERMINAL_ROWS;
    uint8_t print_char_to_screen = (relative_row < SCREEN_ROWS);
    uint8_t flush_screen = 0;
    uint32_t column_copy = terminal->column;
    char colour = (BLACK_COLOUR << 4) | LIGHT_GRAY_COLOUR;

    /* check if next char to write is over the screen */
    if (relative_row >= SCREEN_ROWS)
    {
        if (terminal->ankered == TERMINAL_ANKERED)
            terminal->screen_row = (terminal->screen_row + 1) % TERMINAL_ROWS;
        flush_screen = 1;
    }

    switch (c)
    {
    case '\n':
        terminal->column = 0;
        terminal->row++;
        print_char_to_screen = 0;
        break;

    default:
        terminal->text_buffer[terminal->row][terminal->column] = c;
        colour = terminal->backgroup_colour | terminal->foregroup_colour;
        terminal->colour_buffer[terminal->row][terminal->column] = colour;
        terminal->column++;
        break;
    }

    if (terminal->column >= SCREEN_COLUMNS)
    {
        terminal->column = 0;
        terminal->row++;
    }

    if (terminal->row >= TERMINAL_ROWS)
    { /* next writing will overflow the buffer, so clean the buffer complitly */
        terminal->row = 0;
        terminal_clean_buffer(terminal);
        flush_screen = 1;
    }

    if (print_char_to_screen)
        screen_print_char(c, colour, relative_row, column_copy);
    if (flush_screen == 1)
        screen_flush_text_and_colour_buffer(
            &terminal->text_buffer[0][0], 
            &terminal->colour_buffer[0][0], 
            terminal->screen_row, 
            0, 
            TERMINAL_ROWS, 
            TERMINAL_COLUMNS);
}

void terminal_write_string(terminal_t *terminal, char * str)
{
    for (size_t i = 0; i < TERMINAL_MAX_STRING_PRINT; i++)
    {
        if (str[i] == '\0')
            break;

        /* check if command starts here */
        if ((str[i] == '\033') && (i + 1 < TERMINAL_MAX_STRING_PRINT) && (str[i + 1] == '['))
        {
            const char *new_str = terminal_parse_ansi_escape(terminal, &str[i]);

            /* jump over the command */
            if (new_str != &str[i])
            {
                i += (new_str - &str[i]) - 1; /* -1 because after the continue will do i++ */
                continue;
            }
        }

        terminal_write_char(terminal, str[i]);
    }
}