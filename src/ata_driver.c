#include "ata_driver.h"
#include "port.h"
#include "screen.h"
#include "utils.h"
#include "pic.h"

static void print_identify_device_data(const ATA_IDENTIFY_DEVICE_DATA* id);

ata_request_t ata_request;

void initiate_ata_driver() {
    ata_request.busy = 0;
    ata_request.done = 0;
    ata_request.request_type = 0;

    memset((void *)ata_request.data, 0, sizeof(ata_request.data));

    register_interrupt_handler(46, ata_response_handler);
}

ata_request_t * get_ata_requast_struct() {
    return &ata_request;
}

void ata_send_identify_command(uint8_t channel, uint8_t device) {
    // https://wiki.osdev.org/ATA_PIO_Mode#IDENTIFY_command

    if (channel == ATA_PRIMARY) {
        ata_request.busy = 1; // tell that there is a request on the way 

        ata_request.request_type = ATA_IDENTIFY_REQUEST;

        outb(ATA_PRIMARY_DRIVE_HEAD_REGISTER, 0xa0 | (device << 4)); // spesify what driver to use (0xa0 = 0b10100000 this needs to be always on)
        // set Sector count, LBAlo, LBAmid and LBAhigh to 0
        outb(ATA_PRIMARY_SECTOR_COUNT_REGISTER, 0);
        outb(ATA_PRIMARY_SECTOR_NUMBER_REGISTER, 0);
        outb(ATA_PRIMARY_CYLINDER_LOW_REGISTER, 0);
        outb(ATA_PRIMARY_CYLINDER_HIGH_REGISTER, 0);
        // send IDENTIFY command (0xEC)
        outb(ATA_PRIMARY_COMMAND_REGISTER, 0xEC);
        
        

        while (ata_request.done != 1) {
            asm volatile("hlt"); // wait for the irq to finish working
        }
        
        print_identify_device_data((ATA_IDENTIFY_DEVICE_DATA *)ata_request.data);

    }
}

void ata_response_handler(registers_t* regs) {
    if (ata_request.busy != 1) {
        printf("Something went wrong, there is no pending ata request\n");
        goto return_and_send_eoi;
    }

    if (ata_request.request_type == ATA_IDENTIFY_REQUEST) {
        uint8_t status = inb(ATA_PRIMARY_STATUS_REGISTER);
        uint8_t LBAmid = inb(ATA_PRIMARY_CYLINDER_LOW_REGISTER);
        uint8_t LBAhi  = inb(ATA_PRIMARY_CYLINDER_HIGH_REGISTER);

        if (status != 0) {
            printf("A driver was found\n");
        } else {
            printf("A driver wasn't found\n");
        }
        
        if (LBAmid != 0 || LBAhi != 0) {
            printf("The driver isn't an ATA one\n");
        }

        do {
            status = inb(ATA_PRIMARY_STATUS_REGISTER); 
        } while (!((status & 0b00001000) || (status & 0b00000001))); // white untile ata is ready for a read or there is an error

        

        if (status & 0b00000001) {
            printf("There is a problem in the ata driver, read wasn't available\n");
        }

        // else status & 0b00001000 is set
        for (uint32_t i = 0; i < 256; i++) {
            ata_request.data[i] = inw(ATA_PRIMARY_DATA_REGISTER);
        }
    }
        

    return_and_send_eoi:
        ata_request.done = 1;
        outb(PIC2_COMMAND, PIC_EOI); // EOI to slave
        outb(PIC1_COMMAND, PIC_EOI); // EOI to master
}

static void print_identify_device_data(const ATA_IDENTIFY_DEVICE_DATA* id) {
    const uint16_t* w = (const uint16_t*)id;

    // Word 0: General configuration
    printf("Device type: %s\n", (w[0] & 0x8000) ? "Not a hard disk" : "ATA Hard Disk");

    // Word 83: LBA48 support
    printf("LBA48 support: %s\n", (w[83] & (1 << 10)) ? "Yes" : "No");

    // Word 88: UDMA
    uint8_t udma_supported = w[88] & 0xFF;
    uint8_t udma_active = (w[88] >> 8) & 0xFF;
    printf("UDMA supported modes: ");
    for (int i = 0; i < 8; i++)
        if (udma_supported & (1 << i)) printf("%d ", i);
    printf("\nUDMA active mode: ");
    for (int i = 0; i < 8; i++)
        if (udma_active & (1 << i)) printf("%d ", i);
    printf("\n");

    // Word 93: 80-conductor cable
    printf("80-conductor cable detected: %s\n", (w[93] & (1 << 11)) ? "Yes" : "No");

    // Words 60–61: total 28-bit LBA sectors
    uint32_t lba28_total = ((uint32_t)w[61] << 16) | w[60];
    printf("LBA28 total sectors: %x\n", lba28_total);

    // Words 100–103: total 48-bit LBA sectors
    uint32_t lba48_low = ((uint32_t)w[101] << 16) | w[100];
    uint32_t lba48_high = ((uint32_t)w[103] << 16) | w[102];
    printf("LBA48 total sectors (high, low): %x, %x\n", lba48_high, lba48_low);
}