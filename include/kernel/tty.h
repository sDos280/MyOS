#ifndef TTY_DRIVER_H
#define TTY_DRIVER_H

#include "drivers/keys.h"
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

#define TTY_MAX_STRING_PRINT 1000

#define BLACK_COLOUR          0x00
#define BLUE_COLOUR           0x01
#define GREEN_COLOUR          0x02
#define CYAN_COLOUR           0x03
#define RED_COLOUR            0x04
#define MEGENTA_COLOUR        0x05
#define BROWN_COLOUR          0x06
#define LIGHT_GRAY_COLOUR     0x07
#define DARK_GRAY_COLOUR      0x08
#define LIGHT_BLUE_COLOUR     0x09
#define LIGHT_GREEN_COLOUR    0x0A
#define LIGHT_CYAN_COLOUR     0x0B
#define LIGHT_RED_COLOUR      0x0C
#define LIGHT_MEGENTA_COLOUR  0x0D
#define YELOW_COLOUR          0x0E
#define WHITE_GRAY_COLOUR     0x0F

#define LIGHT_COLOUR          0b1000

typedef struct tty_struct {
    /* write position */
    uint32_t row;                                /* row of the next write position */
    uint32_t column;                             /* column of the next write position */
    uint32_t screen_row;                         /* the starting row of the buffer that the screen will print from */
    char screen_text_buffer[SCREEN_BUFFER_ROWS]
          [SCREEN_BUFFER_COLUMNS];               /* the text buffer used to save the terminal char data */
    char screen_colour_buffer[SCREEN_BUFFER_ROWS]
          [SCREEN_BUFFER_COLUMNS];               /* the colour buffer used to save the terminal colour data */
    char foregroup_colour;                       /* the current foreground colour */
    char backgroup_colour;                       /* the current background colour */
    char in_char_queue[TTY_IN_CHAR_QUEUE_SIZE];  /* a queue of the incoming chars */
    int32_t head_char_queue;                     /* the head of the in char queue */
    int32_t tail_char_queue;                     /* the tail of the in char queue */
    char in_key_queue[TTY_IN_KEY_QUEUE_SIZE];    /* a queue of the incoming chars */
    int32_t head_key_queue;                      /* the head of the in char queue */
    int32_t tail_key_queue;                      /* the tail of the in char queue */
    uint8_t ankered;                             /* the ankered state of the tty */
} tty_t;

void tty_init(tty_t * tty);
void tty_set_foreground_colour(tty_t * tty, uint8_t colour);
void tty_set_background_colour(tty_t * tty, uint8_t colour);
void tty_set_anker_state(tty_t * tty, uint8_t state);
void tty_set_screen_row(tty_t * tty, int32_t row);
void tty_clean_buffer(tty_t * tty); /* clear both buffers, doesn't change colors */
void tty_write_char(tty_t * tty, char c); /* set the current */
void tty_write_string(tty_t * tty, char * str); /* write a string, max number of chars to print is TTY_MAX_STRING_PRINT */
void tty_putchar(tty_t * tty, char c);
char tty_getchar(tty_t * tty);
void tty_put_key_press(tty_t * tty, uint8_t key);
uint8_t tty_get_key_press(tty_t * tty);

#endif // TTY_DRIVER_H