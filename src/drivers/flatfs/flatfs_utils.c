#include "drivers/flatfs/flatfs_driver.h"

flatfs_err_t flatfs_write_blocks(flatfs_t *fs,
                                  uint32_t start_block_idx,
                                  uint32_t block_count,
                                  const uint8_t *data) {
    if (!fs || !data)
        return FLATFS_ERR_INVALID;
    
    uint32_t total_drive_blocks = fs->sb.total_sectors / fs->sb.sectors_per_block;

    if (start_block_idx + block_count >= total_drive_blocks)
        return FLATFS_ERR_INVALID;

    uint8_t err = ata_write28_request(fs->drive,
                                      start_block_idx * fs->sb.sectors_per_block,
                                      block_count * fs->sb.sectors_per_block,
                                      (uint8_t *)data);

    return (err == 1) ? FLATFS_ERR_IO : FLATFS_OK;
}

flatfs_err_t flatfs_read_blocks(flatfs_t *fs,
                                 uint32_t start_block_idx,
                                 uint32_t block_count,
                                 uint8_t *buffer) {
    if (!fs || !buffer)
        return FLATFS_ERR_INVALID;

    uint32_t total_drive_blocks = fs->sb.total_sectors / fs->sb.sectors_per_block;

    if (start_block_idx + block_count >= total_drive_blocks)
        return FLATFS_ERR_INVALID;

    uint8_t err = ata_read28_request(fs->drive,
                                     start_block_idx * fs->sb.sectors_per_block,
                                     block_count * fs->sb.sectors_per_block,
                                     buffer);

    return (err == 1) ? FLATFS_ERR_IO : FLATFS_OK;
}