#include "kernel/print.h"
#include "mm/kheap.h"
#include "drivers/ata_driver.h"
#include "io/port.h"
#include "io/pic.h"
#include "utils.h"

/* =========================================================
                        MACROS
   ========================================================= */

/**
 * Macro: ATA_SELECT_DELAY
 * -------------------------------------
 * Performs 4 dummy reads from the ATA status port to allow
 * the drive time to switch between Master/Slave selection.
 * Required by ATA hardware timing rules.
 */
#define ATA_SELECT_DELAY()                    \
    do {                                      \
        inb(ATA_PRIMARY_IO + ATA_REG_STATUS);  \
        inb(ATA_PRIMARY_IO + ATA_REG_STATUS);  \
        inb(ATA_PRIMARY_IO + ATA_REG_STATUS);  \
        inb(ATA_PRIMARY_IO + ATA_REG_STATUS);  \
    } while (0)

/**
 * Macro: WAIT_FOR_ATA_RESPONSE_TO_BE_DONE
 * -------------------------------------
 * Halts the CPU with `hlt` until the IRQ handler signals
 * that the current ATA request has finished processing.
 */
#define WAIT_FOR_ATA_RESPONSE_TO_BE_DONE()     \
    while (!ata_responce.done) {               \
        asm volatile("hlt");                   \
    }

/* =========================================================
                    REQUEST / RESPONSE STRUCTS
   ========================================================= */

/**
 * Struct: ata_request_struct
 * -------------------------------------
 * Holds a pending ATA command request.
 * Supports 28-bit LBA Read and Write operations.
 */
typedef struct ata_request_struct {
    volatile device_id_t device_id; // Target ATA device (primary/secondary, master/slave)
    volatile uint8_t pending;       // 1 = request active, 0 = idle
    volatile uint8_t request_type;  // Command type (identify, read, write)

    union {
        volatile struct {
            uint32_t sector_address; // LBA28 address to read from
            uint32_t sector_count;   // Number of sectors to read
        } read;

        volatile struct {
            uint32_t sector_address; // LBA28 address to write to
            size_t   sector_count;   // Number of sectors to write
            void    *data;           // Pointer to data (must be sector_count * 512 bytes)
        } write;
    } specific;
} ata_request_t;


/**
 * Struct: ata_responce_struct
 * -------------------------------------
 * Holds the ATA device response after IRQ handling.
 */
typedef struct ata_responce_struct {
    volatile device_id_t device_id; // Source device of the last handled response
    volatile uint8_t done:4;        // 1 = request handled and finished
    volatile uint8_t error:4;       // 1 = request resulted in error

    union {
        volatile struct {
            uint32_t sector_index; // Which sector was just read in this IRQ cycle
            uint8_t *buffer;      // Memory destination for disk → RAM data
        } read_resp;

        volatile struct {
            uint8_t sector_write_done; // Set by IRQ after sector was written
        } write_resp;
    } specific;
} ata_responce_t;

/* Global static functions */
static void ata_response_handler(registers_t *regs);

/* Global driver request/response state */
ata_request_t  ata_request;
ata_responce_t ata_responce;

/* =========================================================
                        WAIT HELPERS
   ========================================================= */

/**
 * Function: ata_wait_bsy
 * -------------------------------------
 * Waits until the drive is no longer busy (BSY flag clears).
 */
void ata_wait_bsy(ata_drive_t *drv) {
    while (inb(drv->device_id.ctrl_base) & ATA_SR_BSY)
        asm volatile("hlt");
}

/**
 * Function: ata_wait_drq
 * -------------------------------------
 * Waits until the drive signals it is ready to transfer
 * data (DRQ flag set).
 */
void ata_wait_drq(ata_drive_t *drv) {
    while (!(inb(drv->device_id.ctrl_base) & ATA_SR_DRQ))
        asm volatile("hlt");
}

/* =========================================================
                RESPONSE HANDLING FUNCTIONS
   ========================================================= */

