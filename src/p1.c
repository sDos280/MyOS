#include "kernel/print.h"

void p1_main() {
    uint32_t i = 0;
    
    while (1) {
        printf("%d 1\n", i);

        i++;

        if (i >= 5000) break;
    }
}