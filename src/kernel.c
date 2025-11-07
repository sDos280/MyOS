#include "types.h"
#include "description_tables.h"
#include "screen.h"
#include "timer.h"
#include "paging.h"
#include "multiboot.h"
#include "kheap.h"
#include "panic.h"
#include "keyboard_driver.h"
#include "multiboot_helper.h"
#include "ata_driver.h"
#include "utils.h"

// Entry point called by GRUB
void kernelMain(multiboot_info_t* multiboot_info_structure, uint32_t multiboot_magic)
{
    identify_device_data_t * a_drive = NULL;

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        PANIC("Error: multiboot magic number unknown")
    }
    
    initialize_gdt();

    initialize_screen();  // initialize the screen
    
    clear_screen();
    printf("Hello, Kernel World!\n");

    initialize_idt();
    printf("IDT initialized.\n");

    initialize_timer(50); // Initialize timer to 50Hz
    printf("Timer initialized.\n");
    
    initialize_paging(); // init paging module
    identity_map_kernal();  // generate identity map and load table

    initialize_keyboard_driver();  // initialize the keyboard driver

    initialize_heap(); // initialize heap

    // enable interrupts
    asm volatile ("sti");

    initiate_ata_driver();
    if (ata_send_identify_command(ATA_PRIMARY, ATA_MASTER_DRIVE) == 0) {
        ata_responce_t * last_ata_responce = get_ata_responce_structure();
        a_drive = kalloc(sizeof(identify_device_data_t));
        memcpy(a_drive, last_ata_responce->data, sizeof(identify_device_data_t));
    }

    kfree(a_drive);

    while (1);
}
