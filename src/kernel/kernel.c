
#include "kernel/description_tables.h"
#include "kernel/print.h"
#include "kernel/timer.h"
#include "kernel/panic.h"
#include "mm/paging.h"
#include "mm/kheap.h"
#include "mm/pmm.h"
#include "drivers/keyboard_driver.h"
#include "drivers/ata_driver.h"
#include "multitasking/process.h"
#include "multitasking/scheduler.h"
#include "multiboot_helper.h"
#include "multiboot.h"
#include "utils.h"
#include "types.h"

void p1_main();
void p2_main();

void print_process_list(process_t *head);

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

    timer_init(1000); // Initialize timer to 1000Hz
    printf("Timer initialized.\n");
    
    keyboard_driver_init();  // initialize the keyboard driver
    printf("Keyboard driver initialized.\n");

    ata_driver_init();  // initiate the ata driver
    printf("Ata driver initialized.\n");
    
    process_t * p1 = process_create(PROCESS_KERNEL, p1_main, 0x100000);
    process_t * p2 = process_create(PROCESS_KERNEL, p2_main, 0x100000);

    scheduler_init();

    scheduler_add_processes_to_list(p1);
    scheduler_add_processes_to_list(p2);

    print_process_list(p1);

    /* we may want to call a schedule function here that will start 
       the first process scheduled*/
    scheduler_context_switch_asm(NULL, p1->esp);

    while (1);
}
