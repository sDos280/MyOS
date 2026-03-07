#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

/* =========================================================================
 * BLOCK ACCESS
 * ========================================================================= */

ext2_error_t ext2_block_read(ext2_fs_t *fs, uint32_t block_no, void *buf) {
    if (!fs || !buf)
        return EXT2_ERR_INVALID;
    
    uint32_t starting_sector = 4 + (fs->sectors_per_block * block_no);

    if (ata_read28_request(fs->drive, starting_sector, fs->sectors_per_block, buf) != 0) {
        /* FIX: should probaly do something */
        return EXT2_ERR_IO;
    }
}

ext2_error_t ext2_block_write(ext2_fs_t *fs, uint32_t block_no, const void *buf) {
    if (!fs || !buf)
        return EXT2_ERR_INVALID;
    
    uint32_t starting_sector = 4 + (fs->sectors_per_block * block_no);

    if (ata_write28_request(fs->drive, starting_sector, fs->sectors_per_block, buf) != 0) {
        /* FIX: should probaly do something */
        return EXT2_ERR_IO;
    }
}

ext2_error_t ext2_block_alloc(ext2_fs_t *fs, uint32_t preferred_group, uint32_t *block_out) {
    if (!fs || !block_out)
        return EXT2_ERR_INVALID;

    if (fs->read_only)
        return EXT2_ERR_READ_ONLY;

    if (fs->superblock.s_free_blocks_count == 0)
        return EXT2_ERR_NO_SPACE;

    uint8_t *bitmap = (uint8_t *)kalloc(fs->block_size);
    if (!bitmap)
        return EXT2_ERR_NO_MEM;

    for (uint32_t i = 0; i < fs->num_groups; i++) { /* note: when i=0, group id = prefered group id */
        uint32_t group_idx = (preferred_group + i) % fs->num_groups;
        ext2_block_group_descriptor_t *bgd = &fs->bgdt[group_idx];

        if (bgd->bg_free_blocks_count == 0)
            continue;

        /* read the bitmap('s block) from memory */
        ext2_error_t err = ext2_block_read(fs, bgd->bg_block_bitmap, bitmap);
        if (err != EXT2_OK) {
            kfree(bitmap);
            return err;
        }

        /* Last group may have fewer blocks than s_blocks_per_group */
        uint32_t blocks_in_group = (group_idx == fs->num_groups - 1)
            ? (fs->superblock.s_blocks_count
               - fs->superblock.s_first_data_block
               - (group_idx * fs->superblock.s_blocks_per_group))
            : fs->superblock.s_blocks_per_group;

        uint32_t local_idx;
        if (!bitmap_find_first_clear(bitmap, blocks_in_group, &local_idx))
            continue;

        bitmap_set(bitmap, local_idx);

        err = ext2_block_write(fs, bgd->bg_block_bitmap, bitmap);
        if (err != EXT2_OK) {
            kfree(bitmap);
            return err;
        }

        bgd->bg_free_blocks_count--;
        fs->bgdt_dirty = 1;

        fs->superblock.s_free_blocks_count--;
        fs->sb_dirty = 1;

        *block_out = fs->superblock.s_first_data_block
                   + (group_idx * fs->superblock.s_blocks_per_group)
                   + local_idx;

        kfree(bitmap);
        return EXT2_OK;
    }

    kfree(bitmap);
    return EXT2_ERR_NO_SPACE;
}

ext2_error_t ext2_block_free(ext2_fs_t *fs, uint32_t block_no) {
    if (!fs)
        return EXT2_ERR_INVALID;

    if (fs->read_only)
        return EXT2_ERR_READ_ONLY;

    uint32_t group_idx = EXT2_BLOCK_GROUP(&fs->superblock, block_no);
    if (group_idx >= fs->num_groups)
        return EXT2_ERR_INVALID;

    uint8_t *bitmap = (uint8_t *)kmalloc(fs->block_size);
    if (!bitmap)
        return EXT2_ERR_NO_MEM;

    ext2_block_group_descriptor_t *bgd = &fs->bgdt[group_idx];

    ext2_error_t err = ext2_block_read(fs, bgd->bg_block_bitmap, bitmap);
    if (err != EXT2_OK) {
        kfree(bitmap);
        return err;
    }

    uint32_t local_idx = (block_no - fs->superblock.s_first_data_block)
                         % fs->superblock.s_blocks_per_group;

    bitmap_clear(bitmap, local_idx);

    err = ext2_block_write(fs, bgd->bg_block_bitmap, bitmap);
    if (err != EXT2_OK) {
        kfree(bitmap);
        return err;
    }

    bgd->bg_free_blocks_count++;
    fs->bgdt_dirty = 1;

    fs->superblock.s_free_blocks_count++;
    fs->sb_dirty = 1;

    kfree(bitmap);
    return EXT2_OK;
}
