#include "kernel/tty.h"
#include "kernel/screen.h"
#include "utils.h"

static const char *tty_parse_ansi_escape(tty_t *tty, const char *s);


static const char *tty_parse_ansi_escape(tty_t *tty, const char *s) {
    char light = 0;
    char real_color;
    char temp;

    // s points to ESC
    s += 2; // skip 'ESC' '['

    int code = 0;

    while (*s >= '0' && *s <= '9') {
        code = code * 10 + (*s - '0');
        s++;
    }

    if (*s != 'm') {
        return s;  // unsupported command
    }

    
    if (code == 0) {
        tty_set_foreground_colour(tty, LIGHT_GRAY_COLOUR);
        tty_set_background_colour(tty, BLACK_COLOUR);
        goto end;
    }

    temp = code % 10;

    /* check if code not defined */
    if (temp >= 0 && code <= 7) goto end;

    switch (temp) {
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

    if(code >= 30 && code <= 37) {
        tty_set_foreground_colour(tty, real_color);
    } else if(code >= 40 && code <= 47) {
        tty_set_background_colour(tty, real_color);
    } else if(code >= 90 && code <= 97) {
        tty_set_foreground_colour(tty, real_color | LIGHT_COLOUR);
    } else if(code >= 100 && code <= 107) {
        tty_set_background_colour(tty, real_color | LIGHT_COLOUR);
    }

end:
    return s + 1; // skip final 'm'
}

void tty_init(tty_t * tty) {
    tty_clean_buffer(tty);
    memset(tty->in_char_queue, 0, TTY_IN_CHAR_QUEUE_SIZE);
    tty->head_char_queue = -1;
    tty->tail_char_queue = -1;
    memset(tty->in_key_queue, 0, TTY_IN_KEY_QUEUE_SIZE);
    tty->head_key_queue = -1;
    tty->tail_key_queue = -1;
    tty->ankered = TTY_ANKERED;
}

void tty_set_foreground_colour(tty_t * tty, uint8_t colour) {
    tty->foregroup_colour = colour;
}

void tty_set_background_colour(tty_t * tty, uint8_t colour) {
    tty->backgroup_colour = colour << 4;
}

void tty_set_anker_state(tty_t * tty, uint8_t state) {
    tty->ankered = state;

    if (state == TTY_ANKERED) { /* there is a need to reset screen row */
        if (tty->row < SCREEN_ROWS) {
            tty->screen_row = 0;
        } else if (tty->row == SCREEN_ROWS) {
            tty->screen_row = tty->row - (tty->column != 0);
        } else { // tty->row > SCREEN_ROWS
            tty->screen_row = tty->row - SCREEN_ROWS + (tty->column != 0);
        }

        screen_flush_tty(tty); /* update the screen */
    }
}

void tty_set_screen_row(tty_t * tty, int32_t row) {
    if (row < 0) row = 0;
    else if (row >= 0 && row < SCREEN_BUFFER_ROWS - SCREEN_ROWS) tty->screen_row = row;
    else tty->screen_row = SCREEN_BUFFER_ROWS - SCREEN_ROWS;

    screen_flush_tty(tty); /* update the screen */
}

void tty_clean_buffer(tty_t * tty) {
    /* clear the buffer */
    tty->column = 0;
    tty->row = 0;
    tty->screen_row = 0;
    memset(tty->screen_text_buffer, 0, SCREEN_BUFFER_ROWS * SCREEN_BUFFER_COLUMNS);
    memset(tty->screen_colour_buffer, (BLACK_COLOUR << 4) | LIGHT_GRAY_COLOUR, 
        SCREEN_BUFFER_ROWS * SCREEN_BUFFER_COLUMNS);

    screen_flush_tty(tty); /* update the screen */
}

void tty_write_char(tty_t * tty, char c) {
    uint32_t relative_row = (tty->row - tty->screen_row) % SCREEN_BUFFER_ROWS;
    uint8_t print_char_to_screen = (relative_row < SCREEN_ROWS);
    uint8_t flush_screen = 0;
    uint32_t column_copy = tty->column;
    char colour = (BLACK_COLOUR << 4) | LIGHT_GRAY_COLOUR;
    
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
        tty->screen_text_buffer[tty->row][tty->column] = c;
        colour = tty->backgroup_colour | tty->foregroup_colour;
        tty->screen_colour_buffer[tty->row][tty->column] = colour;
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

    if (print_char_to_screen) screen_print_char(c, colour, relative_row, column_copy);
    if (flush_screen == 1) screen_flush_tty(tty);
}

void tty_write_string(tty_t * tty, char * str) {
    for (size_t i = 0; i < TTY_MAX_STRING_PRINT; i++) {
        if (str[i] == '\0') break;

        /* check if command starts here */
        if (str[i] == '\033' && i+1 < TTY_MAX_STRING_PRINT && str[i+1] == '[') {
            char * new_str = tty_parse_ansi_escape(tty, &str[i]);
            
            /* jump over the command */
            if (new_str != &str[i]) {
                i += (new_str - &str[i]) - 1; /* -1 because after the continue will do i++ */
                continue;
            }
        }

        tty_write_char(tty, str[i]);
    }
}


static uint8_t is_in_char_queue_empty(tty_t * tty) {
    return tty->head_char_queue == -1;
}

static uint8_t is_in_char_queue_full(tty_t * tty) {
    return (tty->tail_char_queue + 1) % TTY_IN_CHAR_QUEUE_SIZE == tty->head_char_queue;
}

static uint8_t is_in_key_queue_empty(tty_t * tty) {
    return tty->head_key_queue == -1;
}

static uint8_t is_in_key_queue_full(tty_t * tty) {
    return (tty->tail_key_queue + 1) % TTY_IN_KEY_QUEUE_SIZE == tty->head_key_queue;
}

void tty_putchar(tty_t * tty, char c) {
    if (is_in_char_queue_full(tty))
        tty_getchar(tty);  // the quere is full, so remove one element to clear spot

    if (is_in_char_queue_empty(tty)) {
        tty->head_char_queue = 0; // Initialize front for the first element
    }
    
    tty->tail_char_queue = (tty->tail_char_queue + 1) % TTY_IN_CHAR_QUEUE_SIZE;
    tty->in_char_queue[tty->tail_char_queue] = c;
}

char tty_getchar(tty_t * tty) {
    while (is_in_char_queue_empty(tty));  /* do nothing if queue is empty */

    char data = tty->in_char_queue[tty->head_char_queue];

    if (tty->head_char_queue == tty->tail_char_queue) { 
        // If queue becomes empty after getting this element, reset both pointers to -1
        tty->head_char_queue = -1;
        tty->tail_char_queue = -1;
    } else {
        // Use modulo to wrap the head index around
        tty->head_char_queue = (tty->head_char_queue + 1) % TTY_IN_CHAR_QUEUE_SIZE;
    }

    return data;
}

void tty_put_key_press(tty_t * tty, uint8_t key) {
    if (is_in_key_queue_full(tty))
        tty_get_key_press(tty);  // the quere is full, so remove one element to clear spot

    if (is_in_key_queue_empty(tty)) {
        tty->head_key_queue = 0; // Initialize front for the first element
    }

    tty->tail_key_queue = (tty->tail_key_queue + 1) % TTY_IN_KEY_QUEUE_SIZE;
    tty->in_key_queue[tty->tail_key_queue] = key;
}

uint8_t tty_get_key_press(tty_t * tty) {
    while (is_in_key_queue_empty(tty));  /* do nothing if key is empty */

    uint8_t data = tty->in_key_queue[tty->head_key_queue];

    if (tty->head_key_queue == tty->tail_key_queue) { 
        // If queue becomes empty after getting this element, reset both pointers to -1
        tty->head_key_queue = -1;
        tty->tail_key_queue = -1;
    } else {
        // Use modulo to wrap the head index around
        tty->head_key_queue = (tty->head_key_queue + 1) % TTY_IN_KEY_QUEUE_SIZE;
    }

    return data;
}