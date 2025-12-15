#ifndef PANIC_H
#define PANIC_H

#include "kernel/print.h"

#define PANIC(message)                                         \
    do {                                                        \
        printf("KERNEL PANIC: %s at %s:%d in %s()\n",          \
               message, __FILE__, __LINE__, __func__);         \
        while (1) {                                            \
            __asm__ __volatile__("hlt");                       \
        }                                                       \
    } while (0)

#endif // PANIC_H