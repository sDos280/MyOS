#include "drivers/flatfs/flatfs_driver.h"
#include "mm/kheap.h"
#include "bitmap_util.h"
#include "utils.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static flatfs_err_t get_free_data_block(flatfs_t *fs, uint32_t *block_index);
static flatfs_err_t read_inode(flatfs_t *fs, uint32_t inode_idx, flatfs_inode_t *inode);
static flatfs_err_t write_inode(flatfs_t *fs, uint32_t inode_idx, flatfs_inode_t *inode);
static flatfs_err_t flush_inode_bitmap(flatfs_t *fs);
static flatfs_err_t flush_block_bitmap(flatfs_t *fs);
static flatfs_err_t flush_bitmaps(flatfs_t *fs);

flatfs_err_t flatfs_create(flatfs_t *fs, const char *name,
                            uint8_t permissions, uint32_t *inode_idx) {
    if (!fs || !name || !inode_idx)
        return FLATFS_ERR_INVALID;

    flatfs_inode_t inode;

    /* check if file with the same name already exists (pass inode index for function to work properly)*/
    flatfs_err_t err = flatfs_find(fs, name, inode_idx);
    if (err == FLATFS_OK)
        return FLATFS_ERR_EXISTS;
    
    if (err != FLATFS_ERR_NOT_FOUND)
        return err;

    /* find free inode */
    uint32_t ino;
    if (!bitmap_find_first_clear(fs->inode_bitmap, fs->sb.total_inodes, &ino))
        return FLATFS_ERR_NO_SPACE;

    /* find free block */
    uint32_t block_idx;
    if (!bitmap_find_first_clear(fs->block_bitmap, fs->sb.total_blocks, &block_idx))
        return FLATFS_ERR_NO_SPACE;

    /* mark inode and block as used in bitmaps */
    bitmap_set(fs->inode_bitmap, ino);
    bitmap_set(fs->block_bitmap, block_idx);

    fs->sb.free_inodes--;
    fs->sb.free_blocks--;

    /* initialize inode */
    memset(&inode, 0, sizeof(flatfs_inode_t));
    strncpy(inode.name, name, FLATFS_NAME_MAX);
    inode.in_use      = 1;
    inode.permissions = permissions;
    inode.size        = 0;
    inode.block_count = 1;
    memset(inode.blocks, 0, sizeof(inode.blocks));
    inode.blocks[0]   = fs->sb.data_start_block + block_idx;

    /* write inode to disk */
    *inode_idx = ino;
    
    flush_bitmaps(fs);

    return write_inode(fs, ino, &inode);
}

flatfs_err_t flatfs_delete(flatfs_t *fs, const char *name) {
    if (!fs || !name)
        return FLATFS_ERR_INVALID;

    flatfs_inode_t inode;
    uint8_t found = 0;

    /* check if file with the same name already exists (pass inode index for function to work properly)*/
    uint32_t inode_idx;
    flatfs_err_t err = flatfs_find(fs, name, &inode_idx);
    if (err != FLATFS_OK)
        return err;
    
    /* get inode */
    err = flatfs_get_inode_by_index(fs, inode_idx, &inode);
    if (err != FLATFS_OK)
            return err;

    /* mark inode as free */
    inode.in_use = 0;
    err = write_inode(fs, inode_idx, &inode);
    if (err != FLATFS_OK)
        return err;

    /* free inode and data blocks in bitmaps */
    bitmap_clear(fs->inode_bitmap, inode_idx);
    for (uint32_t j = 0; j < inode.block_count; j++)
        bitmap_clear(fs->block_bitmap, inode.blocks[j]);

    /* update superblock counts */
    fs->sb.free_inodes++;
    fs->sb.free_blocks += inode.block_count;

    /* flush bitmaps to disk */
    err = flush_bitmaps(fs);
    if (err != FLATFS_OK)
        return err;

    return FLATFS_OK;
}