/**
 * Function: ata_handle_identify_responce
 * -------------------------------------
 * Validates the IDENTIFY command response and reads the
 * 512-byte identify block into memory (256 words of 2 bytes).
 *
 * Returns: 1 on error, 0 on success
 */
static uint8_t ata_handle_identify_responce() {
    uint8_t status = inb(ata_responce.device_id.io_base + ATA_REG_STATUS);
    uint8_t LBAmid = inb(ata_responce.device_id.io_base + ATA_REG_LBA_MID);
    uint8_t LBAhi  = inb(ata_responce.device_id.io_base + ATA_REG_LBA_HIGH);
    uint8_t err    = 0;

    // Drive not detected
    if (status == 0) {
        err = 1;
        printf("ATA: No device detected on the port\n");
        goto done;
    }

    // Not an ATA device
    if (LBAmid != 0 || LBAhi != 0) {
        err = 1;
        printf("ATA: Device is not ATA compatible\n");
        goto done;
    }

    // Wait until DRQ (ready for data) or ERR (failure)
    do {
        status = inb(ata_responce.device_id.io_base + ATA_REG_STATUS);
    } while (!(status & ATA_SR_DRQ) && !(status & ATA_SR_ERR));

    if (status & ATA_SR_ERR) {
        err = 1;
        printf("ATA: IDENTIFY failed, error flag raised\n");
        goto done;
    }

    // Read 512-byte IDENTIFY data (256 * 2-byte words)
    for (uint32_t i = 0; i < 256; i++)
        ((uint16_t *)ata_responce.specific.read_resp.buffer)[i] =
             inw(ata_responce.device_id.io_base + ATA_REG_DATA);

done:
    ata_responce.error = err;
    return err;
}

/**
 * Function: ata_handle_read_single_sector_responce
 * -------------------------------------
 * Reads one 512-byte sector chunk during an IRQ.
 * Stores it into the provided RAM buffer sequentially.
 */
static uint8_t ata_handle_read_single_sector_responce() {
    uint32_t offset_words = (ATA_SECTOR_SIZE / 2) * ata_responce.specific.read_resp.sector_index;

    // Read 512 bytes = 256 16-bit values
    for (uint32_t i = 0; i < (ATA_SECTOR_SIZE / 2); i++) {
        uint16_t t = inw(ata_responce.device_id.io_base + ATA_REG_DATA);
        ((uint16_t*)ata_responce.specific.read_resp.buffer)[offset_words + i] =
             t;
    }

    ata_responce.specific.read_resp.sector_index++;
    return 0;
}

/**
 * Function: ata_handle_write_single_sector_responce
 * -------------------------------------
 * Marks that a sector write operation completed.
 */
static uint8_t ata_handle_write_single_sector_responce() {
    ata_responce.specific.write_resp.sector_write_done = 1;
    return 0;
}

/* =========================================================
                        DRIVER INIT
   ========================================================= */

/**
 * Function: initiate_ata_driver
 * -------------------------------------
 * Initializes request/response state and registers handlers
 * for ATA primary/secondary device IRQ lines (46,47).
 */
void initiate_ata_driver() {
    memset((void*)&ata_request, 0, sizeof(ata_request_t));
    memset((void*)&ata_responce, 0, sizeof(ata_responce_t));

    register_interrupt_handler(46, ata_response_handler);
    register_interrupt_handler(47, ata_response_handler);
}

/* =========================================================
                 COMMAND SENDING FUNCTIONS
   ========================================================= */

/**
 * Function: ata_send_identify_command
 * -------------------------------------
 * Sends the IDENTIFY command to an ATA device and waits for
 * the IRQ handler to copy the response into memory.
 *
 * Returns: 1 on error, 0 on success
 */
