#include "types.h"
#include "description_tables.h"
#include "screen.h"
#include "timer.h"
#include "paging.h"
#include "kheap.h"

// Entry point called by GRUB
void kernelMain(const void* multiboot_structure, uint32_t multiboot_magic)
{
    initialize_gdt();
    clear_screen();
    print("Hello, Kernel World!\n");

    initialize_idt();
    print("IDT initialized.\n");

    initialize_timer(50); // Initialize timer to 50Hz
    print("Timer initialized.\n");
    
    initialize_paging(); // init paging module
    identity_map_kernal();  // generate identity map and load table

    // enable interrupts
    //asm volatile ("sti");

    initialize_heap();
    void* first = kalloc(4);
    void* second = kalloc(4);
    print_heap_status();

    while (1);
}
