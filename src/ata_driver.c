#include "ata_driver.h"
#include "port.h"
#include "screen.h"
#include "utils.h"
#include "pic.h"

#define ATA_SELECT_DELAY() \
    do { inb(0x80); inb(0x80); inb(0x80); inb(0x80); } while (0)

#define TICKS_TO_END_WAIT 20

static void print_identify_device_data(const identify_device_data_t* id);

ata_request_t ata_request;
ata_responce_t ata_responce;

void initiate_ata_driver() {
    // reset the ata request and responce structs
    ata_request.pending = 0;
    ata_request.request_type = 0;
    ata_request.channel = 0;
    ata_request.device = 0;
    ata_responce.done = 0;
    ata_responce.was_an_error = 0;
    ata_responce.error_message = NULL;
    memset((void *)ata_responce.data, 0, sizeof(ata_responce.data));

    register_interrupt_handler(46, ata_response_handler);
    register_interrupt_handler(47, ata_response_handler);
}

ata_responce_t * get_ata_responce_structure() {
    return &ata_responce;
}

uint8_t ata_send_identify_command(uint8_t channel, uint8_t device) {
    // https://wiki.osdev.org/ATA_PIO_Mode#IDENTIFY_command

    // It is important to set the busy before the any call to send the reqeust
    ata_request.pending = 1;
    ata_request.request_type = ATA_IDENTIFY_REQUEST;
    ata_request.channel = channel;
    ata_request.device = device; // tell that there is a request on the way

    if (channel == ATA_PRIMARY) {
        outb(ATA_PRIMARY_DRIVE_HEAD_REGISTER, 0xa0 | (device << 4)); // spesify what driver to use (0xa0 = 0b10100000 this needs to be always on)
        // set Sector count, LBAlo, LBAmid and LBAhigh to 0
        outb(ATA_PRIMARY_SECTOR_COUNT_REGISTER, 0);
        outb(ATA_PRIMARY_SECTOR_NUMBER_REGISTER, 0);
        outb(ATA_PRIMARY_CYLINDER_LOW_REGISTER, 0);
        outb(ATA_PRIMARY_CYLINDER_HIGH_REGISTER, 0);
        // send IDENTIFY command (0xEC)
        outb(ATA_PRIMARY_COMMAND_REGISTER, 0xEC);

    } else {  // secondry channel
        outb(ATA_SECONDARY_DRIVE_HEAD_REGISTER, 0xa0 | (device << 4)); // spesify what driver to use (0xa0 = 0b10100000 this needs to be always on)
        // set Sector count, LBAlo, LBAmid and LBAhigh to 0
        outb(ATA_SECONDARY_SECTOR_COUNT_REGISTER, 0);
        outb(ATA_SECONDARY_SECTOR_NUMBER_REGISTER, 0);
        outb(ATA_SECONDARY_CYLINDER_LOW_REGISTER, 0);
        outb(ATA_SECONDARY_CYLINDER_HIGH_REGISTER, 0);
        // send IDENTIFY command (0xEC)
        outb(ATA_SECONDARY_COMMAND_REGISTER, 0xEC);
    }


    ATA_SELECT_DELAY();
    uint32_t ticks = 0;

    while (ata_responce.done != 1) {
        ticks++;
        if (ticks == TICKS_TO_END_WAIT) break;
        asm volatile("hlt"); // wait for the irq to finish working
    }

    if (ticks == TICKS_TO_END_WAIT) {
        printf("Error on IDENTIFY channel %x device %x\n", channel, device);
        return 1;
    }

    if (ata_responce.was_an_error != 0)
        print_identify_device_data((identify_device_data_t *)ata_responce.data);  // there was no error
    else 
        printf("Error: %s", ata_responce.error_message);
    
    return ata_responce.was_an_error;
}

