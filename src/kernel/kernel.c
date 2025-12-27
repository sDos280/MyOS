
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
    tty_initialize(&tty);
    print_set_tty(&tty);

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        PANIC("Error: multiboot magic number unknown");
    }

    initialize_gdt();

    memcpy(&multiboot_info_structure, lower_multiboot_info_structure, sizeof(multiboot_info_t));

    print_clean_screen();
    printf("Hello, Kernel World!\n");

    initialize_idt();
    printf("IDT initialized.\n");

    pmm_init();  // initialize physical memory manager
    paging_init(); // init paging module

    initialize_timer(10); // Initialize timer to 10Hz
    printf("Timer initialized.\n");
    
    initialize_keyboard_driver();  // initialize the keyboard driver

    // enable interrupts
    asm volatile ("sti");

    initiate_ata_driver();  // initiate the ata driver
    
    while (1);
}
