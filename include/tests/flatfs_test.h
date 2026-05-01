#ifndef FLATFS_TEST_H
#define FLATFS_TEST_H

#include "drivers/flatfs/flatfs_driver.h"

#define TEST_FILE_NAME "test.bin"
#define TEST_INODES 64
#define TEST_SECTORS_PER_BLOCK 1
#define TEST_SIZE 1500

void flatfs_test_basic(ata_drive_t *drive);

#endif // FLATFS_TEST_H