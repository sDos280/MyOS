#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

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