
#include "kernel/description_tables.h"
#include "kernel/print.h"
#include "kernel/timer.h"
#include "kernel/panic.h"
#include "mm/paging.h"
#include "mm/kheap.h"
#include "mm/pmm.h"
#include "drivers/keyboard_driver.h"
#include "drivers/ata_driver.h"
#include "multiboot_helper.h"
#include "multiboot.h"
#include "utils.h"
#include "types.h"

// Entry point called by GRUB
void kernel_main(multiboot_info_t* lower_multiboot_info_structure, uint32_t multiboot_magic) {
    multiboot_info_t multiboot_info_structure;
    tty_t tty;
    tty_init(&tty);
    print_set_tty(&tty);

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        PANIC("Error: multiboot magic number unknown");
    }

    gdt_init();
    printf("GDT initialized.\n");

    /* there is a need to copy that before paging */
    memcpy(&multiboot_info_structure, lower_multiboot_info_structure, sizeof(multiboot_info_t));

    idt_init();
    printf("IDT initialized.\n");

    pmm_init();  // initialize physical memory manager
    printf("Physical memory initialized.\n");

    paging_init(); // init paging module
    printf("Paging initialized.\n");

    heap_init();  // Initialize heap module
    printf("Heap initialized.\n");

    timer_init(10); // Initialize timer to 10Hz
    printf("Timer initialized.\n");

    
    keyboard_driver_init();  // initialize the keyboard driver
    printf("Keyboard driver initialized.\n");

    // enable interrupts
    asm volatile ("sti");

    ata_driver_init();  // initiate the ata driver
    printf("Ata driver initialized.\n");
    
    printf("\n\n=== kalloc/kfree BASIC test ===\n\n");

    print_heap_status();

    void* a = kalloc(64);
    void* b = kalloc(128); 
    void* c = kalloc(256);

    printf("\nAllocated a=%p b=%p c=%p\n\n", a, b, c);
    print_heap_status();

    kfree(b);
    printf("\nkfreed b\n");
    print_heap_status();

    kfree(a);
    printf("\nkfreed a\n\n");
    print_heap_status();

    kfree(c);
    printf("\nkfreed c\n\n");
    print_heap_status();

    printf("\n=== BASIC test done ===\n");

    while (1) {
        uint8_t key = get_key_press();
        if (tty.ankered == TTY_NOT_ANKERED) 
            if (key == KEY_ARROW_UP) 
                tty_set_screen_row(&tty, tty.screen_row - 1);
            else if (key == KEY_ARROW_DOWN) 
                tty_set_screen_row(&tty, tty.screen_row + 1);
        
        if (key == KEY_LSHIFT) 
            tty_set_anker_state(&tty, !tty.ankered);
    }
}
