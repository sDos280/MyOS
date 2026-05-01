#include "tests/ata_test.h"
#include "tests/tests_helper.h"
#include "utils.h"

void ata_test_write_read_3_sectors(ata_drive_t *drive, uint32_t start_sector)
{
    uint8_t write_buf[ATA_SECTOR_SIZE * 3];
    uint8_t read_buf[ATA_SECTOR_SIZE * 3];

    TEST_LOG_TEST("ATA write/read 3 sectors start\n");

    memset(write_buf, 0, sizeof(write_buf));
    memset(read_buf, 0, sizeof(read_buf));

    TEST_LOG_STEP("Preparing test buffers\n");

    for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i++) {
        write_buf[i] = 0xAA;
        write_buf[ATA_SECTOR_SIZE + i] = 0xBB;
        write_buf[ATA_SECTOR_SIZE * 2 + i] = 0xCC;
    }

    TEST_LOG_STEP("Writing 3 sectors at LBA %u\n", start_sector);

    uint8_t err = ata_write28_request(drive, start_sector, 3, write_buf);
    if (err != 0) {
        TEST_LOG_ERR("ATA write failed err=%u\n", err);
        return;
    }

    TEST_LOG_OK("ATA write completed\n");

    TEST_LOG_STEP("Flushing ATA cache\n");

    err = ata_flush_cache(drive);
    if (err != 0) {
        TEST_LOG_ERR("ATA flush failed err=%u\n", err);
        return;
    }

    TEST_LOG_OK("ATA flush completed\n");

    TEST_LOG_STEP("Reading 3 sectors back from LBA %u\n", start_sector);

    err = ata_read28_request(drive, start_sector, 3, read_buf);
    if (err != 0) {
        TEST_LOG_ERR("ATA read failed err=%u\n", err);
        return;
    }

    TEST_LOG_OK("ATA read completed\n");

    TEST_LOG_STEP("Verifying data\n");

    for (uint32_t i = 0; i < sizeof(write_buf); i++) {
        if (write_buf[i] != read_buf[i]) {
            TEST_LOG_ERR("ATA mismatch at byte %u expected=0x%x actual=0x%x\n",
                         i, write_buf[i], read_buf[i]);
            return;
        }
    }

    TEST_LOG_OK("ATA data verified\n");
    TEST_LOG_TEST("PASS - ATA 3-sector write/read succeeded\n");
}