#include "drivers/ata_driver.h"
#include "kernel/print.h"
#include "utils.h"

void ata_test_write_read_3_sectors(ata_drive_t *drive, uint32_t start_sector)
{
    uint8_t write_buf[ATA_SECTOR_SIZE * 3];
    uint8_t read_buf[ATA_SECTOR_SIZE * 3];

    memset(write_buf, 0, sizeof(write_buf));
    memset(read_buf, 0, sizeof(read_buf));

    /* Fill each sector with a different pattern */
    for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i++) {
        write_buf[i] = 0xAA;                         /* sector 0 */
        write_buf[ATA_SECTOR_SIZE + i] = 0xBB;       /* sector 1 */
        write_buf[ATA_SECTOR_SIZE * 2 + i] = 0xCC;   /* sector 2 */
    }

    printf("ATA TEST: writing 3 sectors at LBA %u\n", start_sector);

    uint8_t err = ata_write28_request(drive, start_sector, 3, write_buf);
    if (err != 0) {
        printf("ATA TEST: write failed, err=%u\n", err);
        return;
    }

    err = ata_flush_cache(drive);
    if (err != 0) {
        printf("ATA TEST: flush failed, err=%u\n", err);
        return;
    }

    printf("ATA TEST: reading 3 sectors back\n");

    err = ata_read28_request(drive, start_sector, 3, read_buf);
    if (err != 0) {
        printf("ATA TEST: read failed, err=%u\n", err);
        return;
    }

    for (uint32_t i = 0; i < sizeof(write_buf); i++) {
        if (write_buf[i] != read_buf[i]) {
            printf("ATA TEST: mismatch at byte %u\n", i);
            printf("expected=0x%x actual=0x%x\n",
                   write_buf[i],
                   read_buf[i]);
            return;
        }
    }

    printf("ATA TEST: PASS - 3 sectors written and read correctly\n");
}