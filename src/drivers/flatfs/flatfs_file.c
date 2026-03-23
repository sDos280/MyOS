#include "drivers/flatfs/flatfs_driver.h"
#include "bitmap_util.h"
#include "utils.h"

flatfs_err_t flatfs_create(flatfs_t *fs, const char *name,
                            uint8_t permissions, uint32_t *inode_idx) {
    if (!fs || !name || !inode_idx)
        return FLATFS_ERR_INVALID;

    flatfs_inode_t inode;

    /* check if file with the same name already exists */
    for (uint32_t i = 0; i < fs->sb.total_inodes; i++) {
        if (ata_read28_request(fs->drive, FLATFS_SECTOR_INODE_TABLE + i,
                               1, (uint8_t *)&inode) != 0)
            return FLATFS_ERR_IO;

        if (inode.in_use &&
            strncmp(inode.name, name, FLATFS_NAME_MAX) == 0)
            return FLATFS_ERR_EXISTS;
    }

    /* find free inode */
    uint32_t ino;
    if (!bitmap_find_first_clear(fs->inode_bitmap, FLATFS_MAX_FILES, &ino))
        return FLATFS_ERR_NO_SPACE;

    /* find free block */
    uint32_t block_idx;
    if (!bitmap_find_first_clear(fs->block_bitmap, FLATFS_MAX_BLOCKS, &block_idx))
        return FLATFS_ERR_NO_SPACE;

    /* mark inode and block as used in bitmaps */
    bitmap_set(fs->inode_bitmap, ino);
    bitmap_set(fs->block_bitmap, block_idx);

    /* initialize inode */
    memset(&inode, 0, sizeof(flatfs_inode_t));
    strncpy(inode.name, name, FLATFS_NAME_MAX);
    inode.in_use      = 1;
    inode.permissions = permissions;
    inode.size        = 0;
    inode.block_count = 1;
    memset(inode.blocks, 0, sizeof(inode.blocks));
    inode.blocks[0]   = FLATFS_SECTOR_DATA_START + block_idx;

    /* write inode to disk */
    if (ata_write28_request(fs->drive, FLATFS_SECTOR_INODE_TABLE + ino,
                            1, (uint8_t *)&inode) != 0)
        return FLATFS_ERR_IO;

    /* write updated bitmaps back to disk */
    if (ata_write28_request(fs->drive, FLATFS_SECTOR_INODE_BITMAP,
                            1, fs->inode_bitmap) != 0)
        return FLATFS_ERR_IO;

    if (ata_write28_request(fs->drive, FLATFS_SECTOR_BLOCK_BITMAP,
                            1, fs->block_bitmap) != 0)
        return FLATFS_ERR_IO;

    *inode_idx = ino;
    return FLATFS_OK;
}

flatfs_err_t flatfs_delete(flatfs_t *fs, const char *name) {
    if (!fs || !name)
        return FLATFS_ERR_INVALID;

    uint8_t err;
    flatfs_inode_t inode;
    uint32_t i;
    uint8_t found = 0;

    for (i = 0; i < fs->sb.total_inodes; i++) {
        err = ata_read28_request(fs->drive, FLATFS_SECTOR_INODE_TABLE + i,
                                 1, (uint8_t *)&inode);
        if (err != 0)
            return FLATFS_ERR_IO;

        if (inode.in_use &&
            strncmp(inode.name, name, FLATFS_NAME_MAX) == 0) {
            found = 1;
            break;
        }
    }

    if (!found)
        return FLATFS_ERR_NOT_FOUND;

    bitmap_clear(fs->inode_bitmap, i);

    for (uint32_t j = 0; j < inode.block_count; j++)
        bitmap_clear(fs->block_bitmap, inode.blocks[j]);

    inode.in_use = 0;
    fs->sb.free_inodes++;
    fs->sb.free_blocks += inode.block_count;

    if (ata_write28_request(fs->drive, FLATFS_SECTOR_INODE_TABLE + i,
                            1, (uint8_t *)&inode) != 0)
        return FLATFS_ERR_IO;

    if (ata_write28_request(fs->drive, FLATFS_SECTOR_INODE_BITMAP,
                            1, fs->inode_bitmap) != 0)
        return FLATFS_ERR_IO;

    if (ata_write28_request(fs->drive, FLATFS_SECTOR_BLOCK_BITMAP,
                            1, fs->block_bitmap) != 0)
        return FLATFS_ERR_IO;

    if (ata_flush_cache(fs->drive) != 0)
        return FLATFS_ERR_IO;

    return FLATFS_OK;
}