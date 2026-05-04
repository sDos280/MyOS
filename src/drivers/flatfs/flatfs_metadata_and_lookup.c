#include "drivers/flatfs/flatfs_driver.h"
#include "mm/kheap.h"
#include "utils/bitmap_util.h"
#include "utils/utils.h"

flatfs_err_t flatfs_stat(flatfs_t *fs,
                         const char *name,
                         flatfs_inode_t *out) {
    if (!fs || !name || !out)
        return FLATFS_ERR_INVALID;

    uint32_t inode_idx;
    flatfs_err_t err = flatfs_find(fs, name, &inode_idx);
    if (err != FLATFS_OK)
        return err;

    return flatfs_get_inode_by_index(fs, inode_idx, out);
}

flatfs_err_t flatfs_find(flatfs_t *fs,
                         const char *name,
                         uint32_t *inode_idx) {
    if (!fs || !name || !inode_idx)
        return FLATFS_ERR_INVALID;
    
    uint8_t *inode_table = kalloc(FLATFS_BLOCK_SIZE(&fs->sb) * fs->sb.inode_table_block_count);
    if (!inode_table)
        return FLATFS_ERR_NO_MEM;
    
    flatfs_err_t err = flatfs_read_blocks(fs, fs->sb.inode_table_start, fs->sb.inode_table_block_count, inode_table);
    if (err != FLATFS_OK) {
        kfree(inode_table);
        return err;
    }

    flatfs_inode_t inode;
    for (uint32_t i = 0; i < fs->sb.total_inodes; i++) {
        if (bitmap_get(fs->inode_bitmap, i) == 0)
            continue;
        
        memcpy(&inode, 
            inode_table + i * sizeof(flatfs_inode_t), 
            sizeof(flatfs_inode_t));

        if (inode.in_use && strncmp(inode.name, name, FLATFS_NAME_MAX) == 0) {
            *inode_idx = i;
            return FLATFS_OK;
        }
    }
    
    kfree(inode_table);
    return FLATFS_ERR_NOT_FOUND;
}

flatfs_err_t flatfs_get_inode_by_index(flatfs_t *fs,
                                       uint32_t inode_idx,
                                       flatfs_inode_t *inode) {
    if (!fs || !inode)
        return FLATFS_ERR_INVALID;

    if (inode_idx >= fs->sb.total_inodes)
        return FLATFS_ERR_INVALID;

    if (bitmap_get(fs->inode_bitmap, inode_idx) == 0)
        return FLATFS_ERR_NOT_FOUND;
    
    /* get the block on the inode from disk */
    uint32_t block_idx = fs->sb.inode_table_start + inode_idx * sizeof(flatfs_inode_t) / FLATFS_BLOCK_SIZE(&fs->sb);
    uint8_t *temp_block = kalloc(FLATFS_BLOCK_SIZE(&fs->sb));
    if (!temp_block)
        return FLATFS_ERR_NO_MEM;
    
    flatfs_err_t err = flatfs_read_blocks(fs, block_idx, 1, temp_block);
    if (err != FLATFS_OK) {
        kfree(temp_block);
        return err;
    }

    memcpy(inode, 
        temp_block + (inode_idx * sizeof(flatfs_inode_t)) % FLATFS_BLOCK_SIZE(&fs->sb), 
        sizeof(flatfs_inode_t));
    
    kfree(temp_block);
    return FLATFS_OK;
}