uint8_t ata_send_identify_command(ata_drive_t *drive, identify_device_data_t *buffer) {
    ata_request.device_id  = drive->device_id;
    ata_request.pending    = 0;
    ata_request.request_type = ATA_CMD_IDENTIFY;

    ata_responce.device_id   = drive->device_id;
    ata_responce.done        = 0;
    ata_responce.error       = 0;
    ata_responce.specific.read_resp.sector_index = 0;
    ata_responce.specific.read_resp.buffer  = (uint8_t*)buffer;

    // Select device
    outb(drive->device_id.io_base + ATA_REG_HDDEVSEL, 0xE0 | (drive->device_id.master << 4));
    outb(drive->device_id.io_base + ATA_REG_SECCOUNT, 0);
    outb(drive->device_id.io_base + ATA_REG_LBA_LOW,  0);
    outb(drive->device_id.io_base + ATA_REG_LBA_MID,  0);
    outb(drive->device_id.io_base + ATA_REG_LBA_HIGH, 0);
    ATA_SELECT_DELAY();

    // Send IDENTIFY
    ata_request.pending = 1;
    outb(drive->device_id.io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    

    WAIT_FOR_ATA_RESPONSE_TO_BE_DONE();
    return ata_responce.error;
}

/**
 * Function: ata_read28_request
 * -------------------------------------
 * Sends a 28-bit LBA sector read request.
 */
uint8_t ata_read28_request(ata_drive_t *drive, uint32_t sector, uint8_t count, uint8_t *buffer) {
    ata_request.device_id  = drive->device_id;
    ata_request.pending    = 0;
    ata_request.request_type = ATA_CMD_READ_PIO;
    ata_request.specific.read.sector_address = sector;
    ata_request.specific.read.sector_count   = count;

    ata_responce.device_id   = drive->device_id;
    ata_responce.done        = 0;
    ata_responce.error       = 0;
    ata_responce.specific.read_resp.sector_index = sector;
    ata_responce.specific.read_resp.buffer = buffer;

    // Select device + LBA high bits
    outb(drive->device_id.io_base + ATA_REG_HDDEVSEL, 0xE0 | (drive->device_id.master<<4) | ((sector>>24)&0x0F));
    outb(drive->device_id.io_base + ATA_REG_FEATURES, 0);  // send Null (0) to the feature register (don't know why)
    outb(drive->device_id.io_base + ATA_REG_SECCOUNT, count);
    outb(drive->device_id.io_base + ATA_REG_LBA_LOW,  sector & 0xFF);
    outb(drive->device_id.io_base + ATA_REG_LBA_MID, (sector >> 8) & 0xFF);
    outb(drive->device_id.io_base + ATA_REG_LBA_HIGH,(sector >>16) & 0xFF);

    // Send READ command
    ata_request.pending = 1;
    ATA_SELECT_DELAY();
    outb(drive->device_id.io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    

    WAIT_FOR_ATA_RESPONSE_TO_BE_DONE();
    return ata_responce.error;
}

/**
 * Function: ata_write28_request
 * -------------------------------------
 * Sends a 28-bit LBA sector write request and writes the
 * sector data manually through PIO, waiting for IRQ after
 * each 512-byte sector completion.
 */
uint8_t ata_write28_request(ata_drive_t *drive, uint32_t sector, uint8_t count, uint8_t *buffer) {
    ata_request.device_id  = drive->device_id;
    ata_request.pending    = 0;
    ata_request.request_type = ATA_CMD_WRITE_PIO;
    ata_request.specific.write.sector_address = sector;
    ata_request.specific.write.sector_count   = count;
    ata_request.specific.write.data           = buffer;

    ata_responce.device_id  = drive->device_id;
    ata_responce.done       = 0;
    ata_responce.error      = 0;
    ata_responce.specific.write_resp.sector_write_done = 0;

    // Select device + LBA bits
    outb(drive->device_id.io_base + ATA_REG_HDDEVSEL, 0xE0 | (drive->device_id.master<<4) | ((sector>>24)&0x0F));
    outb(drive->device_id.io_base + 0x01, count); // not really sure why
    outb(drive->device_id.io_base + ATA_REG_SECCOUNT, count);
    outb(drive->device_id.io_base + ATA_REG_LBA_LOW,  sector & 0xFF);
    outb(drive->device_id.io_base + ATA_REG_LBA_MID, (sector >> 8) & 0xFF);
    outb(drive->device_id.io_base + ATA_REG_LBA_HIGH,(sector >>16) & 0xFF);

    // Send WRITE command
    ata_request.pending = 1;
    ATA_SELECT_DELAY();
    outb(drive->device_id.io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    

    // Write sector data (256 words per sector)
    for (uint32_t s = 0; s < count; s++) {
        ata_responce.specific.write_resp.sector_write_done = 0;
        uint16_t *src = (uint16_t*)buffer + (ATA_SECTOR_SIZE / 2)*s;

        for (uint32_t i = 0; i < (ATA_SECTOR_SIZE / 2); i++) {
            outw(drive->device_id.io_base + ATA_REG_DATA, src[i]);
            asm volatile("jmp .+2"); // short mandatory delay
        }

        // Wait for IRQ to mark sector done
        while (!ata_responce.specific.write_resp.sector_write_done)
             asm volatile("hlt");
    }

    return ata_responce.error;
}

/**
 * Function: ata_flush_cache
 * --------------------------------------
 * Sends ATA_CMD_FLUSH to make sure the disk commits its
 * internal write cache to physical media. This is important
 * after write operations before shutting down or reading
 * sectors that may still be cached.
 */
uint8_t ata_flush_cache(ata_drive_t *drive) {
    // Prepare driver state
    ata_request.device_id = drive->device_id;
    ata_request.pending = 0;
    ata_request.request_type = ATA_CMD_FLUSH;

    ata_responce.device_id = drive->device_id;
    ata_responce.done = 0;
    ata_responce.error = 0;

    // Select Master/Slave (LBA bits not used here)
    outb(drive->device_id.io_base + ATA_REG_HDDEVSEL, 0xE0 | (drive->device_id.master << 4));
    ATA_SELECT_DELAY();

    // Send flush command
    ata_request.pending = 1;
    outb(drive->device_id.io_base + ATA_REG_COMMAND, ATA_CMD_FLUSH);

    // FLUSH usually finishes without transferring a sector,
    // so polling BSY until clear is enough:
    ata_wait_bsy(drive);

    return ata_responce.error;
}

/* =========================================================
                   MAIN IRQ RESPONSE HANDLER
   ========================================================= */

/**
 * Function: ata_response_handler
 * -------------------------------------
 * Unified interrupt handler for all ATA requests.
 *   - Verifies a request is pending
 *   - Calls the correct sub-handler
 *   - Signals completion to waiting caller
 *   - Sends EOI to the :contentReference[oaicite:0]{index=0}
 */
static void ata_response_handler(registers_t *regs) {
    uint8_t st;

    if (!ata_request.pending) {
        printf("ATA: IRQ triggered but no request was pending!\n");
        goto irq_end;
    }

    ata_request.pending = 0;

    switch (ata_request.request_type) {
        case ATA_CMD_IDENTIFY:
            ata_responce.error = ata_handle_identify_responce();
            break;
        case ATA_CMD_READ_PIO:
            ata_handle_read_single_sector_responce();
            break;
        case ATA_CMD_WRITE_PIO:
            ata_handle_write_single_sector_responce();
            break;
        case ATA_CMD_FLUSH:
            // FLUSH does not read/write sectors. We only check flags.
            st = inb(ata_request.device_id.io_base + ATA_REG_STATUS);
            if (st & ATA_SR_ERR) ata_responce.error = 1;
            break;
        default:
            printf("ATA Drive Error: Unknown command\n");
            break;
    }

    ata_responce.done = 1;

irq_end:
    outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

/* =========================================================
                 IDENTIFY DATA PRINT FUNCTION
   ========================================================= */

/**
 * Function: print_identify_device_data
 * -------------------------------------
 * Prints decoded information from the 512-byte IDENTIFY block.
 * Helps debugging and confirming drive capabilities.
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
