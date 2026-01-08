#include "kernel/print.h"

void idle_process_main() {

    while (1)
        __asm__ __volatile__("hlt");
    
}