#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

static ext2_error_t free_direct_blocks(ext2_fs_t *fs, ext2_inode_t *inode);
static ext2_error_t free_single_indirect(ext2_fs_t *fs, uint32_t block_no, uint8_t *buf);
static ext2_error_t free_double_indirect(ext2_fs_t *fs, uint32_t block_no, uint8_t *buf1, uint8_t *buf2);
static ext2_error_t free_triple_indirect(ext2_fs_t *fs, uint32_t block_no, uint8_t *buf1, uint8_t *buf2, uint8_t *buf3);

/* =========================================================================
 * INODE ACCESS
 * ========================================================================= */

ext2_error_t ext2_inode_read(ext2_fs_t *fs, uint32_t ino, ext2_inode_t *inode) {
    if (!fs || !inode)
        return EXT2_ERR_INVALID;
    
    /* find the inode table */
    /* find the group index of the inode table */
    uint32_t group_idx    = (ino - 1) / fs->superblock.s_inodes_per_group;

    if (group_idx >= fs->num_groups)  // add this
        return EXT2_ERR_INVALID;
    /* find the inner local index of the inode entry in the inode table */
    uint32_t local_idx    = (ino - 1) % fs->superblock.s_inodes_per_group;
    /* caluclate the block offset from the start of the table that the inode entry resides */
    uint32_t block_offset = (local_idx * fs->superblock.s_inode_size) / fs->block_size;
    /* caluclate the block index that the inode entry resides in */
    uint32_t inode_table_block_no     = fs->bgdt[group_idx].bg_inode_table + block_offset;

    uint8_t *block_buf = (uint8_t *)kalloc(fs->block_size);
    if (!block_buf)
        return EXT2_ERR_NO_MEM;
    
    ext2_error_t err = ext2_block_read(fs, inode_table_block_no, block_buf);
    if (err != EXT2_OK) {
        kfree(block_buf);
        return err;
    }

    uint32_t inode_entry_offset = (local_idx % fs->inodes_per_block) * fs->superblock.s_inode_size;
    memcpy(inode, block_buf + inode_entry_offset, sizeof(ext2_inode_t));

    kfree(block_buf);
    return EXT2_OK;
}

ext2_error_t ext2_inode_write(ext2_fs_t *fs, uint32_t ino, const ext2_inode_t *inode) {
    if (!fs || !inode)
        return EXT2_ERR_INVALID;
    
    if (fs->read_only)
        return EXT2_ERR_READ_ONLY;
    
    /* find the inode table */
    /* find the group index of the inode table */
    uint32_t group_idx    = (ino - 1) / fs->superblock.s_inodes_per_group;

    if (group_idx >= fs->num_groups)  // add this
        return EXT2_ERR_INVALID;
    /* find the inner local index of the inode entry in the inode table */
    uint32_t local_idx    = (ino - 1) % fs->superblock.s_inodes_per_group;
    /* caluclate the block offset from the start of the table that the inode entry resides */
    uint32_t block_offset = (local_idx * fs->superblock.s_inode_size) / fs->block_size;
    /* caluclate the block index that the inode entry resides in */
    uint32_t inode_table_block_no     = fs->bgdt[group_idx].bg_inode_table + block_offset;

    uint8_t *block_buf = (uint8_t *)kalloc(fs->block_size);
    if (!block_buf)
        return EXT2_ERR_NO_MEM;
    
    ext2_error_t err = ext2_block_read(fs, inode_table_block_no, block_buf);
    if (err != EXT2_OK) {
        kfree(block_buf);
        return err;
    }

    uint32_t inode_entry_offset = (local_idx % fs->inodes_per_block) * fs->superblock.s_inode_size;
    memcpy(block_buf + inode_entry_offset, inode, sizeof(ext2_inode_t));

    kfree(block_buf);
    return EXT2_OK;
}

