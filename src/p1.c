#include "kernel/print.h"

void p1_main() {
    uint32_t i = 0;
    
    while (1) {
        print_int_padded(i, 0, ' ');
        print_char(' ');
        print_char('1');
        print_char('\n');

        i++;

        if (i >= 5000) break;
    }
}