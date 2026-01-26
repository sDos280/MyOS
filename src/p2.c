#include "kernel/print.h"

void p2_main() {
    uint32_t i = 0;
    
    while (1) {
        printf("%d 2\n", i);

        i++;

        if (i >= 5000) break;
    }
}