void ata_response_handler(registers_t* regs) {
    if (ata_request.pending != 1) {
        printf("Something went wrong, there is no pending ata request\n");
        goto return_and_send_eoi;
    }

    ata_request.pending = 0;  // update the request pending status

    if (ata_request.request_type == ATA_IDENTIFY_REQUEST) {
        uint8_t status = inb(ATA_PRIMARY_STATUS_REGISTER);
        uint8_t LBAmid = inb(ATA_PRIMARY_CYLINDER_LOW_REGISTER);
        uint8_t LBAhi  = inb(ATA_PRIMARY_CYLINDER_HIGH_REGISTER);

        if (status == 0) {
            ata_responce.was_an_error = 1;
            ata_responce.error_message = "A driver wasn't found\n";
            
            goto return_and_send_eoi;
        }
        
        if (LBAmid != 0 || LBAhi != 0) {
            ata_responce.was_an_error = 1;
            ata_responce.error_message = "The driver isn't an ATA one\n";
            goto return_and_send_eoi;
        }

        do {
            status = inb(ATA_PRIMARY_STATUS_REGISTER); 
        } while (!((status & 0b00001000) || (status & 0b00000001))); // white untile ata is ready for a read or there is an error

        

        if (status & 0b00000001) {
            ata_responce.was_an_error = 1;
            ata_responce.error_message = "There is a problem in the ata driver, read wasn't available\n";
            goto return_and_send_eoi;
        }
        ata_responce.was_an_error = 0;
        // else status & 0b00001000 is set
        for (uint32_t i = 0; i < 256; i++) {
            ata_responce.data[i] = inw(ATA_PRIMARY_DATA_REGISTER);
        }
    }
        

    return_and_send_eoi:
        ata_responce.done = 1;
        outb(PIC2_COMMAND, PIC_EOI); // EOI to slave
        outb(PIC1_COMMAND, PIC_EOI); // EOI to master
}

static void print_identify_device_data(const identify_device_data_t* id) {
    printf("=== ATA IDENTIFY DEVICE DATA ===\n");

    // --- General information ---
    printf("Device type: %s\n",
           id->GeneralConfiguration.DeviceType ? "Non-disk device" : "ATA Hard Disk");
    printf("Fixed device: %s\n",
           id->GeneralConfiguration.FixedDevice ? "Yes" : "No");
    printf("Removable media: %s\n",
           id->GeneralConfiguration.RemovableMedia ? "Yes" : "No");

    // --- LBA and addressing ---
    printf("LBA supported: %s\n", id->Capabilities.LbaSupported ? "Yes" : "No");
    printf("DMA supported: %s\n", id->Capabilities.DmaSupported ? "Yes" : "No");

    // --- 28-bit LBA total sectors (words 60–61) ---
    printf("User addressable sectors (LBA28): %d (%x)\n",
           id->UserAddressableSectors, id->UserAddressableSectors);

    // --- 48-bit LBA total sectors (words 100–103) ---
    uint64_t lba48_total =
        ((uint64_t)id->Max48BitLBA[1] << 32) | id->Max48BitLBA[0];
    if (lba48_total)
        printf("User addressable sectors (LBA48): %d (%x)\n",
               (unsigned long long)lba48_total, (unsigned long long)lba48_total);
    else
        printf("User addressable sectors (LBA48): Not supported\n");

    // --- UDMA support and active modes (word 88) ---
    printf("UDMA supported modes: ");
    for (int i = 0; i < 8; i++)
        if (id->UltraDMASupport & (1 << i))
            printf("%d ", i);
    printf("\n");

    printf("UDMA active mode: ");
    for (int i = 0; i < 8; i++)
        if (id->UltraDMAActive & (1 << i))
            printf("%d ", i);
    printf("\n");

    // --- 80-conductor cable detection (word 93, bit 11) ---
    printf("80-conductor cable detected: %s\n",
           (id->HardwareResetResult & (1 << 11)) ? "Yes" : "No");

    // --- Serial / Model ---
    //printf("Serial Number: %s\n", id->SerialNumber); those doesn't work well with the current print
    //printf("Firmware Revision: %s\n", id->FirmwareRevision);
    //printf("Model Number: %s\n", id->ModelNumber);

    printf("===============================\n");
}
