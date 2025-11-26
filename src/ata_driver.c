#include "ata_driver.h"
#include "port.h"
#include "screen.h"
#include "utils.h"
#include "pic.h"
#include "kheap.h"

#define ATA_SELECT_DELAY()                    \
    do {                                      \
        inb(ATA_PRIMARY_IO + ATA_REG_STATUS); \
        inb(ATA_PRIMARY_IO + ATA_REG_STATUS); \
        inb(ATA_PRIMARY_IO + ATA_REG_STATUS); \
        inb(ATA_PRIMARY_IO + ATA_REG_STATUS); \
    } while (0)

#define WAIT_FOR_ATA_RESPONCE_TO_BE_DONE()               \
    /* wait for the irq to finish working */             \
    while (!ata_responce.done) {                         \
        asm volatile("hlt");                             \
    }

typedef struct ata_request_struct{
    volatile device_id_t device_id; // the device id the request was sent to
    volatile uint8_t pending; // set if there is a pending request, else clear
    volatile uint8_t request_type;
    union {
        volatile struct {
            uint32_t sector_address; // the sector address
            uint32_t sector_count; // the sector count
        } ata_read_request;
        volatile struct {
            uint32_t sector_address; // the first sector address to write
            size_t sector_count; // the sector count to be writen
            void * data; // the data that wanted to be passed to the driver (len(data) = sector_count * ATA_SECTOR_SIZE)
        } ata_write_request;
    } spesific_request;  // a spesific request
} ata_request_t;

typedef struct ata_responce_struct{
    volatile device_id_t device_id; // the device id the responce was returned from
    volatile uint8_t done: 4;  // 1 if there the request have been handled, else 0 
    volatile uint8_t was_an_error: 4;  // 1 if there was an error in the handeling of the request, else 0
    union {
        volatile struct {
            uint32_t current_secotr_writen; // the current sector that is writen in the current responce (irq)
            uint8_t * memory; // the end memory address of the data from the device 
        } ata_read_responce;
        volatile struct {
            uint8_t was_currect_sector_write_done; // 1 if the driver sent an interrupt that he have finish writing to the disk, else 1
        } ata_write_responce;
    } spesific_request;
} ata_responce_t;

static uint8_t ata_handle_identify_responce();
static uint8_t ata_handle_read_single_sector_responce();
static uint8_t ata_handle_write_single_sector_responce();

ata_request_t ata_request;
ata_responce_t ata_responce;

void ata_wait_bsy(ata_drive_t* drv) {
    while (inb(drv->device_id.ctrl_base) & ATA_SR_BSY) asm volatile("hlt");
}

void ata_wait_drq(ata_drive_t* drv) {
    while (!(inb(drv->device_id.ctrl_base) & ATA_SR_DRQ)) asm volatile("hlt");
}

void initiate_ata_driver() {
    // reset the ata request and responce structs
    memset((void *)&ata_request, 0, sizeof(ata_request_t));
    memset((void *)&ata_responce, 0, sizeof(ata_responce_t));

    register_interrupt_handler(46, ata_response_handler);
    register_interrupt_handler(47, ata_response_handler);
}

static uint8_t ata_handle_identify_responce() {
    uint8_t status = inb(ata_responce.device_id.io_base + ATA_REG_STATUS);
    uint8_t LBAmid = inb(ata_responce.device_id.io_base + ATA_REG_LBA_MID);
    uint8_t LBAhi = inb(ata_responce.device_id.io_base + ATA_REG_LBA_HIGH);
    uint8_t error = 0;

    if (status == 0) {
        error = 1;
        printf("ATA: %s - %s : A driver wasn't found\n", (ata_responce.device_id.io_base == ATA_PRIMARY_IO)? "Primary" : "Secondary", (ata_responce.device_id.master == 1)? "Master" : "Slave");
        goto _return;
    }
        
    if (LBAmid != 0 || LBAhi != 0) {
        error = 1;
        printf("ATA: %s - %s : The driver isn't an ATA one\n", (ata_responce.device_id.io_base == ATA_PRIMARY_IO)? "Primary" : "Secondary", (ata_responce.device_id.master == 1)? "Master" : "Slave");
        goto _return;
    }

    do {
        status = inb(ata_responce.device_id.io_base + ATA_REG_STATUS);
    } while (!(status & ATA_SR_DRQ & ATA_SR_ERR)); // white until ata is ready for a read or there is an error

    if (status & ATA_SR_ERR) {
        error = 1;
        printf("ATA: %s - %s : There is a problem in the ata driver, read wasn't available\n", (ata_responce.device_id.io_base == ATA_PRIMARY_IO)? "Primary" : "Secondary", (ata_responce.device_id.master == 1)? "Master" : "Slave");
        goto _return;
    }

    // else status & ATA_SR_DRQ is set
    for (uint32_t i = 0; i < 256; i++)
        ((uint16_t *)&ata_responce.spesific_request.ata_read_responce.memory)[i] = inw(ata_responce.device_id.io_base + ATA_REG_DATA);

    _return:
        return error;
}