flatfs_err_t flatfs_write(flatfs_t *fs,
                          const char *name,
                          uint32_t offset,
                          const uint8_t *buf,
                          uint32_t size,
                          uint32_t *bytes_written) {
    if (!fs || !name || !buf || !bytes_written)
        return FLATFS_ERR_INVALID;
    
    if (size == 0) {
        if (bytes_written) *bytes_written = 0;
        return FLATFS_OK;
    }

    if (offset + size > fs->drive->size_in_sectors * ATA_SECTOR_SIZE)
        return FLATFS_ERR_NO_SPACE;

    uint32_t inode_idx;
    flatfs_err_t err = flatfs_find(fs, name, &inode_idx);
    if (err != FLATFS_OK)
        return err;

    flatfs_inode_t inode;
    err = flatfs_get_inode_by_index(fs, inode_idx, &inode);
    if (err != FLATFS_OK)
        return err;

    /* allocate any blocks that are needed but not yet assigned */
    uint32_t block_size       = FLATFS_BLOCK_SIZE(&fs->sb);
    uint32_t last_block_needed = (offset + size - 1) / block_size;
    
    /* check we don't exceed direct block limit */
    if (last_block_needed >= FLATFS_DIRECT_BLOCKS)
        return FLATFS_ERR_NO_SPACE;

    while (inode.block_count <= last_block_needed) {
        uint32_t free_block_idx;
        err = get_free_data_block(fs, &free_block_idx);
        if (err != FLATFS_OK)
            return err;

        inode.blocks[inode.block_count] = free_block_idx;
        inode.block_count++;
    }
    uint8_t *block_buf = kalloc(block_size);
    if (!block_buf) {
        kfree(block_buf);
        return FLATFS_ERR_NO_MEM;
    }
    
    /* read-modify-write each sector touched by this write */
    uint32_t local_bytes_written = 0;
    while (local_bytes_written < size) {
        uint32_t block_idx    = (offset + local_bytes_written) / block_size;
        uint32_t local_offset = (offset + local_bytes_written) % block_size;
        uint32_t local_size   = MIN(block_size - local_offset, size - local_bytes_written);

        

        uint32_t phys_block = fs->sb.data_start_block + inode.blocks[block_idx];
        
        if ((err = flatfs_read_blocks(fs, phys_block, 1, block_buf)) != FLATFS_OK)
            return err;

        memcpy(block_buf + local_offset, buf + local_bytes_written, local_size);
        
        if ((err = flatfs_write_blocks(fs, phys_block, 1, block_buf)) != FLATFS_OK)
            return err;

        local_bytes_written += local_size;
        if (bytes_written)
            *bytes_written = local_bytes_written;
    }

    kfree(block_buf);

    /* update inode size if write extended the file */
    if (offset + size > inode.size)
        inode.size = offset + size;

    /* write inode back to disk */
    if ((err = write_inode(fs, inode_idx, &inode)) != FLATFS_OK)
        return err;
    
    return FLATFS_OK;
}

flatfs_err_t flatfs_read(flatfs_t *fs,
                         const char *name,
                         uint32_t offset,
                         uint8_t *buf,
                         uint32_t size,
                         uint32_t *bytes_read) {
    if (!fs || !name || !buf || !bytes_read)
        return FLATFS_ERR_INVALID;

    uint32_t inode_idx;
    flatfs_err_t err = flatfs_find(fs, name, &inode_idx);
    if (err != FLATFS_OK)
        return err;

    flatfs_inode_t inode;
    err = flatfs_get_inode_by_index(fs, inode_idx, &inode);
    if (err != FLATFS_OK)
        return err;

    if (offset >= inode.size) {
        *bytes_read = 0;
        return FLATFS_OK;
    }

    size = MIN(size, inode.size - offset);

    uint32_t br = 0;
    while (br < size) {
        uint32_t block_idx    = (offset + br) / FLATFS_BLOCK_SIZE;
        uint32_t local_offset = (offset + br) % FLATFS_BLOCK_SIZE;
        uint32_t local_size   = MIN(FLATFS_BLOCK_SIZE - local_offset, size - br);

        uint8_t block_buf[FLATFS_BLOCK_SIZE];

        if (ata_read28_request(fs->drive, FLATFS_SECTOR_DATA_START + inode.blocks[block_idx],
                               1, block_buf) != 0)
            return FLATFS_ERR_IO;

        memcpy(buf + br, block_buf + local_offset, local_size);

        br += local_size;
    }

    *bytes_read = br;
    return FLATFS_OK;
}

flatfs_err_t flatfs_truncate(flatfs_t *fs,
                             const char *name,
                             uint32_t new_size) {
    if (!fs || !name)
        return FLATFS_ERR_INVALID;

    flatfs_inode_t inode;
    flatfs_err_t err = flatfs_stat(fs, name, &inode);
    if (err != FLATFS_OK)
        return err;

    if (new_size > inode.size)
        return FLATFS_ERR_INVALID;

    uint32_t new_block_count =
        (new_size == 0) ? 0 : ((new_size + FLATFS_BLOCK_SIZE - 1) / FLATFS_BLOCK_SIZE);

    for (uint32_t i = new_block_count; i < inode.block_count; i++) {
        bitmap_clear(fs->block_bitmap, inode.blocks[i]);
        fs->sb.free_blocks++;
    }

    inode.block_count = new_block_count;
    inode.size = new_size;

    return FLATFS_OK;
}

flatfs_err_t flatfs_rename(flatfs_t *fs,
                           const char *old_name,
                           const char *new_name) {
    if (!fs || !old_name || !new_name)
        return FLATFS_ERR_INVALID;

    /* check if there is a file with the new_name */
    uint32_t inox;
    flatfs_err_t err = flatfs_find(fs, new_name, &inox);
    if (err == FLATFS_OK)
        return FLATFS_ERR_EXISTS;
    if (err != FLATFS_ERR_NOT_FOUND)
        return err;

    /* find the old name file */
    flatfs_inode_t inode;
    err = flatfs_find(fs, old_name, &inox);
    if (err != FLATFS_OK)
        return err;

    if (ata_read28_request(fs->drive,
                           FLATFS_SECTOR_INODE_TABLE + inox,
                           1,
                           (uint8_t *)&inode) != 0)
        return FLATFS_ERR_IO;

    memset(inode.name, 0, FLATFS_NAME_MAX);
    strncpy(inode.name, new_name, FLATFS_NAME_MAX);

    if (ata_write28_request(fs->drive,
                            FLATFS_SECTOR_INODE_TABLE + inox,
                            1,
                            (uint8_t *)&inode) != 0)
        return FLATFS_ERR_IO;

    return FLATFS_OK;
}

