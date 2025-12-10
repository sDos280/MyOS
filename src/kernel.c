#include "types.h"
#include "description_tables.h"
#include "print.h"
#include "timer.h"
#include "paging.h"
#include "multiboot.h"
#include "kheap.h"
#include "panic.h"
#include "keyboard_driver.h"
#include "multiboot_helper.h"
#include "ata_driver.h"
#include "utils.h"
#include "pmm.h"

// Entry point called by GRUB
void kernelMain(multiboot_info_t* multiboot_info_structure, uint32_t multiboot_magic)
{
    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        PANIC("Error: multiboot magic number unknown")
    }

    tty_t tty;
    tty_initialize(&tty);
    print_set_tty(&tty);

    initialize_gdt();

    print_clean_screen();
    printf("Hello, Kernel World!\n");

    initialize_idt();
    printf("IDT initialized.\n");

    initialize_timer(10); // Initialize timer to 10Hz
    printf("Timer initialized.\n");

    initialize_heap(); // initialize heap

    initialize_paging(); // init paging module
    identity_map_kernal();  // generate identity map and load table

    initialize_keyboard_driver();  // initialize the keyboard driver

    // enable interrupts
    asm volatile ("sti");

    initiate_ata_driver();  // initiate the driver

    pmm_init();  // initialize physical memory manager

    while (1);
}
