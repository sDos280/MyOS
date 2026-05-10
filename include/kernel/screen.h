#ifndef SCREEN_H
#define SCREEN_H
#include "types.h"

#define SCREEN_COLUMNS 80
#define SCREEN_ROWS 25

/* buffers should be of size rows*columns,
   the print will start at start_row, start_column
   and will flush screen size to screen */
void screen_flush_text_and_colour_buffer(char *text_buffer, 
                                         char *colour_buffer, 
                                         uint32_t start_row, 
                                         uint32_t start_column, 
                                         uint32_t rows, 
                                         uint32_t columns);
void screen_print_char(char c, char colour, uint32_t row, uint32_t column);

#endif // SCREEN_H