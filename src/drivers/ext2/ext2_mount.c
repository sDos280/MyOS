#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

/* =========================================================================
 * FILESYSTEM LIFECYCLE
 * ========================================================================= */
/*ext2_fs_t   *ext2_mount(ata_drive_t *drive, uint32_t flags);
ext2_error_t ext2_unmount(ext2_fs_t *fs);
ext2_error_t ext2_remount(ext2_fs_t *fs, uint32_t flags);
ext2_error_t ext2_flush(ext2_fs_t *fs);
ext2_error_t ext2_last_error(void);*/
ext2_error_t ext2_get_superblock(ext2_fs_t *fs, ext2_superblock_t *sb) {
    if (fs == NULL || sb == NULL)
        return EXT2_ERR_INVALID;

    memcpy(sb, &fs->superblock, sizeof(ext2_superblock_t));
    return EXT2_OK;
}

ext2_error_t ext2_get_block_group_descriptor(ext2_fs_t *fs,
                                              uint32_t group_idx,
                                              ext2_block_group_descriptor_t *bgd) {
    if (fs == NULL || bgd == NULL)
        return EXT2_ERR_INVALID;
    
    memcpy(bgd, &fs->bgdt, sizeof(ext2_block_group_descriptor_t));
    return EXT2_OK;
}

uint32_t ext2_block_size(ext2_fs_t *fs) {
    if (fs == NULL) return 0;

    return fs->block_size;
}

uint32_t ext2_block_group_count(ext2_fs_t *fs) {
    if (fs == NULL) return 0;

    return fs->num_groups;
}
