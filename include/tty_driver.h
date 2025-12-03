#ifndef TTY_DRIVER_H
#define TTY_DRIVER_H

#define SCREEN_COLUMNS 80
#define SCREEN_ROWS 25
#define ROW_FACTOR 4
#define SCREEN_BUFFER_COLUMNS 80
#define SCREEN_BUFFER_ROWS (SCREEN_ROWS * ROW_FACTOR)

#include "types.h"

typedef struct tty_struct {
    /* write position */
    uint32_t row;                           /* row of the next write position */
    uint32_t column;                        /* column of the next write position */
    uint32_t screen_row;                    /* the starting row of the buffer that the screen will print from */
    char buffer[SCREEN_BUFFER_ROWS]
          [SCREEN_BUFFER_COLUMNS];          /* the char buffer used to save the terminal char data */
} tty_t;

void tty_initialize(tty_t * tty);
void tty_clean_buffer(tty_t * tty);  /* write a char to the current write position */
void tty_write_char(tty_t * tty, char c); /* set the current */
uint8_t tty_is_next_write_in_screen_space();  /* 1 if the next write would affect the screen, else 0 */

#endif // TTY_DRIVER_H