#include "screen.h"
#include "types.h"

volatile char *video = (volatile char*)0xB8000;
static uint32_t x = 0;
static uint32_t y = 0;
void clear_screen() {
    x = 0;
    y = 0;
    for(int i = 0; i < 80 * 25 * 2; i++) {
        video[i] = 0;
    }
}

void print(const char* str) {
    while (*str) {
        if (*str == '\n') {
            x = 0;
            y++;
        } else {
            video[(y * 80 + x) * 2] = *str;
            video[(y * 80 + x) * 2 + 1] = 0x07; // Light grey on black background
            x++;
            if (x >= 80) {
                x = 0;
                y++;
            }

            if (y >= 25) {
                clear_screen();
                x = 0;
                y = 0;
            }
        }
        
        str++;
    }
}

void print_int(uint32_t num){
    if(num == 0){
        print("0");
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
        print(str);
    }
}

void print_hex(uint32_t num) {
    print("0x");
    if (num == 0) {
        print("0");
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
        print(str);
    }
}