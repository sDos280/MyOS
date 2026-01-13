#include "kernel/print.h"

void idle_process_main() {
    // enable interrupts
    asm volatile ("sti");
    
    while (1)
        __asm__ __volatile__("hlt");
    
}