ext2_error_t ext2_inode_alloc(ext2_fs_t *fs, uint32_t preferred_group, uint32_t *ino_out) {
    if (!fs || !ino_out)
        return EXT2_ERR_INVALID;

    if (fs->read_only)
        return EXT2_ERR_READ_ONLY;

    if (fs->superblock.s_free_inodes_count == 0)
        return EXT2_ERR_NO_INODES;

    uint8_t *bitmap = (uint8_t *)kmalloc(fs->block_size);
    if (!bitmap)
        return EXT2_ERR_NO_MEM;

    for (uint32_t i = 0; i < fs->num_groups; i++) {
        uint32_t group_idx = (preferred_group + i) % fs->num_groups;
        ext2_block_group_descriptor_t *bgd = &fs->bgdt[group_idx];

        if (bgd->bg_free_inodes_count == 0)
            continue;

        ext2_error_t err = ext2_block_read(fs, bgd->bg_inode_bitmap, bitmap);
        if (err != EXT2_OK) {
            kfree(bitmap);
            return err;
        }

        /* Last group may have fewer inodes than s_inodes_per_group */
        uint32_t inodes_in_group = (group_idx == fs->num_groups - 1)
            ? (fs->superblock.s_inodes_count
               - (group_idx * fs->superblock.s_inodes_per_group))
            : fs->superblock.s_inodes_per_group;

        uint32_t local_idx;
        if (!bitmap_find_first_clear(bitmap, inodes_in_group, &local_idx))
            continue;

        bitmap_set(bitmap, local_idx);

        err = ext2_block_write(fs, bgd->bg_inode_bitmap, bitmap);
        if (err != EXT2_OK) {
            kfree(bitmap);
            return err;
        }

        bgd->bg_free_inodes_count--;
        fs->bgdt_dirty = 1;

        fs->superblock.s_free_inodes_count--;
        fs->sb_dirty = 1;

        /* inode numbers are 1-based */
        *ino_out = (group_idx * fs->superblock.s_inodes_per_group) + local_idx + 1;

        kfree(bitmap);
        return EXT2_OK;
    }

    kfree(bitmap);
    return EXT2_ERR_NO_INODES;
}

ext2_error_t ext2_inode_free(ext2_fs_t *fs, uint32_t ino) {
    if (!fs)
        return EXT2_ERR_INVALID;

    if (fs->read_only)
        return EXT2_ERR_READ_ONLY;
    
    /* find the group index of the inode table */
    uint32_t group_idx    = (ino - 1) / fs->superblock.s_inodes_per_group;

    if (group_idx >= fs->num_groups)  // add this
        return EXT2_ERR_INVALID;
    /* find the inner local index of the inode entry in the inode table */
    uint32_t local_idx    = (ino - 1) % fs->superblock.s_inodes_per_group;
    ext2_block_group_descriptor_t *bgd = &fs->bgdt[group_idx];

    uint8_t *bitmap = (uint8_t *)kalloc(fs->block_size);
    if (!bitmap)
        return EXT2_ERR_NO_MEM;
    
    ext2_error_t err = ext2_block_read(fs, bgd->bg_inode_bitmap, bitmap);
    if (err != EXT2_OK) {
        kfree(bitmap);
        return err;
    }

    bitmap_clear(bitmap, local_idx);

    /* save the change */
    err = ext2_block_write(fs, bgd->bg_inode_bitmap, bitmap);
    if (err != EXT2_OK) {
        kfree(bitmap);
        return err;
    }

    bgd->bg_free_inodes_count++;
    fs->bgdt_dirty = 1;

    fs->superblock.s_free_inodes_count++;
    fs->sb_dirty = 1;

    kfree(bitmap);
    return EXT2_OK;
}

