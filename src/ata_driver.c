#include "ata_driver.h"
#include "port.h"
#include "screen.h"
#include "utils.h"
#include "pic.h"
#include "kheap.h"

#define ATA_SELECT_DELAY() \
    do { inb(0x80); inb(0x80); inb(0x80); inb(0x80); } while (0)

#define TICKS_TO_END_WAIT 100

static void print_identify_device_data(const identify_device_data_t* id);
static void print_sector_batch_data(void * sectors, uint32_t sector_count);
static uint8_t ata_handle_read_single_sector_responce();

ata_request_t ata_request;
ata_responce_t ata_responce;

void initiate_ata_driver() {
    // reset the ata request and responce structs
    memset((void *)&ata_request, 0, sizeof(ata_request_t));
    memset((void *)&ata_responce, 0, sizeof(ata_responce_t));

    register_interrupt_handler(46, ata_response_handler);
    register_interrupt_handler(47, ata_response_handler);
}

ata_responce_t * get_ata_responce_structure() {
    return &ata_responce;
}

static uint8_t ata_handle_identify_responce() {
    uint8_t status = 0;
    uint8_t LBAmid = 0;
    uint8_t LBAhi = 0;
    uint8_t error = 0;

    if (ata_responce.device_id.channel == ATA_PRIMARY) {
        status = inb(ATA_PRIMARY_STATUS_REGISTER);
        LBAmid = inb(ATA_PRIMARY_CYLINDER_LOW_REGISTER);
        LBAhi  = inb(ATA_PRIMARY_CYLINDER_HIGH_REGISTER);
    } else { // Secondary channel
        status = inb(ATA_SECONDARY_STATUS_REGISTER);
        LBAmid = inb(ATA_SECONDARY_CYLINDER_LOW_REGISTER);
        LBAhi  = inb(ATA_SECONDARY_CYLINDER_HIGH_REGISTER);
    }

    if (status == 0) {
        error = 1;
        ata_responce.error_message = "A driver wasn't found\n";
        goto _return;
    }
        
    if (LBAmid != 0 || LBAhi != 0) {
        error = 1;
        ata_responce.error_message = "The driver isn't an ATA one\n";
        goto _return;
    }

    do {
        if (ata_responce.device_id.channel == ATA_PRIMARY) 
            status = inb(ATA_PRIMARY_STATUS_REGISTER); 
        else  // Secondary channel
            status = inb(ATA_SECONDARY_STATUS_REGISTER);
    } while (!((status & 0b00001000) || (status & 0b00000001))); // white until ata is ready for a read or there is an error

    if (status & 0b00000001) {
        error = 1;
        ata_responce.error_message = "There is a problem in the ata driver, read wasn't available\n";
        goto _return;
    }

    // else status & 0b00001000 is set
    if (ata_responce.device_id.channel == ATA_PRIMARY)
        for (uint32_t i = 0; i < 256; i++)
            ((uint16_t *)&ata_responce.spesific_request.ata_identify_responce.device_data)[i] = inw(ATA_PRIMARY_DATA_REGISTER);
    else
        for (uint32_t i = 0; i < 256; i++) 
            ((uint16_t *)&ata_responce.spesific_request.ata_identify_responce.device_data)[i] = inw(ATA_SECONDARY_DATA_REGISTER);

    _return:
        return error;
}