/* =========================================================================
 * INTERNAL HELPERS
 * ========================================================================= */

/*
 * Get a free data block (if there is one).
 */
static flatfs_err_t get_free_data_block(flatfs_t *fs, uint32_t *block_index) {
    if (!fs || !block_index)
        return FLATFS_ERR_INVALID;

    if (bitmap_find_first_clear(fs->block_bitmap, fs->sb.total_blocks, block_index) == 0)
        return FLATFS_ERR_NO_SPACE;

    bitmap_set(fs->block_bitmap, *block_index);

    flush_block_bitmap(fs);
    
    fs->sb.free_blocks--;

    return FLATFS_OK;
}

/*
 * Read inode (by index) from inode table
 */
static flatfs_err_t read_inode(flatfs_t *fs, uint32_t inode_idx, flatfs_inode_t *inode) {
    if (!fs || !inode)
        return FLATFS_ERR_INVALID;
    
    if (inode_idx >= fs->sb.total_inodes)
        return FLATFS_ERR_INVALID;
    
    uint32_t byte_offset = inode_idx * sizeof(flatfs_inode_t);
    uint32_t first_block = byte_offset / FLATFS_BLOCK_SIZE(&fs->sb);
    /* check if data span two blocks, that is the case, if last byte of inode is in next block */
    uint32_t last_block  = (byte_offset + sizeof(flatfs_inode_t) - 1) / FLATFS_BLOCK_SIZE(&fs->sb);
    uint32_t block_count = (first_block != last_block) ? 2 : 1;
    uint32_t block_idx   = fs->sb.inode_table_start + first_block;

    uint8_t *buf = (uint8_t *)kmalloc(FLATFS_BLOCK_SIZE(&fs->sb) * block_count);
    if (!buf)
        return FLATFS_ERR_NO_MEM;

    flatfs_err_t err = flatfs_read_blocks(fs, block_idx, block_count, buf);
    if (err != FLATFS_OK) {
        kfree(buf);
        return err;
    }

    memcpy(inode, buf + (byte_offset % FLATFS_BLOCK_SIZE(&fs->sb)), sizeof(flatfs_inode_t));

    kfree(buf);
    return FLATFS_OK;
}

/*
 * Write inode (by index) from inode table
 */
static flatfs_err_t write_inode(flatfs_t *fs, uint32_t inode_idx, flatfs_inode_t *inode) {
    if (!fs || !inode)
        return FLATFS_ERR_INVALID;
    
    if (inode_idx >= fs->sb.total_inodes)
        return FLATFS_ERR_INVALID;
    
    uint32_t byte_offset = inode_idx * sizeof(flatfs_inode_t);
    uint32_t first_block = byte_offset / FLATFS_BLOCK_SIZE(&fs->sb);
    /* check if data span two blocks, that is the case, if last byte of inode is in next block */
    uint32_t last_block  = (byte_offset + sizeof(flatfs_inode_t) - 1) / FLATFS_BLOCK_SIZE(&fs->sb);
    uint32_t block_count = (first_block != last_block) ? 2 : 1;
    uint32_t block_idx   = fs->sb.inode_table_start + first_block;

    uint8_t *buf = (uint8_t *)kmalloc(FLATFS_BLOCK_SIZE(&fs->sb) * block_count);
    if (!buf)
        return FLATFS_ERR_NO_MEM;

    flatfs_err_t err = flatfs_read_blocks(fs, block_idx, block_count, buf);
    if (err != FLATFS_OK) {
        kfree(buf);
        return err;
    }

    memcpy(buf + (byte_offset % FLATFS_BLOCK_SIZE(&fs->sb)), inode, sizeof(flatfs_inode_t));

    err = flatfs_write_blocks(fs, block_idx, block_count, buf);
    if (err != FLATFS_OK) {
        kfree(buf);
        return err;
    }

    kfree(buf);
    return FLATFS_OK;
}

static flatfs_err_t flush_inode_bitmap(flatfs_t *fs) {
    return flatfs_write_blocks(fs, fs->sb.inode_bitmap_start,
                               fs->sb.inode_bitmap_block_count,
                               fs->inode_bitmap);
}

static flatfs_err_t flush_block_bitmap(flatfs_t *fs) {
    return flatfs_write_blocks(fs, fs->sb.block_bitmap_start,
                               fs->sb.block_bitmap_block_count,
                               fs->block_bitmap);
}

static flatfs_err_t flush_bitmaps(flatfs_t *fs) {
    flatfs_err_t err = flush_inode_bitmap(fs);
    if (err != FLATFS_OK)
        return err;

    return flush_block_bitmap(fs);
}