
#include "kernel/description_tables.h"
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
    ata_drive_t drive_prime_master;

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
    
    syscall_init();  // initialize the syscall module 

    keyboard_driver_init();  // initialize the keyboard driver
    printf("Keyboard driver initialized.\n");

    ata_driver_init();  // initiate the ata driver
    printf("Ata driver initialized.\n");

    scheduler_init(); // initialize the scheduler
    printf("Scheduler initialized.\n");
    
    asm volatile ("sti"); // enable interrupts

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

    process_t * p1 = process_create(PROCESS_KERNEL, p1_main, 0x10000);
    process_t * p2 = process_create(PROCESS_KERNEL, p2_main, 0x10000);

    scheduler_add_process_to_ready_queue(p1);
    scheduler_add_process_to_ready_queue(p2);

    print_process_list(p1);

    scheduler_set_on();


    while (1);
}
