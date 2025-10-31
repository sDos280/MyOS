#include "types.h"
#include "description_tables.h"
#include "screen.h"
#include "timer.h"
#include "paging.h"
#include "multiboot.h"
#include "kheap.h"
#include "panic.h"

// Entry point called by GRUB
void kernelMain(multiboot_info_t* multiboot_info_structure, uint32_t multiboot_magic)
{
    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        PANIC("Error: multiboot magic number unknown")
    }
    
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

    kfree(first);

    print_heap_status();

    kfree(second);

    print_heap_status();

    first = kalloc(0x100);
    second = kalloc(0x100);

    print_heap_status();

    kfree(second);

    print_heap_status();
    
    kfree(first);

    print_heap_status();
    
    while (1);
}