ext2_error_t ext2_inode_free_blocks(ext2_fs_t *fs, ext2_inode_t *inode) {
    if (!fs || !inode)
        return EXT2_ERR_INVALID;

    if (fs->read_only)
        return EXT2_ERR_READ_ONLY;

    uint8_t *buf1 = (uint8_t *)kmalloc(fs->block_size);
    uint8_t *buf2 = (uint8_t *)kmalloc(fs->block_size);
    uint8_t *buf3 = (uint8_t *)kmalloc(fs->block_size);

    if (!buf1 || !buf2 || !buf3) {
        kfree(buf1); kfree(buf2); kfree(buf3);
        return EXT2_ERR_NO_MEM;
    }

    ext2_error_t err;

    err = free_direct_blocks(fs, inode);
    if (err != EXT2_OK) goto fail;

    if (inode->i_block[12] != 0) {
        err = free_single_indirect(fs, inode->i_block[12], buf1);
        if (err != EXT2_OK) goto fail;
    }

    if (inode->i_block[13] != 0) {
        err = free_double_indirect(fs, inode->i_block[13], buf1, buf2);
        if (err != EXT2_OK) goto fail;
    }

    if (inode->i_block[14] != 0) {
        err = free_triple_indirect(fs, inode->i_block[14], buf1, buf2, buf3);
        if (err != EXT2_OK) goto fail;
    }

    kfree(buf1); kfree(buf2); kfree(buf3);
    return EXT2_OK;

fail:
    kfree(buf1); kfree(buf2); kfree(buf3);
    return err;
}

/*ext2_error_t ext2_inode_resolve_block(ext2_fs_t *fs, const ext2_inode_t *inode, uint32_t logical_block, uint32_t *phys_block);
uint64_t     ext2_inode_get_size(ext2_fs_t *fs, const ext2_inode_t *inode);*/

/* =========================================================================
 * INTERNAL HELPERS
 * ========================================================================= */

static ext2_error_t free_direct_blocks(ext2_fs_t *fs, ext2_inode_t *inode) {
    for (uint32_t i = 0; i < 12; i++) {
        if (inode->i_block[i] == 0)
            continue;

        ext2_error_t err = ext2_block_free(fs, inode->i_block[i]);
        if (err != EXT2_OK)
            return err;
    }
    return EXT2_OK;
}

static ext2_error_t free_single_indirect(ext2_fs_t *fs, uint32_t block_no,
                                          uint8_t *buf) {
    ext2_error_t err = ext2_block_read(fs, block_no, buf);
    if (err != EXT2_OK)
        return err;

    uint32_t *ptrs = (uint32_t *)buf;
    for (uint32_t i = 0; i < fs->ptrs_per_block; i++) {
        if (ptrs[i] == 0)
            continue;

        err = ext2_block_free(fs, ptrs[i]);
        if (err != EXT2_OK)
            return err;
    }

    /* Free the indirect block itself */
    return ext2_block_free(fs, block_no);
}

static ext2_error_t free_double_indirect(ext2_fs_t *fs, uint32_t block_no,
                                          uint8_t *buf1, uint8_t *buf2) {
    ext2_error_t err = ext2_block_read(fs, block_no, buf1);
    if (err != EXT2_OK)
        return err;

    uint32_t *l1 = (uint32_t *)buf1;
    for (uint32_t i = 0; i < fs->ptrs_per_block; i++) {
        if (l1[i] == 0)
            continue;

        err = free_single_indirect(fs, l1[i], buf2);
        if (err != EXT2_OK)
            return err;
    }

    /* Free the doubly indirect block itself */
    return ext2_block_free(fs, block_no);
}

static ext2_error_t free_triple_indirect(ext2_fs_t *fs, uint32_t block_no,
                                          uint8_t *buf1, uint8_t *buf2,
                                          uint8_t *buf3) {
    ext2_error_t err = ext2_block_read(fs, block_no, buf1);
    if (err != EXT2_OK)
        return err;

    uint32_t *l1 = (uint32_t *)buf1;
    for (uint32_t i = 0; i < fs->ptrs_per_block; i++) {
        if (l1[i] == 0)
            continue;

        err = free_double_indirect(fs, l1[i], buf2, buf3);
        if (err != EXT2_OK)
            return err;
    }

    /* Free the triply indirect block itself */
    return ext2_block_free(fs, block_no);
}
