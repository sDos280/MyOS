#include "screen.h"
#include "types.h"
#include "utils.h"
#include "arg.h"

#define SCREEN_COLUMNS 80
#define SCREEN_ROWS 25
#define ROW_FACTOR 4
#define SCREEN_BUFFER_COLUMNS 80
#define SCREEN_BUFFER_ROWS (SCREEN_ROWS * ROW_FACTOR)


volatile char *video = (volatile char*)0xB8000;
static uint32_t screen_start_row;
// Note: the [row, column] is index that is next to be writen
static uint32_t row = 0;
static uint32_t column = 0;

char screen_buffer[SCREEN_BUFFER_ROWS * SCREEN_BUFFER_COLUMNS * 2];

void initialize_screen() {
    clear_screen();
}

static void blip_to_screen() {
    for (uint32_t y = 0; y < SCREEN_ROWS; y++) {
        for (uint32_t x = 0; x < SCREEN_COLUMNS; x++) {
            video[(y * SCREEN_COLUMNS + x) * 2] = 
                screen_buffer[(((y + screen_start_row) % SCREEN_BUFFER_ROWS) * SCREEN_BUFFER_COLUMNS + x) * 2];
            video[(y * SCREEN_COLUMNS + x) * 2 + 1] = 
                screen_buffer[(((y + screen_start_row) % SCREEN_BUFFER_ROWS) * SCREEN_BUFFER_COLUMNS + x) * 2 + 1];
        }
    }
}

void clear_screen() {
    // reset screen and buffer parameters
    row = 0;
    column = 0;
    screen_start_row = 0;

    for (uint32_t r = 0; r < SCREEN_BUFFER_ROWS; r++) {
        for (uint32_t c = 0; c < SCREEN_BUFFER_COLUMNS; c++) {
            screen_buffer[(r * SCREEN_BUFFER_COLUMNS + c) * 2] = 0;
            screen_buffer[(r * SCREEN_BUFFER_COLUMNS + c) * 2 + 1] = 0x07; // Light grey on black background
        }
    }

    blip_to_screen();
}

void print_char(char c) {
    // write and update the buffer
    // uint8_t is_write_over_screen;

    if (c == '\n') {
        column = 0;
        row++;
    } else {
        screen_buffer[(row  * SCREEN_BUFFER_COLUMNS + column) * 2] = c;
        screen_buffer[(row  * SCREEN_BUFFER_COLUMNS + column) * 2 + 1] = 0x07; // Light grey on black background

        column++;
    }
    
    if (column >= SCREEN_BUFFER_COLUMNS) {
        column = 0;
        row++;
    }

    if (row >= SCREEN_BUFFER_ROWS) 
        clear_screen();
    
    // check if next write will be over the screen
    if ((row - screen_start_row) % SCREEN_BUFFER_ROWS >= SCREEN_ROWS) {
        screen_start_row = (screen_start_row + 1) % SCREEN_BUFFER_ROWS;
    }

    blip_to_screen();
}

void printf(const char* format, ...) {
    // https://medium.com/@turman1701/va-list-in-c-exploring-ft-printf-bb2a19fcd128
    va_list args; // Declare a va_list variable to manage the variable arguments

    // Initialize the va_list 'args' to start at the argument after 'format'
    va_start(args, format);
 
    while (*format)  {// Loop through the format string
        // If a format specifier is encountered
        if (*format != '%') {
            // Print regular characters
            print_char(*format);
        } else {
            format++;
        
            if (*format == 'd') {
                // Fetch the next argument as an integer and print it
                print_int(va_arg(args, int));
            }

            else if (*format == 'x' || *format == 'X') {
                // Fetch the next argument as an integer and print it
                print_hex(va_arg(args, uint32_t));
            }

            else if (*format == 's') {
                // Fetch the next argument as a string and print it
                print_const_string(va_arg(args, char *));
            }
        }

        format++;
    }
 
    va_end(args);
}

void print_const_string(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

void print_int(uint32_t num){
    if(num == 0){
        print_const_string("0");
        return;
    }

    char buffer[11]; // Enough to hold all digits of a 32-bit integer
    int i = 0;

    while(num > 0){
        buffer[i++] = (num % 10) + '0'; // Convert digit to character
        num /= 10;
    }

    // Digits are in reverse order, so print them backwards
    for(int j = i - 1; j >= 0; j--){
        char str[2] = {buffer[j], '\0'};
        print_const_string(str);
    }
}

void print_hex(uint32_t num) {
    print_const_string("0x");
    if (num == 0) {
        print_const_string("0");
        return;
    }

    char buffer[9]; // Enough to hold all hex digits of a 32-bit integer
    int i = 0;

    while (num > 0) {
        uint32_t digit = num & 0xF; // Get the last hex digit
        if (digit < 10) {
            buffer[i++] = digit + '0'; // Convert to character '0'-'9'
        } else {
            buffer[i++] = digit - 10 + 'A'; // Convert to character 'A'-'F'
        }
        num >>= 4; // Shift right by 4 bits to process the next hex digit
    }

    // Digits are in reverse order, so print them backwards
    for (int j = i - 1; j >= 0; j--) {
        char str[2] = {buffer[j], '\0'};
        print_const_string(str);
    }
}