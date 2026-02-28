#include "mm/kheap.h"
#include "drivers/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

/* NOTE:
 *  1. we will only have one copy of the superblock, the one on the start, 1024 bytes from the start of the drive */

/* =========================================================================
 * FILESYSTEM LIFECYCLE
 * ========================================================================= */
ext2_fs_t   *ext2_mount(ata_drive_t *drive, uint32_t flags);
ext2_error_t ext2_unmount(ext2_fs_t *fs);
ext2_error_t ext2_remount(ext2_fs_t *fs, uint32_t flags);
ext2_error_t ext2_flush(ext2_fs_t *fs);
ext2_error_t ext2_last_error(void);
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

/* =========================================================================
 * FILE OPERATIONS
 * ========================================================================= */
/*ext2_file_t *ext2_file_open(ext2_fs_t *fs, const char *path, uint32_t flags, uint16_t mode);
ext2_file_t *ext2_file_open_inode(ext2_fs_t *fs, uint32_t ino, uint32_t flags);
ext2_error_t ext2_file_close(ext2_file_t *file);
ext2_error_t ext2_file_read(ext2_file_t *file, void *buf, size_t count, size_t *bytes_read);
ext2_error_t ext2_file_write(ext2_file_t *file, const void *buf, size_t count, size_t *bytes_written);
ext2_error_t ext2_file_seek(ext2_file_t *file, int64_t offset, int whence, uint64_t *new_pos);
uint64_t     ext2_file_tell(ext2_file_t *file);
ext2_error_t ext2_file_truncate(ext2_file_t *file, uint64_t new_size);
ext2_error_t ext2_file_sync(ext2_file_t *file);*/

/* =========================================================================
 * FILE METADATA & LINKS
 * ========================================================================= */
/*ext2_error_t ext2_stat(ext2_fs_t *fs, const char *path, ext2_stat_t *st);
ext2_error_t ext2_lstat(ext2_fs_t *fs, const char *path, ext2_stat_t *st);
ext2_error_t ext2_fstat(ext2_file_t *file, ext2_stat_t *st);
ext2_error_t ext2_chmod(ext2_fs_t *fs, const char *path, uint16_t mode);
ext2_error_t ext2_chown(ext2_fs_t *fs, const char *path, uint16_t uid, uint16_t gid);
ext2_error_t ext2_utime(ext2_fs_t *fs, const char *path, uint32_t atime, uint32_t mtime);
ext2_error_t ext2_link(ext2_fs_t *fs, const char *oldpath, const char *newpath);
ext2_error_t ext2_unlink(ext2_fs_t *fs, const char *path);
ext2_error_t ext2_rename(ext2_fs_t *fs, const char *oldpath, const char *newpath);
ext2_error_t ext2_symlink(ext2_fs_t *fs, const char *target, const char *linkpath);
ext2_error_t ext2_readlink(ext2_fs_t *fs, const char *path, char *buf, size_t bufsz);*/

/* =========================================================================
 * DIRECTORY OPERATIONS
 * ========================================================================= */
/*ext2_error_t ext2_mkdir(ext2_fs_t *fs, const char *path, uint16_t mode);
ext2_error_t ext2_mkdir_p(ext2_fs_t *fs, const char *path, uint16_t mode);
ext2_error_t ext2_rmdir(ext2_fs_t *fs, const char *path);
ext2_error_t ext2_rmdir_r(ext2_fs_t *fs, const char *path);
ext2_dir_t  *ext2_dir_open(ext2_fs_t *fs, const char *path);
ext2_dir_t  *ext2_dir_open_inode(ext2_fs_t *fs, uint32_t ino);
ext2_error_t ext2_dir_read(ext2_dir_t *dir, ext2_dirent_t *entry);
ext2_error_t ext2_dir_rewind(ext2_dir_t *dir);
ext2_error_t ext2_dir_close(ext2_dir_t *dir);*/

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

ext2_error_t ext2_inode_alloc(ext2_fs_t *fs, uint32_t preferred_group, uint32_t *ino_out)
{
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

/*ext2_error_t ext2_inode_free(ext2_fs_t *fs, uint32_t ino);
ext2_error_t ext2_inode_free_blocks(ext2_fs_t *fs, ext2_inode_t *inode);
ext2_error_t ext2_inode_resolve_block(ext2_fs_t *fs, const ext2_inode_t *inode, uint32_t logical_block, uint32_t *phys_block);
uint64_t     ext2_inode_get_size(ext2_fs_t *fs, const ext2_inode_t *inode);*/

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

ext2_error_t ext2_block_free(ext2_fs_t *fs, uint32_t block_no)
{
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

/* =========================================================================
 * UTILITIES
 * ========================================================================= */
/*ext2_error_t ext2_statvfs(ext2_fs_t *fs, uint32_t *total_blocks, uint32_t *free_blocks, uint32_t *total_inodes, uint32_t *free_inodes);
ext2_error_t ext2_lookup(ext2_fs_t *fs, const char *path, uint32_t *ino_out);
int          ext2_path_exists(ext2_fs_t *fs, const char *path);
int          ext2_is_dir(ext2_fs_t *fs, const char *path);
int          ext2_is_file(ext2_fs_t *fs, const char *path);
int          ext2_is_symlink(ext2_fs_t *fs, const char *path);
const char  *ext2_strerror(ext2_error_t err);*/