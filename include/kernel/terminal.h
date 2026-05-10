#ifndef TERMINAL_H
#define TERMINAL_H

#include "screen.h"
#include "types.h"

#define TERMINAL_ROW_FACTOR 6
#define TERMINAL_COLUMNS SCREEN_COLUMNS
#define TERMINAL_ROWS (SCREEN_ROWS * TERMINAL_ROW_FACTOR)

#define TERMINAL_MAX_STRING_PRINT 1000

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

#define TERMINAL_ANKERED 0
#define TERMINAL_NOT_ANKERED 1


typedef struct terminal_struct {
    /* write position */
    uint32_t row;                                /* row of the next write position */
    uint32_t column;                             /* column of the next write position */
    uint32_t screen_row;                         /* the starting row of the buffer that the screen will print from */
    char text_buffer[TERMINAL_ROWS]
          [TERMINAL_COLUMNS];             /* the text buffer used to save the terminal char data */
    char colour_buffer[TERMINAL_ROWS]
          [TERMINAL_COLUMNS];             /* the colour buffer used to save the terminal colour data */
    char foregroup_colour;                       /* the current foreground colour */
    char backgroup_colour;                       /* the current background colour */
    uint8_t ankered;                             /* the ankered state of the tty */
} terminal_t;

void terminal_init(terminal_t *terminal);
void terminal_set_foreground_colour(terminal_t *terminal, uint8_t colour);
void terminal_set_background_colour(terminal_t *terminal, uint8_t colour);
void terminal_set_anker_state(terminal_t *terminal, uint8_t state);
void terminal_set_screen_row(terminal_t *terminal, int32_t row);
void terminal_clean_buffer(terminal_t *terminal); /* clear both buffers, doesn't change colors */
void terminal_write_char(terminal_t *terminal, char c); 
void terminal_write_string(terminal_t *terminal, char * str); /* write a string, max number of chars to print is TERMINAL_MAX_STRING_PRINT */

#endif // TERMINAL_BUFFER_H
