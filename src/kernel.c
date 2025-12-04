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

char story[512] =
        "The old lighthouse keeper, Elias, polished the great lens as a storm "
        "raged outside. For forty years, the beam had cut through the fog and "
        "rain, a reliable sentinel on the jagged coast. Tonight, the wind "
        "howled with unusual fury, shaking the very foundation of the stone "
        "tower. A young apprentice, Sarah, watched him, her hands clenched "
        "tightly. 'Will it hold, Elias?' she shouted over the gale. He nodded, "
        "a calm smile beneath his grizzled beard. 'It always has.' A sudden "
        "crack of thunder shook the ground. Outside, the sea crashed against "
        "the cliffs, hungry and dark. The light flickered, a brief heart "
        "tremor, before steadying. Inside, a sense of duty stronger than the "
        "storm prevailed. The light was their promise to the distant ships, "
        "unbroken and eternal. Sarah finally relaxed, understanding the quiet "
        "strength of the man beside her. The night wore on, a battle of "
        "elements against a stubborn beacon of hope. By dawn, the storm had "
        "passed, leaving a crisp, clear sky and a calm sea. Elias wiped a "
        "final speck of salt spray from the glass, satisfied. The beacon "
        "remained a silent testament to endurance. The cycle would continue, "
        "one generation teaching the next the simple, vital truth: some things "
        "must never fail. The sea always respected that resolve in the end.";

char buffer[ATA_SECTOR_SIZE + 1];

extern uint32_t heap_start;
extern uint32_t heap_end;

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

    ata_drive_t drive_a;
    drive_a.device_id.io_base = ATA_PRIMARY_IO;
    drive_a.device_id.ctrl_base = ATA_PRIMARY_CTRL;
    drive_a.exists = 0;
    drive_a.size_in_sectors = 0;

    identify_device_data_t drive_a_identify;
    memset(&drive_a_identify, 0, sizeof(identify_device_data_t));
    
    if (ata_send_identify_command(&drive_a, &drive_a_identify) == 0) {
        print_identify_device_data(&drive_a_identify);

        ata_write28_request(&drive_a, 2, 1, story);
        ata_write28_request(&drive_a, 3, 1, story);
        ata_flush_cache(&drive_a);

        ata_read28_request(&drive_a, 2, 1, buffer);

        printf("%c\n", buffer[0]);
        print_hexdump(story, ATA_SECTOR_SIZE);
    }

    while (1) {
        char c = getc();

        if (c == 'm') {
            tty_set_anker_state(&tty, !tty.ankered);
            printf("Tty state: %s\n", (tty.ankered)? "ankered" : "not ankered");
        } else printf("char: %c\n", c);
    }
}
