#include "kernel/tty.h"
#include "kernel/print.h"
#include "arg.h"

tty_t * ctty; /* current tty */

tty_t * print_get_tty() {
    return ctty;
}

void print_set_tty(tty_t * tty) {
    ctty = tty;
}

void print_char(char c) {
    tty_write_char(ctty, c);
}

void print_clean_screen() {
    tty_clean_buffer(ctty);
}

void putchar(char c) {
    tty_putchar(ctty, c);
}

char getchar() {
    return tty_getchar(ctty);
}

void put_key_press(uint8_t key) {
    tty_put_key_press(ctty, key);
}

uint8_t get_key_press() {
    return tty_get_key_press(ctty);
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