#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

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