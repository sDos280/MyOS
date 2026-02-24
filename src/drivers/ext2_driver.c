#include "drivers/ext2_driver.h"
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
ext2_file_t *ext2_file_open(ext2_fs_t *fs, const char *path, uint32_t flags, uint16_t mode);
ext2_file_t *ext2_file_open_inode(ext2_fs_t *fs, uint32_t ino, uint32_t flags);
ext2_error_t ext2_file_close(ext2_file_t *file);
ext2_error_t ext2_file_read(ext2_file_t *file, void *buf, size_t count, size_t *bytes_read);
ext2_error_t ext2_file_write(ext2_file_t *file, const void *buf, size_t count, size_t *bytes_written);
ext2_error_t ext2_file_seek(ext2_file_t *file, int64_t offset, int whence, uint64_t *new_pos);
uint64_t     ext2_file_tell(ext2_file_t *file);
ext2_error_t ext2_file_truncate(ext2_file_t *file, uint64_t new_size);
ext2_error_t ext2_file_sync(ext2_file_t *file);

/* =========================================================================
 * FILE METADATA & LINKS
 * ========================================================================= */
ext2_error_t ext2_stat(ext2_fs_t *fs, const char *path, ext2_stat_t *st);
ext2_error_t ext2_lstat(ext2_fs_t *fs, const char *path, ext2_stat_t *st);
ext2_error_t ext2_fstat(ext2_file_t *file, ext2_stat_t *st);
ext2_error_t ext2_chmod(ext2_fs_t *fs, const char *path, uint16_t mode);
ext2_error_t ext2_chown(ext2_fs_t *fs, const char *path, uint16_t uid, uint16_t gid);
ext2_error_t ext2_utime(ext2_fs_t *fs, const char *path, uint32_t atime, uint32_t mtime);
ext2_error_t ext2_link(ext2_fs_t *fs, const char *oldpath, const char *newpath);
ext2_error_t ext2_unlink(ext2_fs_t *fs, const char *path);
ext2_error_t ext2_rename(ext2_fs_t *fs, const char *oldpath, const char *newpath);
ext2_error_t ext2_symlink(ext2_fs_t *fs, const char *target, const char *linkpath);
ext2_error_t ext2_readlink(ext2_fs_t *fs, const char *path, char *buf, size_t bufsz);

/* =========================================================================
 * DIRECTORY OPERATIONS
 * ========================================================================= */
ext2_error_t ext2_mkdir(ext2_fs_t *fs, const char *path, uint16_t mode);
ext2_error_t ext2_mkdir_p(ext2_fs_t *fs, const char *path, uint16_t mode);
ext2_error_t ext2_rmdir(ext2_fs_t *fs, const char *path);
ext2_error_t ext2_rmdir_r(ext2_fs_t *fs, const char *path);
ext2_dir_t  *ext2_dir_open(ext2_fs_t *fs, const char *path);
ext2_dir_t  *ext2_dir_open_inode(ext2_fs_t *fs, uint32_t ino);
ext2_error_t ext2_dir_read(ext2_dir_t *dir, ext2_dirent_t *entry);
ext2_error_t ext2_dir_rewind(ext2_dir_t *dir);
ext2_error_t ext2_dir_close(ext2_dir_t *dir);

/* =========================================================================
 * INODE ACCESS
 * ========================================================================= */
ext2_error_t ext2_inode_read(ext2_fs_t *fs, uint32_t ino, ext2_inode_t *inode);
ext2_error_t ext2_inode_write(ext2_fs_t *fs, uint32_t ino, const ext2_inode_t *inode);
ext2_error_t ext2_inode_alloc(ext2_fs_t *fs, uint32_t preferred_group, uint32_t *ino_out);
ext2_error_t ext2_inode_free(ext2_fs_t *fs, uint32_t ino);
ext2_error_t ext2_inode_free_blocks(ext2_fs_t *fs, ext2_inode_t *inode);
ext2_error_t ext2_inode_resolve_block(ext2_fs_t *fs, const ext2_inode_t *inode, uint32_t logical_block, uint32_t *phys_block);
uint64_t     ext2_inode_get_size(ext2_fs_t *fs, const ext2_inode_t *inode);

/* =========================================================================
 * BLOCK ACCESS
 * ========================================================================= */
ext2_error_t ext2_block_read(ext2_fs_t *fs, uint32_t block_no, void *buf) {
    if (fs == NULL || buf == NULL)
        return EXT2_ERR_INVALID;
    
    uint32_t starting_sector = 4 + (fs->sectors_per_block * block_no);

    if (ata_read28_request(fs->drive, starting_sector, fs->sectors_per_block, buf) != 0) {
        /* FIX: should probaly do something */
        return EXT2_ERR_IO;
    }
}

ext2_error_t ext2_block_write(ext2_fs_t *fs, uint32_t block_no, const void *buf) {
    if (fs == NULL || buf == NULL)
        return EXT2_ERR_INVALID;
    
    uint32_t starting_sector = 4 + (fs->sectors_per_block * block_no);

    if (ata_write28_request(fs->drive, starting_sector, fs->sectors_per_block, buf) != 0) {
        /* FIX: should probaly do something */
        return EXT2_ERR_IO;
    }
}
ext2_error_t ext2_block_alloc(ext2_fs_t *fs, uint32_t preferred_group, uint32_t *block_out) {
    
}
ext2_error_t ext2_block_free(ext2_fs_t *fs, uint32_t block_no);

/* =========================================================================
 * UTILITIES
 * ========================================================================= */
ext2_error_t ext2_statvfs(ext2_fs_t *fs, uint32_t *total_blocks, uint32_t *free_blocks, uint32_t *total_inodes, uint32_t *free_inodes);
ext2_error_t ext2_lookup(ext2_fs_t *fs, const char *path, uint32_t *ino_out);
int          ext2_path_exists(ext2_fs_t *fs, const char *path);
int          ext2_is_dir(ext2_fs_t *fs, const char *path);
int          ext2_is_file(ext2_fs_t *fs, const char *path);
int          ext2_is_symlink(ext2_fs_t *fs, const char *path);
const char  *ext2_strerror(ext2_error_t err);