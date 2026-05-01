#include "tests/flatfs_test.h"
#include "tests/test_log.h"
#include "utils.h"

#define TEST_FILE_NAME "test.bin"
#define TEST_INODES 64
#define TEST_SECTORS_PER_BLOCK 1
#define TEST_SIZE 1500

void flatfs_test_basic(ata_drive_t *drive)
{
    flatfs_t fs;
    flatfs_err_t err;
    uint32_t inode_idx = 0;
    uint32_t bytes_written = 0;
    uint32_t bytes_read = 0;
    flatfs_inode_t st;

    uint8_t write_buf[TEST_SIZE];
    uint8_t read_buf[TEST_SIZE];

    TEST_LOG_TEST("FlatFS basic test start\n");

    memset(&fs, 0, sizeof(fs));
    memset(write_buf, 0, sizeof(write_buf));
    memset(read_buf, 0, sizeof(read_buf));

    TEST_LOG_STEP("Preparing test buffer\n");

    for (uint32_t i = 0; i < TEST_SIZE; i++) {
        write_buf[i] = (uint8_t)(i * 7 + 3);
    }

    TEST_LOG_STEP("Formatting filesystem\n");

    err = flatfs_format(drive, TEST_INODES, TEST_SECTORS_PER_BLOCK);
    if (err != FLATFS_OK) {
        TEST_LOG_ERR("Format failed err=%d\n", err);
        return;
    }

    TEST_LOG_OK("Format completed\n");

    TEST_LOG_STEP("Mounting filesystem\n");

    err = flatfs_mount(&fs, drive);
    if (err != FLATFS_OK) {
        TEST_LOG_ERR("Mount failed err=%d\n", err);
        return;
    }

    TEST_LOG_OK("Mount completed\n");

    TEST_LOG_STEP("Creating file '%s'\n", TEST_FILE_NAME);

    err = flatfs_create(&fs, TEST_FILE_NAME,
                        FLATFS_PERMISSION_R | FLATFS_PERMISSION_W,
                        &inode_idx);
    if (err != FLATFS_OK) {
        TEST_LOG_ERR("Create failed err=%d\n", err);
        return;
    }

    TEST_LOG_INFO("Created inode=%u\n", inode_idx);

    TEST_LOG_STEP("Writing %u bytes\n", TEST_SIZE);

    err = flatfs_write(&fs, TEST_FILE_NAME, 0,
                       write_buf, TEST_SIZE, &bytes_written);
    if (err != FLATFS_OK) {
        TEST_LOG_ERR("Write failed err=%d written=%u\n", err, bytes_written);
        return;
    }

    if (bytes_written != TEST_SIZE) {
        TEST_LOG_ERR("Wrong bytes_written=%u expected=%u\n",
                     bytes_written, TEST_SIZE);
        return;
    }

    TEST_LOG_OK("Write completed\n");

    memset(read_buf, 0, sizeof(read_buf));

    TEST_LOG_STEP("Reading back data\n");

    err = flatfs_read(&fs, TEST_FILE_NAME, 0,
                      read_buf, TEST_SIZE, &bytes_read);
    if (err != FLATFS_OK) {
        TEST_LOG_ERR("Read failed err=%d read=%u\n", err, bytes_read);
        return;
    }

    if (bytes_read != TEST_SIZE) {
        TEST_LOG_ERR("Wrong bytes_read=%u expected=%u\n",
                     bytes_read, TEST_SIZE);
        return;
    }

    TEST_LOG_STEP("Verifying data\n");

    for (uint32_t i = 0; i < TEST_SIZE; i++) {
        if (write_buf[i] != read_buf[i]) {
            TEST_LOG_ERR("Data mismatch at byte %u expected=0x%x got=0x%x\n",
                         i, write_buf[i], read_buf[i]);
            return;
        }
    }

    TEST_LOG_OK("Data verified\n");

    TEST_LOG_STEP("Reading file metadata\n");

    err = flatfs_stat(&fs, TEST_FILE_NAME, &st);
    if (err != FLATFS_OK) {
        TEST_LOG_ERR("Stat failed err=%d\n", err);
        return;
    }

    if (st.size != TEST_SIZE) {
        TEST_LOG_ERR("Wrong file size=%u expected=%u\n", st.size, TEST_SIZE);
        return;
    }

    TEST_LOG_OK("Stat verified\n");

    TEST_LOG_STEP("Unmounting filesystem\n");

    err = flatfs_unmount(&fs);
    if (err != FLATFS_OK) {
        TEST_LOG_ERR("Unmount failed err=%d\n", err);
        return;
    }

    TEST_LOG_OK("Unmount completed\n");

    memset(&fs, 0, sizeof(fs));
    memset(read_buf, 0, sizeof(read_buf));
    bytes_read = 0;

    TEST_LOG_STEP("Remounting filesystem\n");

    err = flatfs_mount(&fs, drive);
    if (err != FLATFS_OK) {
        TEST_LOG_ERR("Remount failed err=%d\n", err);
        return;
    }

    TEST_LOG_OK("Remount completed\n");

    TEST_LOG_STEP("Reading after remount\n");

    err = flatfs_read(&fs, TEST_FILE_NAME, 0,
                      read_buf, TEST_SIZE, &bytes_read);
    if (err != FLATFS_OK) {
        TEST_LOG_ERR("Read after remount failed err=%d read=%u\n",
                     err, bytes_read);
        return;
    }

    if (bytes_read != TEST_SIZE) {
        TEST_LOG_ERR("Wrong remount bytes_read=%u expected=%u\n",
                     bytes_read, TEST_SIZE);
        return;
    }

    TEST_LOG_STEP("Verifying persistence data\n");

    for (uint32_t i = 0; i < TEST_SIZE; i++) {
        if (write_buf[i] != read_buf[i]) {
            TEST_LOG_ERR("Remount mismatch at byte %u expected=0x%x got=0x%x\n",
                         i, write_buf[i], read_buf[i]);
            return;
        }
    }

    TEST_LOG_OK("Persistence verified\n");
    TEST_LOG_TEST("PASS - FlatFS basic test succeeded\n");

    flatfs_unmount(&fs);
}