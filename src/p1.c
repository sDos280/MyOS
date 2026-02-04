#include "kernel/print.h"
#include "kernel/syscall.h"

void p1_main() {
    uint32_t i = 0;
    
    void *addr = mmap(0x40000000, 0x1000, PPROT_WRITE);

    int *ptr = (int*)addr;

    ptr[0] = 42;

    while (1) {
        printf("%d 1\n", i);

        i++;

        if (i >= 5000) break;
    }
}