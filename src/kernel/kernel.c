
#include "kernel/description_tables.h"
#include "kernel/early_print.h"
#include "kernel/print.h"
#include "kernel/screen.h"
#include "kernel/timer.h"
#include "kernel/panic.h"
#include "kernel/syscall.h"
#include "mm/paging.h"
#include "mm/kheap.h"
#include "mm/pmm.h"
#include "drivers/flatfs/flatfs_driver.h"
#include "drivers/keyboard_driver.h"
#include "drivers/ata_driver.h"
#include "multitasking/process.h"
#include "multitasking/scheduler.h"
#include "tests/ata_test.h"
#include "tests/flatfs_test.h"
#include "tests/heap_test.h"
#include "multiboot_info.h"
#include "multiboot.h"
#include "utils/utils.h"
#include "types.h"

void p1_main();
void p2_main();

void print_process_list(process_t *head);

multiboot_info_t multiboot_info;
tty_t tty;
ata_drive_t drive_prime_master;
uint8_t temp_buffer[50 * sizeof(event_t)];

void tty_event_handler_func_wrapper(event_t e) {
    console_handle_event(&tty.console, e);
}

// Entry point called by GRUB
void kernel_main(multiboot_info_t* lower_multiboot_info_structure, uint32_t multiboot_magic) {
    early_print_init();

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        early_printf("KERNEL PANIC: %s at %s:%d in %s()\n",          
               "Error: multiboot magic number unknown", 
               __FILE__, 
               __LINE__, 
               __func__);         
        while (1)__asm__ __volatile__("hlt");                      
        
    }

    /* remember, when we get here, lower half of the kernel is still mapped*/
    /* there is a need to copy that before paging */
    memcpy(&multiboot_info, lower_multiboot_info_structure, sizeof(multiboot_info_t));

    multiboot_info_print(multiboot_magic, lower_multiboot_info_structure);

    pmm_init();  // initialize physical memory manager (will be needed for invalidation of invalide memory address)
    early_printf("Physical memory initialized.\n");

    multiboot_info_invalidate_unavailable_memory(lower_multiboot_info_structure);

    gdt_init();
    early_printf("GDT initialized.\n");

    idt_init();
    early_printf("IDT initialized.\n");

    paging_init(); // init paging module
    early_printf("Paging initialized.\n");

    heap_init();  // Initialize heap module
    early_printf("Heap initialized.\n");

    timer_init(1000); // Initialize timer to 1000Hz
    early_printf("Timer initialized.\n");
    
    syscall_init();  // initialize the syscall module 
    early_printf("Syscall module  initialized.\n");

    keyboard_driver_init();  // initialize the keyboard driver
    early_printf("Keyboard driver initialized.\n");

    ata_driver_init();  // initiate the ata driver
    early_printf("Ata driver initialized.\n");

    scheduler_init(); // initialize the scheduler
    early_printf("Scheduler initialized.\n");
    
    asm volatile ("sti"); // enable interrupts
    
    tty_init(&tty, tty_event_handler_func_wrapper); /* initialize tty again after all modules initialized (heap is now initizlied)*/
    print_set_tty(&tty);
    
    /* setup information on the first primery master drive */
    identify_device_data_t identify_buf;
    ata_drive_init(&drive_prime_master, ATA_PRIMARY_IO, ATA_PRIMARY_CTRL, ATA_MASTER_DRIVE);

    uint8_t _ = ata_send_identify_command(&drive_prime_master, &identify_buf);
    if (_ == 1) PANIC("ATA identify error");
    drive_prime_master.exists = 1;
    
    print_identify_device_data(&identify_buf);
    drive_prime_master.size_in_sectors = identify_buf.UserAddressableSectors;
    

    /* test modules */
    ata_test_write_read_3_sectors(&drive_prime_master, 50);
    flatfs_test_basic(&drive_prime_master);
    
    heap_test_basic();
    heap_test_many_small_allocs();

    /*process_t * p1 = process_create(PROCESS_KERNEL, p1_main, 0x10000);
    process_t * p2 = process_create(PROCESS_KERNEL, p2_main, 0x10000);

    scheduler_add_process_to_ready_queue(p1);
    scheduler_add_process_to_ready_queue(p2);

    print_process_list(p1);

    scheduler_set_on();*/


    while (1);
}
