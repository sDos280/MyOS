#ifndef PANIC_H
#define PANIC_H

#include "screen.h"

#define PANIC(message) {print_const_string(message);while(1){__asm__ __volatile("hlt");}};

#endif // PANIC_H