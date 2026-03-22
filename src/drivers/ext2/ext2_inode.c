#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

static ext2_error_t free_direct_blocks(ext2_fs_t *fs, ext2_inode_t *inode);
static ext2_error_t free_single_indirect(ext2_fs_t *fs, uint32_t block_no, uint8_t *buf);
static ext2_error_t free_double_indirect(ext2_fs_t *fs, uint32_t block_no, uint8_t *buf1, uint8_t *buf2);
static ext2_error_t free_triple_indirect(ext2_fs_t *fs, uint32_t block_no, uint8_t *buf1, uint8_t *buf2, uint8_t *buf3);
static ext2_error_t resolve_single_indirect(ext2_fs_t *fs, uint32_t indirect_block, uint32_t idx, uint32_t *phys_block);
static ext2_error_t resolve_double_indirect(ext2_fs_t *fs, uint32_t indirect_block, uint32_t idx, uint32_t *phys_block);
static ext2_error_t resolve_triple_indirect(ext2_fs_t *fs, uint32_t indirect_block, uint32_t idx, uint32_t *phys_block);
static ext2_error_t ensure_indirect_block(ext2_fs_t *fs, uint32_t *block_ptr);
static ext2_error_t assign_single_indirect(ext2_fs_t *fs, uint32_t indirect_block, uint32_t idx, uint32_t phys_block);
static ext2_error_t assign_double_indirect(ext2_fs_t *fs, uint32_t indirect_block, uint32_t idx, uint32_t phys_block);
static ext2_error_t assign_triple_indirect(ext2_fs_t *fs, uint32_t indirect_block, uint32_t idx, uint32_t phys_block);

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

    uint8_t *bitmap = (uint8_t *)kalloc(fs->block_size);
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

    uint8_t *buf1 = (uint8_t *)kalloc(fs->block_size);
    uint8_t *buf2 = (uint8_t *)kalloc(fs->block_size);
    uint8_t *buf3 = (uint8_t *)kalloc(fs->block_size);

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

ext2_error_t ext2_inode_resolve_block(ext2_fs_t *fs, const ext2_inode_t *inode,
                                       uint32_t logical_block, uint32_t *phys_block) {
    if (!fs || !inode || !phys_block)
        return EXT2_ERR_INVALID;

    /* ------------------------------------------------------------------ */
    /* Region boundaries                                                   */
    /* ------------------------------------------------------------------ */
    uint32_t direct_limit  = 12;
    uint32_t single_limit  = direct_limit + fs->ptrs_per_block;
    uint32_t double_limit  = single_limit + (fs->ptrs_per_block * fs->ptrs_per_block);
    uint32_t triple_limit  = double_limit + (fs->ptrs_per_block * fs->ptrs_per_block
                                             * fs->ptrs_per_block);

    /* ------------------------------------------------------------------ */
    /* Direct blocks [0, 11]                                               */
    /* ------------------------------------------------------------------ */
    if (logical_block < direct_limit) {
        *phys_block = inode->i_block[logical_block];
        return EXT2_OK;
    }

    /* ------------------------------------------------------------------ */
    /* Singly indirect [12, 12 + ptrs_per_block)                          */
    /* ------------------------------------------------------------------ */
    if (logical_block < single_limit) {
        if (inode->i_block[12] == 0) {
            *phys_block = 0;    /* sparse hole */
            return EXT2_OK;
        }

        uint32_t idx = logical_block - direct_limit;
        return resolve_single_indirect(fs, inode->i_block[12], idx, phys_block);
    }

    /* ------------------------------------------------------------------ */
    /* Doubly indirect [single_limit, double_limit)                        */
    /* ------------------------------------------------------------------ */
    if (logical_block < double_limit) {
        if (inode->i_block[13] == 0) {
            *phys_block = 0;
            return EXT2_OK;
        }

        uint32_t idx = logical_block - single_limit;
        return resolve_double_indirect(fs, inode->i_block[13], idx, phys_block);
    }

    /* ------------------------------------------------------------------ */
    /* Triply indirect [double_limit, triple_limit)                        */
    /* ------------------------------------------------------------------ */
    if (logical_block < triple_limit) {
        if (inode->i_block[14] == 0) {
            *phys_block = 0;
            return EXT2_OK;
        }

        uint32_t idx = logical_block - double_limit;
        return resolve_triple_indirect(fs, inode->i_block[14], idx, phys_block);
    }

    /* Beyond the maximum file size ext2 can represent */
    return EXT2_ERR_OVERFLOW;
}

uint64_t ext2_inode_get_size(ext2_fs_t *fs, const ext2_inode_t *inode) {
    if (!fs || !inode)
        return 0;

    if (fs->superblock.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_LARGE_FILE)
        return (uint64_t)inode->i_size
             | ((uint64_t)inode->i_dir_acl << 32);

    return (uint64_t)inode->i_size;
}

