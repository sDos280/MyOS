#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

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
