#include "drivers/flatfs/flatfs_driver.h"

flatfs_err_t flatfs_write_block(flatfs_t *fs, 
                                uint32_t block_idx, 
                                const uint8_t *data) {
    if (!fs || !data)
        return FLATFS_ERR_INVALID;
    
    if (block_idx >= fs->sb.total_blocks)
        return FLATFS_ERR_INVALID;

    uint8_t err = ata_write28_request(fs->drive, 
                                    block_idx * fs->sb.sectors_per_block, 
                                    fs->sb.sectors_per_block,
                                    data);
    
    return (err == 1)? FLATFS_ERR_IO : FLATFS_OK;
}

flatfs_err_t flatfs_read_block(flatfs_t *fs, uint32_t block_idx, uint8_t *buffer) {
    if (!fs || !buffer)
        return FLATFS_ERR_INVALID;
    
    if (block_idx >= fs->sb.total_blocks)
        return FLATFS_ERR_INVALID;
    
    uint8_t err = ata_read28_request(fs->drive, 
                                    block_idx * fs->sb.sectors_per_block, 
                                    fs->sb.sectors_per_block,
                                    buffer);
    
    return (err == 1)? FLATFS_ERR_IO : FLATFS_OK;
}