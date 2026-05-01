#include "drivers/ata_driver.h"
#include "kernel/print.h"
#include "io/port.h"

static ata_drive_t * current_working_drive;

/*
 * Static helper functions
 */
static void delay_400ns(ata_drive_t *drive);
static uint8_t read_status_reg(ata_drive_t *drive);
static ata_error_t ata_read28_one_sector_request(ata_drive_t *drive, uint32_t sector, uint8_t *buffer);
static ata_error_t ata_write28_one_sector_request(ata_drive_t *drive, uint32_t sector, uint8_t *buffer);
static uint32_t ata_response_handler(cpu_status_t *regs);

static void delay_400ns(ata_drive_t *drive) {
    if (!drive) return;

    inb(drive->drive_id.ctrl_base + ATA_REG_ALTSTATUS);
    inb(drive->drive_id.ctrl_base + ATA_REG_ALTSTATUS);
    inb(drive->drive_id.ctrl_base + ATA_REG_ALTSTATUS);
    inb(drive->drive_id.ctrl_base + ATA_REG_ALTSTATUS);
}

static uint8_t read_status_reg(ata_drive_t *drive) {
    if (!drive) return 0;

    return inb(drive->drive_id.io_base + ATA_REG_STATUS);
}

static ata_error_t ata_read28_one_sector_request(ata_drive_t *drive, uint32_t sector, uint8_t *buffer) {
    if (!drive || !buffer) return ATA_ERR_INVALID;
    
    lock_acquire(&drive->lock);
    outb(drive->drive_id.io_base + ATA_REG_HDDEVSEL, 0xE0 | (drive->drive_id.master<<4) | ((sector>>24)&0x0F));
    outb(drive->drive_id.io_base + ATA_REG_FEATURES, 0);  // send Null (0) to the feature register (don't know why)
    outb(drive->drive_id.io_base + ATA_REG_SECCOUNT, 1);
    outb(drive->drive_id.io_base + ATA_REG_LBA_LOW,  sector & 0xFF);
    outb(drive->drive_id.io_base + ATA_REG_LBA_MID, (sector >> 8) & 0xFF);
    outb(drive->drive_id.io_base + ATA_REG_LBA_HIGH,(sector >>16) & 0xFF);

    /* check if disk is not busy and ready */
    ata_wait_not_busy(drive);
    ata_wait_drive_ready(drive);

    /* send the command */
    outb(drive->drive_id.io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    ata_wait_not_busy(drive);

    /* check if we got an error */
    if (ata_check_err(drive))
        return ATA_ERR_STATUS_ERR;

    for(int i = 0; i < ATA_SECTOR_SIZE/2; i++) {
		uint16_t data = inw(drive->drive_id.io_base + ATA_REG_DATA);
		((uint16_t *)buffer)[i] = data;
	}

    /* Note for polling PIO drivers: After transferring the last uint16_t of a PIO data block to the data IO port, 
     * give the drive a 400ns delay to reset its DRQ bit (and possibly set BSY again, 
     * while emptying/filling its buffer to/from the drive).
     */
    delay_400ns(drive);

    lock_release(&drive->lock);

    return ATA_OK;
}

static ata_error_t ata_write28_one_sector_request(ata_drive_t *drive, uint32_t sector, uint8_t *buffer) {
    if (!drive || !buffer) return ATA_ERR_INVALID;
    
    lock_acquire(&drive->lock);
    outb(drive->drive_id.io_base + ATA_REG_HDDEVSEL, 0xE0 | (drive->drive_id.master<<4) | ((sector>>24)&0x0F));
    outb(drive->drive_id.io_base + ATA_REG_FEATURES, 0);  // send Null (0) to the feature register (don't know why)
    outb(drive->drive_id.io_base + ATA_REG_SECCOUNT, 1);
    outb(drive->drive_id.io_base + ATA_REG_LBA_LOW,  sector & 0xFF);
    outb(drive->drive_id.io_base + ATA_REG_LBA_MID, (sector >> 8) & 0xFF);
    outb(drive->drive_id.io_base + ATA_REG_LBA_HIGH,(sector >>16) & 0xFF);

    /* check if disk is not busy and ready */
    ata_wait_not_busy(drive);
    ata_wait_drive_ready(drive);

    /* send the command */
    outb(drive->drive_id.io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    /* check if disk is not busy and ready */
    ata_wait_not_busy(drive);

    /* check if we got an error */
    if (ata_get_err(drive)) return ATA_ERR_STATUS_ERR;

    for(int i = 0; i < ATA_SECTOR_SIZE/2; i++) {
        outw(drive->drive_id.io_base + ATA_REG_DATA, ((uint16_t *)buffer)[i]);
        asm volatile("jmp .+2"); // short mandatory delay
	}

    ata_wait_not_busy(drive);

    lock_release(&drive->lock);

    return ATA_OK;
}

static uint32_t ata_response_handler(cpu_status_t *regs) {
    read_status_reg(current_working_drive);
    return 0;
}

/*
 * Helper functions
 */
void ata_wait_not_busy(ata_drive_t *drive) {
    if (!drive) return;
    uint8_t status;

    do {
        status = read_status_reg(drive);
    } while (status & ATA_SR_BSY);
}

void ata_wait_drq_ready(ata_drive_t *drive) {
    if (!drive) return;
    uint8_t status;

    do {
        status = read_status_reg(drive);

        if (status & ATA_SR_ERR)
            return;

    } while (!(status & ATA_SR_DRQ));
}

void ata_wait_drive_ready(ata_drive_t *drive) {
    if (!drive) return;
    uint8_t status;

    while (1)
    {
        status = inb(drive->drive_id.io_base + ATA_REG_STATUS);

        // Wait while BSY is set
        if (status & ATA_SR_BSY)
            continue;

        // Check for errors
        if (status & (ATA_SR_ERR | ATA_SR_DF))
            return;

        // Ready when DRDY is set
        if (status & ATA_SR_DRDY)
            return;
    }
}

uint8_t ata_check_err(ata_drive_t *drive) {
    uint8_t status;

    status = read_status_reg(drive);

    return status & ATA_SR_ERR;
}

uint8_t ata_get_err(ata_drive_t *drive) {
    uint8_t err;

    err = inb(drive->drive_id.io_base + ATA_REG_ERROR);
    
    return err;
}

/* 
 * Driver functions
 */

 void ata_driver_init() {
    current_working_drive = NULL;

    register_interrupt_handler(46, ata_response_handler);
    register_interrupt_handler(47, ata_response_handler);
}

ata_error_t ata_drive_init(ata_drive_t *drive,
                   uint16_t io_base,
                   uint16_t ctrl_base,
                   uint8_t kind) {
    if (!drive) return ATA_ERR_INVALID;
    
    drive->drive_id.io_base = io_base;
    drive->drive_id.ctrl_base = ctrl_base;
    drive->drive_id.master = kind;
    drive->exists = 0;

    lock_init(&drive->lock);

    return ATA_OK;
}

ata_error_t ata_read28_request(ata_drive_t *drive, uint32_t sector, uint8_t count, uint8_t *buffer) {
    if (!drive || !buffer) return ATA_ERR_INVALID;
    uint8_t err;

    current_working_drive = drive;

    for (uint32_t i = 0; i < count; i++) {
        err = ata_read28_one_sector_request(drive, sector + i, buffer);
        if (err != ATA_OK) return err;

        buffer += ATA_SECTOR_SIZE;
    }

    return ATA_OK;
}

ata_error_t ata_write28_request(ata_drive_t *drive, uint32_t sector, uint8_t count, uint8_t *buffer) {
    if (!drive || !buffer) return ATA_ERR_INVALID;
    uint8_t err;

    current_working_drive = drive;

    for (uint32_t i = 0; i < count; i++) {
        err = ata_write28_one_sector_request(drive, sector + i, buffer);
        if (err != ATA_OK) return err;

        buffer += ATA_SECTOR_SIZE;
    }

    return ata_flush_cache(drive);
}

ata_error_t ata_send_identify_command(ata_drive_t *drive, identify_device_data_t *buffer) {
    if (!drive || !buffer) return ATA_ERR_INVALID;

    lock_acquire(&drive->lock);
    current_working_drive = drive;

    outb(drive->drive_id.io_base + ATA_REG_HDDEVSEL, 0xE0 | (drive->drive_id.master << 4));
    outb(drive->drive_id.io_base + ATA_REG_LBA_LOW,  0);
    outb(drive->drive_id.io_base + ATA_REG_LBA_MID,  0);
    outb(drive->drive_id.io_base + ATA_REG_LBA_HIGH, 0);

    /* send the command */
    outb(drive->drive_id.io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = read_status_reg(drive);
    if (status == 0) {
        printf("ATA: No device detected on the port\n");
        return ATA_ERR_NO_DEVICE;
    }

    /* wait for drive to not be busy */
    ata_wait_not_busy(drive);

    uint8_t LBAmid = inb(drive->drive_id.io_base + ATA_REG_LBA_MID);
    uint8_t LBAhi  = inb(drive->drive_id.io_base + ATA_REG_LBA_HIGH);

    if (LBAmid != 0 || LBAhi != 0) {
        printf("ATA: Device is not ATA compatible\n");
        return ATA_ERR_NO_DEVICE;
    }

    ata_wait_drq_ready(drive);
    if (ata_check_err(drive)) {
        printf("ATA: IDENTIFY failed, error flag raised\n");
        return ATA_ERR_STATUS_ERR;
    }

    // Read IDENTIFY data
    for (uint32_t i = 0; i < ATA_SECTOR_SIZE/2; i++)
        ((uint16_t *)buffer)[i] = inw(drive->drive_id.io_base + ATA_REG_DATA);

    /* may or may not be needed, not so sure */
    delay_400ns(drive);

    lock_release(&drive->lock);

    return ATA_OK;
}

ata_error_t ata_flush_cache(ata_drive_t *drive) {
    if (!drive) return ATA_ERR_INVALID;
    
    lock_acquire(&drive->lock);

    current_working_drive = drive;

    outb(drive->drive_id.io_base + ATA_REG_HDDEVSEL, 0xE0 | (drive->drive_id.master << 4));
    outb(drive->drive_id.io_base + ATA_REG_COMMAND, ATA_CMD_FLUSH);

    /* "sending the 0xE7 command to the Command Register (then waiting for BSY to clear)" */
    ata_wait_not_busy(drive);

    /* check if we got an error */
    uint8_t err = ata_check_err(drive);
    if (err != 0) return ATA_ERR_STATUS_ERR;

    lock_release(&drive->lock);

    return ATA_OK;
}

/* 
 * Print helper functions 
 */
void print_identify_device_data(const identify_device_data_t *id) {
    printf("=== ATA IDENTIFY DEVICE DATA ===\n");

    printf("Device type: %s\n",
           id->GeneralConfiguration.DeviceType ? "Non-disk device" : "ATA Hard Disk");

    printf("LBA28 Sector count: %u (0x%x)\n",
           id->UserAddressableSectors, id->UserAddressableSectors);

    printf("LBA48 Sector count: %s\n",
           id->Max48BitLBA[0] ? "Supported" : "Not supported");

    printf("UDMA supported modes: ");
    for (int i = 0; i < 8; i++)
        if (id->UltraDMASupport & (1 << i))
            printf("%d ", i);

    printf("\n80-wire cable detected: %s\n",
           (id->HardwareResetResult & (1 << 11)) ? "Yes" : "No");

    printf("================================\n");
}