uint8_t ata_send_identify_command(uint8_t channel, uint8_t device) {
    // https://wiki.osdev.org/ATA_PIO_Mode#IDENTIFY_command

    // It is important to set the busy before the any call to send the reqeust
    ata_request.pending = 1; // tell that there is a request on the way
    ata_request.request_type = ATA_IDENTIFY_REQUEST;
    ata_request.device_id.channel = channel;
    ata_request.device_id.device = device;
    
    // make sure the responce struct is ready
    ata_responce.device_id.channel = channel;
    ata_responce.device_id.device = device;
    ata_responce.done = 0;
    ata_responce.error_message = NULL;

    if (channel == ATA_PRIMARY) {
        outb(ATA_PRIMARY_DRIVE_HEAD_REGISTER, 0xa0 | (device << 4)); // spesify what device to use (0xa0 = 0b10100000 this needs to be always on)
        // set Sector count, LBAlo, LBAmid and LBAhigh to 0
        outb(ATA_PRIMARY_SECTOR_COUNT_REGISTER, 0);
        outb(ATA_PRIMARY_SECTOR_NUMBER_REGISTER, 0);
        outb(ATA_PRIMARY_CYLINDER_LOW_REGISTER, 0);
        outb(ATA_PRIMARY_CYLINDER_HIGH_REGISTER, 0);
        // send IDENTIFY command (0xEC)
        outb(ATA_PRIMARY_COMMAND_REGISTER, 0xEC);

    } else {  // secondry channel
        outb(ATA_SECONDARY_DRIVE_HEAD_REGISTER, 0xa0 | (device << 4)); // spesify what device to use (0xa0 = 0b10100000 this needs to be always on)
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

    if (ata_responce.was_an_error == 0)
        print_identify_device_data(&ata_responce.spesific_request.ata_identify_responce.device_data);  // there was no error
    else 
        printf("Error: %s", ata_responce.error_message);
    
    return ata_responce.was_an_error;
}

static uint8_t ata_handle_read_single_sector_responce() {
    uint32_t memory_start = (ATA_SECTOR_SIZE / 2) * ata_responce.spesific_request.ata_read_responce.current_secotr_writen;

    uint16_t what_register_to_use = (ata_responce.device_id.channel == ATA_PRIMARY)? ATA_PRIMARY_DATA_REGISTER : ATA_SECONDARY_DATA_REGISTER;

    for (uint32_t i = 0; i < ATA_SECTOR_SIZE / 2; i++)
            ((uint16_t *)ata_responce.spesific_request.ata_read_responce.memory)[memory_start + i] = inw(what_register_to_use);
    
    ata_responce.spesific_request.ata_read_responce.current_secotr_writen++; // increment the current sector to be writen

    return 0;
}

uint8_t ata_send_read_command(uint8_t channel, uint8_t device, uint32_t sector_address, uint8_t sector_count) {
    // https://wiki.osdev.org/ATA_PIO_Mode#28_bit_PIO

    // It is important to set the busy before the any call to send the reqeust
    ata_request.pending = 1; // tell that there is a request on the way
    ata_request.request_type = ATA_READ_REQUEST;
    ata_request.device_id.channel = channel;
    ata_request.spesific_request.ata_read_request.sector_address = sector_address; 
    ata_request.spesific_request.ata_read_request.sector_count = sector_count;

    // make sure the responce struct is ready
    ata_responce.device_id.channel = channel;
    ata_responce.device_id.device = device;
    ata_responce.done = 0;
    ata_responce.error_message = NULL;
    ata_responce.spesific_request.ata_read_responce.current_secotr_writen = 0;
    ata_responce.spesific_request.ata_read_responce.memory = kalloc(ATA_SECTOR_SIZE * sector_count);

    if (channel == ATA_PRIMARY) {
        outb(ATA_PRIMARY_DRIVE_HEAD_REGISTER, 0xe0 | (device << 4) | ((sector_address >> 24) & 0x0F)); // spesify what device to use (0xe0 = 0b11100000 this needs to be always on)
        outb(ATA_PRIMARY_FEATURES_REGISTER, 0);  // send Null (0) to the feature register (don't know why)
        outb(ATA_PRIMARY_SECTOR_COUNT_REGISTER, sector_count);
        outb(ATA_PRIMARY_SECTOR_NUMBER_REGISTER, (uint8_t)(sector_address & 0xFF));
        outb(ATA_PRIMARY_CYLINDER_LOW_REGISTER, (uint8_t)((sector_address >> 8) & 0xFF));
        outb(ATA_PRIMARY_CYLINDER_HIGH_REGISTER, (uint8_t)((sector_address >> 16) & 0xFF));
        outb(ATA_PRIMARY_COMMAND_REGISTER, ATA_READ_REQUEST);
    } else {  // Secondry channel
        outb(ATA_SECONDARY_DRIVE_HEAD_REGISTER, 0xe0 | (device << 4) | ((sector_address >> 24) & 0x0F)); // spesify what device to use (0xe0 = 0b11100000 this needs to be always on)
        outb(ATA_SECONDARY_FEATURES_REGISTER, 0);  // send Null (0) to the feature register (don't know why)
        outb(ATA_SECONDARY_SECTOR_COUNT_REGISTER, sector_count);
        outb(ATA_SECONDARY_SECTOR_NUMBER_REGISTER, (uint8_t)(sector_address & 0xFF));
        outb(ATA_SECONDARY_CYLINDER_LOW_REGISTER, (uint8_t)((sector_address >> 8) & 0xFF));
        outb(ATA_SECONDARY_CYLINDER_HIGH_REGISTER, (uint8_t)((sector_address >> 16) & 0xFF));
        outb(ATA_SECONDARY_COMMAND_REGISTER, ATA_READ_REQUEST);
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

    if (ata_responce.was_an_error == 0)
        print_sector_batch_data(ata_responce.spesific_request.ata_read_responce.memory, ata_request.spesific_request.ata_read_request.sector_count);  // there was no error
    else 
        printf("Error: %s", ata_responce.error_message);
    
    return ata_responce.was_an_error;
}

static uint8_t ata_handle_write_single_sector_responce() {
    ata_responce.spesific_request.ata_write_responce.was_currect_sector_write_done = 1;
    return 0;
}

uint8_t ata_send_write_command(uint8_t channel, uint8_t device, uint32_t sector_address, uint8_t sector_count, void * data) {
    // https://wiki.osdev.org/ATA_PIO_Mode#28_bit_PIO
    // Note: a write request only raise an interrupt after writing a whole sector

    // It is important to set the busy before the any call to send the reqeust
    ata_request.pending = 1; // tell that there is a request on the way
    ata_request.request_type = ATA_WRITE_REQUEST;
    ata_request.device_id.channel = channel;
    ata_request.spesific_request.ata_write_request.sector_address = sector_address; 
    ata_request.spesific_request.ata_write_request.sector_count = sector_count;
    ata_request.spesific_request.ata_write_request.data = data;

    // make sure the responce struct is ready
    ata_responce.device_id.channel = channel;
    ata_responce.device_id.device = device;
    ata_responce.done = 0;
    ata_responce.error_message = NULL;
    ata_responce.spesific_request.ata_write_responce.was_currect_sector_write_done = 0; // not done

    if (channel == ATA_PRIMARY) {
        outb(ATA_PRIMARY_DRIVE_HEAD_REGISTER, 0xe0 | (device << 4) | ((sector_address >> 24) & 0x0F)); // spesify what device to use (0xe0 = 0b11100000 this needs to be always on)
        outb(ATA_PRIMARY_FEATURES_REGISTER, 0);  // send Null (0) to the feature register (don't know why)
        outb(ATA_PRIMARY_SECTOR_COUNT_REGISTER, sector_count);
        outb(ATA_PRIMARY_SECTOR_NUMBER_REGISTER, (uint8_t)(sector_address & 0xFF));
        outb(ATA_PRIMARY_CYLINDER_LOW_REGISTER, (uint8_t)((sector_address >> 8) & 0xFF));
        outb(ATA_PRIMARY_CYLINDER_HIGH_REGISTER, (uint8_t)((sector_address >> 16) & 0xFF));
        outb(ATA_PRIMARY_COMMAND_REGISTER, ATA_WRITE_REQUEST);
    } else {  // Secondry channel
        outb(ATA_SECONDARY_DRIVE_HEAD_REGISTER, 0xe0 | (device << 4) | ((sector_address >> 24) & 0x0F)); // spesify what device to use (0xe0 = 0b11100000 this needs to be always on)
        outb(ATA_SECONDARY_FEATURES_REGISTER, 0);  // send Null (0) to the feature register (don't know why)
        outb(ATA_SECONDARY_SECTOR_COUNT_REGISTER, sector_count);
        outb(ATA_SECONDARY_SECTOR_NUMBER_REGISTER, (uint8_t)(sector_address & 0xFF));
        outb(ATA_SECONDARY_CYLINDER_LOW_REGISTER, (uint8_t)((sector_address >> 8) & 0xFF));
        outb(ATA_SECONDARY_CYLINDER_HIGH_REGISTER, (uint8_t)((sector_address >> 16) & 0xFF));
        outb(ATA_SECONDARY_COMMAND_REGISTER, ATA_WRITE_REQUEST);
    }

    ATA_SELECT_DELAY();
    uint32_t ticks = 0;

    uint16_t what_register_to_use = (ata_responce.device_id.channel == ATA_PRIMARY)? ATA_PRIMARY_DATA_REGISTER : ATA_SECONDARY_DATA_REGISTER;

    for (uint32_t i = 0; i < ata_request.spesific_request.ata_write_request.sector_count; i++) {
        uint32_t memory_start = (ATA_SECTOR_SIZE / 2) * i;
        ata_responce.spesific_request.ata_write_responce.was_currect_sector_write_done = 0;  // reset flag for next irq

        for (uint32_t k = 0; k < (ATA_SECTOR_SIZE / 2); k++) {
                outw(what_register_to_use, ((uint16_t *)ata_request.spesific_request.ata_write_request.data)[memory_start + k]);
                asm volatile ("jmp . + 2");
            }

        while (ata_responce.spesific_request.ata_write_responce.was_currect_sector_write_done != 1) {
            ticks++;
            if (ticks == TICKS_TO_END_WAIT) break;
            asm volatile("hlt"); // wait for the irq to finish working
        }

        if (ticks == TICKS_TO_END_WAIT) {
            printf("Error on IDENTIFY channel %x device %x\n", channel, device);
            return 1;
        }
    }
    
    return ata_responce.was_an_error;
}

void ata_response_handler(registers_t* regs) {
    if (ata_request.pending != 1) {
        printf("Something went wrong, there is no pending ata request\n");
        goto return_and_send_eoi;
    }

    ata_request.pending = 0;  // update the request pending status

    if (ata_request.request_type == ATA_IDENTIFY_REQUEST) {
        ata_responce.was_an_error = ata_handle_identify_responce();
    } else if (ata_request.request_type == ATA_READ_REQUEST) {
        ata_handle_read_single_sector_responce();
    } else if (ata_request.request_type == ATA_WRITE_REQUEST) {
        ata_handle_write_single_sector_responce();
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

static void print_sector_batch_data(void *sectors, uint32_t sector_count) {
    uint32_t size = sector_count * ATA_SECTOR_SIZE;
    print_hexdump(sectors, size);
}