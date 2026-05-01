#ifndef ATA_TEST_H
#define ATA_TEST_H

#include "drivers/ata_driver.h"

void ata_test_write_read_3_sectors(ata_drive_t *drive, uint32_t start_sector);

#endif // ATA_TEST_H