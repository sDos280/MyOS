#include "screen.h"
#include "types.h"
#include "utils.h"
#include "arg.h"

static void blip_to_screen();

static volatile char *video = (volatile char*)0xB8000;
static screen_handler_t * screen_handler;


static void blip_to_screen() {
    for (uint32_t y = 0; y < SCREEN_ROWS; y++) {
        for (uint32_t x = 0; x < SCREEN_COLUMNS; x++) {
            video[(y * SCREEN_COLUMNS + x) * 2] = 
                screen_handler->screen_buffer[(((y + screen_handler->screen_start_row) % SCREEN_BUFFER_ROWS) * SCREEN_BUFFER_COLUMNS + x) * 2];
            video[(y * SCREEN_COLUMNS + x) * 2 + 1] = 
                screen_handler->screen_buffer[(((y + screen_handler->screen_start_row) % SCREEN_BUFFER_ROWS) * SCREEN_BUFFER_COLUMNS + x) * 2 + 1];
        }
    }
}

void assign_screen_handler(screen_handler_t * handler) {
    screen_handler = handler;
}

void initialize_screen_handler() {
    clear_screen();
}

static void clear_row_of_buffer(char * buffer, uint32_t row) {
    for (uint32_t c = 0; c < SCREEN_BUFFER_COLUMNS; c++) {
        screen_handler->screen_buffer[(row * SCREEN_BUFFER_COLUMNS + c) * 2] = 0;
        screen_handler->screen_buffer[(row * SCREEN_BUFFER_COLUMNS + c) * 2 + 1] = 0x07; // Light grey on black background
    }
}

void clear_screen() {
    // reset screen and buffer parameters
    screen_handler->row = 0;
    screen_handler->column = 0;
    screen_handler->screen_start_row = 0;

    for (uint32_t r = 0; r < SCREEN_BUFFER_ROWS; r++) {
        clear_row_of_buffer(screen_handler->screen_buffer, r);
    }

    blip_to_screen();
}

void print_char(char c) {
    uint32_t relative_row = (screen_handler->row - screen_handler->screen_start_row) % SCREEN_BUFFER_ROWS;
    uint32_t should_print_to_screen = (relative_row < SCREEN_ROWS);

    // return 1 if the is there is need to blit to the screen after the call else 1
    uint8_t return_num = 0;

    if (c == '\n') {
        screen_handler->column = 0;
        screen_handler->row++;
    } else {
        screen_handler->screen_buffer[(screen_handler->row  * SCREEN_BUFFER_COLUMNS + screen_handler->column) * 2] = c;
        screen_handler->screen_buffer[(screen_handler->row  * SCREEN_BUFFER_COLUMNS + screen_handler->column) * 2 + 1] = 0x07; // Light grey on black background

        if (should_print_to_screen) {
            video[(relative_row * SCREEN_COLUMNS + screen_handler->column) * 2] = c;
            video[(relative_row * SCREEN_COLUMNS + screen_handler->column) * 2 + 1] = 0x07; // Light grey on black background
        }

        screen_handler->column++;
    }
    
    if (screen_handler->column >= SCREEN_BUFFER_COLUMNS) {
        screen_handler->column = 0;
        screen_handler->row++;
    }

    if (screen_handler->row >= SCREEN_BUFFER_ROWS) 
    {
        clear_screen(); // a new screen
    }
    
    // check if next write will be over the screen
    if ((screen_handler->row - screen_handler->screen_start_row) % SCREEN_BUFFER_ROWS >= SCREEN_ROWS) {
        screen_handler->screen_start_row = (screen_handler->screen_start_row + 1) % SCREEN_BUFFER_ROWS;
        should_print_to_screen = 1;
    }

    if (should_print_to_screen == 1) blip_to_screen();
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

            // --- Parse padding and width ---
            char pad_char = ' ';
            int width = 0;

            if (*format == '0') {
                pad_char = '0';
                format++;
            }

            // Read numeric width (e.g. "08" -> 8)
            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }

            // --- Handle specifier ---
            if (*format == 'd' || *format == 'u') {
                print_int_padded(va_arg(args, int), width, pad_char);
            }
            else if (*format == 'x' || *format == 'p') {
                print_hex_padded(va_arg(args, uint32_t), width, pad_char);
            }
            else if (*format == 'c') {
                print_char(va_arg(args, char));
            }
            else if (*format == 's') {
                print_const_string(va_arg(args, char*));
            }
            else {
                print_char('%');
                print_char(*format);
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

void print_int_padded(uint32_t num, int width, char pad_char) {
    char buffer[11];
    int i = 0;

    if (num == 0) {
        buffer[i++] = '0';
    } else {
        while (num > 0) {
            buffer[i++] = (num % 10) + '0';
            num /= 10;
        }
    }

    // Print padding if needed
    while (i < width) {
        print_char(pad_char);
        width--;
    }

    // Print digits in reverse
    for (int j = i - 1; j >= 0; j--) {
        print_char(buffer[j]);
    }
}

void print_hex_padded(uint32_t num, int width, char pad_char) {
    char buffer[9];
    int i = 0;

    if (num == 0) {
        buffer[i++] = '0';
    } else {
        while (num > 0) {
            uint32_t digit = num & 0xF;
            buffer[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
            num >>= 4;
        }
    }

    int num_len = i;
    while (num_len < width) {
        print_char(pad_char);
        width--;
    }

    for (int j = i - 1; j >= 0; j--) {
        print_char(buffer[j]);
    }
}


void print_hexdump(const void *data, size_t size) {
    #define BYTES_PER_ROW 16
    const uint8_t *bytes = (const uint8_t *)data;

    for (size_t offset = 0; offset < size; offset += BYTES_PER_ROW) {
        // Print offset
        printf("%08x: ", (unsigned int)offset);

        // Print hex values
        for (size_t i = 0; i < BYTES_PER_ROW; i++) {
            if (offset + i < size)
                printf("%02x ", bytes[offset + i]);
            else
                printf("   "); // keep columns aligned if incomplete line
        }

        printf(" ");

        // Print ASCII representation
        for (size_t i = 0; i < BYTES_PER_ROW; i++) {
            if (offset + i < size) {
                uint8_t c = bytes[offset + i];
                if (c >= 32 && c < 127) // printable ASCII range
                    printf("%c", c);
                else
                    printf(".");
            }
        }

        printf("\n");
    }
}