static uint8_t ata_handle_read_single_sector_responce() {
    uint32_t memory_start = (ATA_SECTOR_SIZE / 2) * ata_responce.spesific_request.ata_read_responce.current_secotr_writen;

    for (uint32_t i = 0; i < ATA_SECTOR_SIZE / 2; i++)
        ((uint16_t *)ata_responce.spesific_request.ata_read_responce.memory)[memory_start + i] = inw(ata_responce.device_id.io_base + ATA_REG_DATA);
    
    ata_responce.spesific_request.ata_read_responce.current_secotr_writen++; // increment the current sector to be writen

    return 0;
}

static uint8_t ata_handle_write_single_sector_responce() {
    ata_responce.spesific_request.ata_write_responce.was_currect_sector_write_done = 1;
    return 0;
}

uint8_t ata_send_identify_command(ata_drive_t* drive, identify_device_data_t* buffer) {
    // https://wiki.osdev.org/ATA_PIO_Mode#28_bit_PIO
    ata_request.device_id = drive->device_id;
    ata_request.pending = 0;
    ata_request.request_type = ATA_CMD_READ_PIO;

    // make sure the responce struct is ready
    ata_responce.device_id = drive->device_id;
    ata_responce.done = 0;
    ata_responce.was_an_error = 0;
    ata_responce.spesific_request.ata_read_responce.current_secotr_writen = 0;
    ata_responce.spesific_request.ata_read_responce.memory = buffer;

    outb(drive->device_id.io_base + ATA_REG_HDDEVSEL, 0xe0 | (drive->device_id.master << 4));
    outb(drive->device_id.io_base + ATA_REG_SECCOUNT, 0);
    outb(drive->device_id.io_base + ATA_REG_LBA_LOW, 0);
    outb(drive->device_id.io_base + ATA_REG_LBA_MID, 0);
    outb(drive->device_id.io_base + ATA_REG_LBA_HIGH, 0);
    outb(drive->device_id.io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    ata_request.pending = 1;

    ATA_SELECT_DELAY();

    WAIT_FOR_ATA_RESPONCE_TO_BE_DONE();

    return ata_responce.was_an_error; // TODO: add an error handling situation (max tick in responce done)
}

uint8_t ata_read28_request(ata_drive_t* drive, uint32_t sector_address, uint8_t sector_count, uint8_t* buffer) {
    // https://wiki.osdev.org/ATA_PIO_Mode#28_bit_PIO
    ata_request.device_id = drive->device_id;
    ata_request.pending = 0;
    ata_request.request_type = ATA_CMD_READ_PIO;
    ata_request.spesific_request.ata_read_request.sector_address = sector_address; 
    ata_request.spesific_request.ata_read_request.sector_count = sector_count;

    // make sure the responce struct is ready
    ata_responce.device_id = drive->device_id;
    ata_responce.done = 0;
    ata_responce.was_an_error = 0;
    ata_responce.spesific_request.ata_read_responce.current_secotr_writen = 0;
    ata_responce.spesific_request.ata_read_responce.memory = buffer;

    outb(drive->device_id.io_base + ATA_REG_HDDEVSEL, 0xe0 | (drive->device_id.master << 4) | ((sector_address >> 24) & 0x0F)); // spesify what device to use (0xe0 = 0b11100000 this needs to be always on)
    outb(drive->device_id.io_base + ATA_REG_FEATURES, 0);  // send Null (0) to the feature register (don't know why)
    outb(drive->device_id.io_base + ATA_REG_SECCOUNT, sector_count);
    outb(drive->device_id.io_base + ATA_REG_LBA_LOW, (uint8_t)(sector_address & 0xFF));
    outb(drive->device_id.io_base + ATA_REG_LBA_MID, (uint8_t)((sector_address >> 8) & 0xFF));
    outb(drive->device_id.io_base + ATA_REG_LBA_HIGH, (uint8_t)((sector_address >> 16) & 0xFF));
    outb(drive->device_id.io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    ata_request.pending = 1;

    ATA_SELECT_DELAY();

    WAIT_FOR_ATA_RESPONCE_TO_BE_DONE();

    return ata_responce.was_an_error; // TODO: add an error handling situation (max tick in responce done)
}

uint8_t ata_write28_request(ata_drive_t* drive, uint32_t sector_address, uint8_t sector_count, uint8_t* buffer) {
    // https://wiki.osdev.org/ATA_PIO_Mode#28_bit_PIO
    // Note: a write request only raise an interrupt after writing a whole sector

    // It is important to set the busy before the any call to send the reqeust
    ata_request.device_id = drive->device_id;
    ata_request.pending = 0;
    ata_request.request_type = ATA_CMD_WRITE_PIO;
    ata_request.spesific_request.ata_write_request.sector_address = sector_address; 
    ata_request.spesific_request.ata_write_request.sector_count = sector_count;
    ata_request.spesific_request.ata_write_request.data = buffer;

    // make sure the responce struct is ready
    ata_responce.device_id = drive->device_id;
    ata_responce.done = 0;
    ata_responce.spesific_request.ata_write_responce.was_currect_sector_write_done = 0;

    outb(drive->device_id.io_base + ATA_REG_HDDEVSEL, 0xe0 | (drive->device_id.master << 4) | ((sector_address >> 24) & 0x0F)); // spesify what device to use (0xe0 = 0b11100000 this needs to be always on)
    outb(drive->device_id.io_base + ATA_REG_FEATURES, 0);  // send Null (0) to the feature register (don't know why)
    outb(drive->device_id.io_base + ATA_REG_SECCOUNT, sector_count);
    outb(drive->device_id.io_base + ATA_REG_LBA_LOW, (uint8_t)(sector_address & 0xFF));
    outb(drive->device_id.io_base + ATA_REG_LBA_MID, (uint8_t)((sector_address >> 8) & 0xFF));
    outb(drive->device_id.io_base + ATA_REG_LBA_HIGH, (uint8_t)((sector_address >> 16) & 0xFF));
    outb(drive->device_id.io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    ata_request.pending = 1;

    ATA_SELECT_DELAY();
    uint32_t ticks = 0;

    for (uint32_t i = 0; i < ata_request.spesific_request.ata_write_request.sector_count; i++) {
        uint32_t memory_start = (ATA_SECTOR_SIZE / 2) * i;
        ata_responce.spesific_request.ata_write_responce.was_currect_sector_write_done = 0;  // reset flag for next irq

        for (uint32_t k = 0; k < (ATA_SECTOR_SIZE / 2); k++) {
                outw(drive->device_id.io_base + ATA_REG_DATA, ((uint16_t *)ata_request.spesific_request.ata_write_request.data)[memory_start + k]);
                asm volatile ("jmp . + 2");
            }

        while (ata_responce.spesific_request.ata_write_responce.was_currect_sector_write_done != 1)
             asm volatile("hlt"); // wait for the irq to finish working
    }
    
    return ata_responce.was_an_error;
}


void ata_response_handler(registers_t* regs) {
    if (ata_request.pending != 1) {
        printf("Something went wrong, there is no pending ata request\n");
        goto return_and_send_eoi;
    }

    ata_request.pending = 0;  // update the request pending status

    if (ata_request.request_type == ATA_CMD_IDENTIFY) {
        ata_responce.was_an_error = ata_handle_identify_responce();
    } else if (ata_request.request_type == ATA_CMD_READ_PIO) {
        ata_handle_read_single_sector_responce();
    } else if (ata_request.request_type == ATA_CMD_WRITE_PIO) {
        ata_handle_write_single_sector_responce();
    }

    return_and_send_eoi:
        ata_responce.done = 1;
        outb(PIC2_COMMAND, PIC_EOI); // EOI to slave
        outb(PIC1_COMMAND, PIC_EOI); // EOI to master
}

void print_identify_device_data(const identify_device_data_t* id) {
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