ext2_error_t ext2_inode_assign_block(ext2_fs_t *fs, ext2_inode_t *inode,
                                      uint32_t logical_block, uint32_t phys_block) {
    if (!fs || !inode)
        return EXT2_ERR_INVALID;

    ext2_error_t err;

    uint32_t direct_limit = 12;
    uint32_t single_limit = direct_limit + fs->ptrs_per_block;
    uint32_t double_limit = single_limit + (fs->ptrs_per_block * fs->ptrs_per_block);

    /* ------------------------------------------------------------------ */
    /* Direct                                                              */
    /* ------------------------------------------------------------------ */
    if (logical_block < direct_limit) {
        inode->i_block[logical_block] = phys_block;
        return EXT2_OK;
    }

    /* ------------------------------------------------------------------ */
    /* Singly indirect                                                     */
    /* ------------------------------------------------------------------ */
    if (logical_block < single_limit) {
        err = ensure_indirect_block(fs, &inode->i_block[12]);
        if (err != EXT2_OK)
            return err;

        return assign_single_indirect(fs, inode->i_block[12],
                                      logical_block - direct_limit,
                                      phys_block);
    }

    /* ------------------------------------------------------------------ */
    /* Doubly indirect                                                     */
    /* ------------------------------------------------------------------ */
    if (logical_block < double_limit) {
        err = ensure_indirect_block(fs, &inode->i_block[13]);
        if (err != EXT2_OK)
            return err;

        return assign_double_indirect(fs, inode->i_block[13],
                                      logical_block - single_limit,
                                      phys_block);
    }

    /* ------------------------------------------------------------------ */
    /* Triply indirect                                                     */
    /* ------------------------------------------------------------------ */
    err = ensure_indirect_block(fs, &inode->i_block[14]);
    if (err != EXT2_OK)
        return err;

    return assign_triple_indirect(fs, inode->i_block[14],
                                  logical_block - double_limit,
                                  phys_block);
}

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

/*
 * Free all the blocks in the nested blocks in the single indirect block, and the block itself.
 */
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

/*
 * Free all the blocks in the nested blocks in the double indirect block, and the block itself.
 */
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

/*
 * Free all the blocks in the nested blocks in the triple indirect block, and the block itself.
 */
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

/*
 * Read one block pointer at position idx from a singly indirect block.
 */
static ext2_error_t resolve_single_indirect(ext2_fs_t *fs, uint32_t indirect_block,
                                             uint32_t idx, uint32_t *phys_block)
{
    uint8_t *buf = (uint8_t *)kalloc(fs->block_size);
    if (!buf)
        return EXT2_ERR_NO_MEM;

    ext2_error_t err = ext2_block_read(fs, indirect_block, buf);
    if (err != EXT2_OK) {
        kfree(buf);
        return err;
    }

    *phys_block = ((uint32_t *)buf)[idx];

    kfree(buf);
    return EXT2_OK;
}

/*
 * Resolve a logical index within a doubly indirect block.
 * idx is relative to the start of the doubly indirect region.
 */
static ext2_error_t resolve_double_indirect(ext2_fs_t *fs, uint32_t indirect_block,
                                             uint32_t idx, uint32_t *phys_block)
{
    uint8_t *buf = (uint8_t *)kalloc(fs->block_size);
    if (!buf)
        return EXT2_ERR_NO_MEM;

    ext2_error_t err = ext2_block_read(fs, indirect_block, buf);
    if (err != EXT2_OK) {
        kfree(buf);
        return err;
    }

    uint32_t l1_idx       = idx / fs->ptrs_per_block;
    uint32_t l2_idx       = idx % fs->ptrs_per_block;
    uint32_t l1_block     = ((uint32_t *)buf)[l1_idx];

    kfree(buf);

    if (l1_block == 0) {
        *phys_block = 0;    /* sparse hole */
        return EXT2_OK;
    }

    return resolve_single_indirect(fs, l1_block, l2_idx, phys_block);
}

/*
 * Resolve a logical index within a triply indirect block.
 * idx is relative to the start of the triply indirect region.
 */
