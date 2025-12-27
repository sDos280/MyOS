#ifndef TTY_DRIVER_H
#define TTY_DRIVER_H

#include "types.h"

#define SCREEN_COLUMNS 80
#define SCREEN_ROWS 25
#define ROW_FACTOR 6
#define SCREEN_BUFFER_COLUMNS 80
#define SCREEN_BUFFER_ROWS (SCREEN_ROWS * ROW_FACTOR)

#define TTY_ANKERED 0
#define TTY_NOT_ANKERED 1

#define TTY_IN_CHAR_QUEUE_SIZE 50
#define TTY_IN_KEY_QUEUE_SIZE TTY_IN_CHAR_QUEUE_SIZE

typedef struct tty_struct {
    /* write position */
    uint32_t row;                                /* row of the next write position */
    uint32_t column;                             /* column of the next write position */
    uint32_t screen_row;                         /* the starting row of the buffer that the screen will print from */
    char screeb_buffer[SCREEN_BUFFER_ROWS]
          [SCREEN_BUFFER_COLUMNS];               /* the char buffer used to save the terminal char data */
    char in_char_queue[TTY_IN_CHAR_QUEUE_SIZE];  /* a queue of the incoming chars */
    int32_t head_char_queue;                     /* the head of the in char queue */
    int32_t tail_char_queue;                     /* the tail of the in char queue */
    char in_key_queue[TTY_IN_KEY_QUEUE_SIZE];   /* a queue of the incoming chars */
    int32_t head_key_queue;                      /* the head of the in char queue */
    int32_t tail_key_queue;                      /* the tail of the in char queue */
    uint8_t ankered;                             /* the ankered state of the tty */
} tty_t;

void tty_init(tty_t * tty);
void tty_set_anker_state(tty_t * tty, uint8_t state);
void tty_set_screen_row(tty_t * tty, int32_t row);
void tty_clean_buffer(tty_t * tty);  /* write a char to the current write position */
void tty_write_char(tty_t * tty, char c); /* set the current */
void tty_putchar(tty_t * tty, char c);
char tty_getchar(tty_t * tty);
void tty_put_key_press(tty_t * tty, uint8_t key);
uint8_t tty_get_key_press(tty_t * tty);

#endif // TTY_DRIVER_H