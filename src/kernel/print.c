#include "kernel/tty.h"
#include "kernel/print.h"
#include "kernel/print.h"
#include "arg.h"
#include "utils.h"

static char buf[TTY_MAX_STRING_PRINT+1]; /* temporary buffer for printf */
tty_t * ctty; /* current tty */

tty_t * print_get_tty() {
    return ctty;
}

void print_set_tty(tty_t * tty) {
    ctty = tty;
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
    va_list args;
    int i;

    memset(buf, 0, (TTY_MAX_STRING_PRINT + 1) * sizeof(char));
    
    va_start(args, format);
	i=vsprintf(buf, format,args);
	va_end(args);

    tty_write_string(ctty, buf);
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