static ext2_error_t resolve_triple_indirect(ext2_fs_t *fs, uint32_t indirect_block,
                                             uint32_t idx, uint32_t *phys_block)
{
    uint8_t *buf = (uint8_t *)kalloc(fs->block_size);
    if (!buf)
        return EXT2_ERR_NO_MEM;

    ext2_error_t err = ext2_block_read(fs, indirect_block, buf);
    if (err != EXT2_OK) {
        kfree(buf);
        return err;
    }

    uint32_t ptrs_per_dind = fs->ptrs_per_block * fs->ptrs_per_block;
    uint32_t l1_idx        = idx / ptrs_per_dind;
    uint32_t l2_idx        = idx % ptrs_per_dind;
    uint32_t l1_block      = ((uint32_t *)buf)[l1_idx];

    kfree(buf);

    if (l1_block == 0) {
        *phys_block = 0;    /* sparse hole */
        return EXT2_OK;
    }

    return resolve_double_indirect(fs, l1_block, l2_idx, phys_block);
}


/*
 * Allocates a new zeroed indirect block if *block_ptr is 0.
 * Writes it to disk and updates *block_ptr in place.
 */
static ext2_error_t ensure_indirect_block(ext2_fs_t *fs, uint32_t *block_ptr)
{
    if (*block_ptr != 0)
        return EXT2_OK;

    uint32_t new_block;
    ext2_error_t err = ext2_block_alloc(fs, 0, &new_block);
    if (err != EXT2_OK)
        return err;

    uint8_t *buf = (uint8_t *)kmalloc(fs->block_size);
    if (!buf) {
        ext2_block_free(fs, new_block);
        return EXT2_ERR_NO_MEM;
    }

    memset(buf, 0, fs->block_size);
    err = ext2_block_write(fs, new_block, buf);
    kfree(buf);
    if (err != EXT2_OK) {
        ext2_block_free(fs, new_block);
        return err;
    }

    *block_ptr = new_block;
    return EXT2_OK;
}

/*
 * Write phys_block into slot idx of a singly indirect block.
 */
static ext2_error_t assign_single_indirect(ext2_fs_t *fs, uint32_t indirect_block,
                                            uint32_t idx, uint32_t phys_block)
{
    uint32_t *buf = (uint32_t *)kmalloc(fs->block_size);
    if (!buf)
        return EXT2_ERR_NO_MEM;

    ext2_error_t err = ext2_block_read(fs, indirect_block, buf);
    if (err != EXT2_OK) {
        kfree(buf);
        return err;
    }

    buf[idx] = phys_block;

    err = ext2_block_write(fs, indirect_block, buf);
    kfree(buf);
    return err;
}

/*
 * Write phys_block into the correct slot within a doubly indirect block.
 * idx is relative to the start of the doubly indirect region.
 */
static ext2_error_t assign_double_indirect(ext2_fs_t *fs, uint32_t indirect_block,
                                            uint32_t idx, uint32_t phys_block)
{
    uint32_t l1_idx = idx / fs->ptrs_per_block;
    uint32_t l2_idx = idx % fs->ptrs_per_block;

    uint32_t *buf = (uint32_t *)kmalloc(fs->block_size);
    if (!buf)
        return EXT2_ERR_NO_MEM;

    ext2_error_t err = ext2_block_read(fs, indirect_block, buf);
    if (err != EXT2_OK) {
        kfree(buf);
        return err;
    }

    err = ensure_indirect_block(fs, &buf[l1_idx]);
    if (err != EXT2_OK) {
        kfree(buf);
        return err;
    }

    /* write back l1 in case ensure_indirect_block allocated a new block */
    err = ext2_block_write(fs, indirect_block, buf);
    if (err != EXT2_OK) {
        kfree(buf);
        return err;
    }

    uint32_t l1_block = buf[l1_idx];
    kfree(buf);

    return assign_single_indirect(fs, l1_block, l2_idx, phys_block);
}

/*
 * Write phys_block into the correct slot within a triply indirect block.
 * idx is relative to the start of the triply indirect region.
 */
static ext2_error_t assign_triple_indirect(ext2_fs_t *fs, uint32_t indirect_block,
                                            uint32_t idx, uint32_t phys_block)
{
    uint32_t ptrs_per_dind = fs->ptrs_per_block * fs->ptrs_per_block;
    uint32_t l1_idx        = idx / ptrs_per_dind;
    uint32_t l2_idx        = idx % ptrs_per_dind;

    uint32_t *buf = (uint32_t *)kmalloc(fs->block_size);
    if (!buf)
        return EXT2_ERR_NO_MEM;

    ext2_error_t err = ext2_block_read(fs, indirect_block, buf);
    if (err != EXT2_OK) {
        kfree(buf);
        return err;
    }

    err = ensure_indirect_block(fs, &buf[l1_idx]);
    if (err != EXT2_OK) {
        kfree(buf);
        return err;
    }

    err = ext2_block_write(fs, indirect_block, buf);
    if (err != EXT2_OK) {
        kfree(buf);
        return err;
    }

    uint32_t l1_block = buf[l1_idx];
    kfree(buf);

    return assign_double_indirect(fs, l1_block, l2_idx, phys_block);
}