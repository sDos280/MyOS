#include "drivers/flatfs/flatfs_driver.h"
#include "bitmap_util.h"
#include "utils.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static flatfs_err_t get_file_ino_by_name(flatfs_t *fs, const char *name, uint32_t * inode_idx);
static flatfs_err_t get_inode_by_index(flatfs_t *fs, uint32_t inode_idx, flatfs_inode_t *inode);

flatfs_err_t flatfs_create(flatfs_t *fs, const char *name,
                            uint8_t permissions, uint32_t *inode_idx) {
    if (!fs || !name || !inode_idx)
        return FLATFS_ERR_INVALID;

    flatfs_inode_t inode;

    /* check if file with the same name already exists (pass inode index for function to work properly)*/
    flatfs_err_t err = get_file_ino_by_name(fs, name, inode_idx);
    if (err == FLATFS_OK)
        return FLATFS_ERR_EXISTS;
    
    if (err != FLATFS_ERR_NOT_FOUND)
        return err;

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

    /* check if file with the same name already exists (pass inode index for function to work properly)*/
    uint32_t inode_idx;
    flatfs_err_t err = get_file_ino_by_name(fs, name, &inode_idx);
    if (err != FLATFS_OK)
        return err;
    
    /* get inode */
    err = get_inode_by_index(fs, inode_idx, &inode);
    if (err != FLATFS_OK)
            return err;

    bitmap_clear(fs->inode_bitmap, inode_idx);

    for (uint32_t j = 0; j < inode.block_count; j++)
        bitmap_clear(fs->block_bitmap, inode.blocks[j]);

    inode.in_use = 0;
    fs->sb.free_inodes++;
    fs->sb.free_blocks += inode.block_count;

    /* write back changes */
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

flatfs_err_t flatfs_write(flatfs_t *fs,
                          const char *name,
                          uint32_t offset,
                          const uint8_t *buf,
                          uint32_t size) {
    if (!fs || !name || !buf)
        return FLATFS_ERR_INVALID;

    if (offset + size > FLATFS_DIRECT_BLOCKS * FLATFS_BLOCK_SIZE)
        return FLATFS_ERR_NO_SPACE;

    uint32_t inode_idx;
    flatfs_err_t err = get_file_ino_by_name(fs, name, &inode_idx);
    if (err != FLATFS_OK)
        return err;

    flatfs_inode_t inode;
    err = get_inode_by_index(fs, inode_idx, &inode);
    if (err != FLATFS_OK)
        return err;

    /* allocate any blocks that are needed but not yet assigned */
    uint32_t last_block_needed = (offset + size - 1) / FLATFS_BLOCK_SIZE;
    while (inode.block_count <= last_block_needed) {
        uint32_t free_block_idx;
        err = get_free_block(fs, &free_block_idx);
        if (err != FLATFS_OK)
            return err;

        inode.blocks[inode.block_count] = free_block_idx;
        inode.block_count++;
        fs->sb.free_blocks--;
    }

    /* read-modify-write each sector touched by this write */
    uint32_t bytes_written = 0;
    while (bytes_written < size) {
        uint32_t block_idx    = (offset + bytes_written) / FLATFS_BLOCK_SIZE;
        uint32_t local_offset = (offset + bytes_written) % FLATFS_BLOCK_SIZE;
        uint32_t local_size   = MIN(FLATFS_BLOCK_SIZE - local_offset, size - bytes_written);

        uint8_t block_buf[FLATFS_BLOCK_SIZE];

        if (ata_read28_request(fs->drive, FLATFS_SECTOR_DATA_START + inode.blocks[block_idx],
                               1, block_buf) != 0)
            return FLATFS_ERR_IO;

        memcpy(block_buf + local_offset, buf + bytes_written, local_size);

        if (ata_write28_request(fs->drive, FLATFS_SECTOR_DATA_START + inode.blocks[block_idx],
                                1, block_buf) != 0)
            return FLATFS_ERR_IO;

        bytes_written += local_size;
    }

    /* update inode size if write extended the file */
    if (offset + size > inode.size)
        inode.size = offset + size;

    /* write inode back to disk */
    if (ata_write28_request(fs->drive, FLATFS_SECTOR_INODE_TABLE + inode_idx,
                            1, (uint8_t *)&inode) != 0)
        return FLATFS_ERR_IO;

    return FLATFS_OK;
}

/* =========================================================================
 * INTERNAL HELPERS
 * ========================================================================= */
/*
 * Find (if exists) the inode index of a file with the passed name.
 */
static flatfs_err_t get_file_ino_by_name(flatfs_t *fs, const char *name, uint32_t * inode_idx) {
    if (!fs || !name || !inode_idx)
        return FLATFS_ERR_INVALID;
    
    flatfs_inode_t inode;

    for (uint32_t i = 0; i < fs->sb.total_inodes; i++) {
        if (ata_read28_request(fs->drive, FLATFS_SECTOR_INODE_TABLE + i,
                               1, (uint8_t *)&inode) != 0)
            return FLATFS_ERR_IO;

        if (inode.in_use && strncmp(inode.name, name, FLATFS_NAME_MAX) == 0) {
            *inode_idx = i;
            return FLATFS_OK;
        }
    }

    return FLATFS_ERR_NOT_FOUND;
}

/*
 * Get the inode data of the spesefied inode index.
 */
static flatfs_err_t get_inode_by_index(flatfs_t *fs, uint32_t inode_idx, flatfs_inode_t *inode) {
    if (!fs || !inode)
        return FLATFS_ERR_INVALID;

    if (bitmap_get(fs->inode_bitmap, inode_idx) == 0)
        return FLATFS_ERR_NOT_FOUND;

    if (ata_read28_request(fs->drive, FLATFS_SECTOR_INODE_TABLE + inode_idx,
                           1, (uint8_t *)inode) != 0)
        return FLATFS_ERR_IO;

    return FLATFS_OK;
}

/*
 * Get a free block (if there is one)
 */
static flatfs_err_t get_free_block(flatfs_t *fs, uint32_t *block_index) {
    if (!fs || !block_index)
        return FLATFS_ERR_INVALID;

    if (bitmap_find_first_clear(fs->block_bitmap, FLATFS_MAX_BLOCKS, block_index) == 0)
        return FLATFS_ERR_NO_SPACE;

    bitmap_set(fs->block_bitmap, *block_index);

    if (ata_write28_request(fs->drive, FLATFS_SECTOR_BLOCK_BITMAP,
                            1, fs->block_bitmap) != 0)
        return FLATFS_ERR_IO;

    return FLATFS_OK;
}