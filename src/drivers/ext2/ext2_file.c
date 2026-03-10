#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

/* =========================================================================
 * FILE OPERATIONS
 * ========================================================================= */

/*ext2_error_t ext2_file_open(ext2_fs_t *fs, const char *path, uint32_t flags, uint16_t mode, ext2_file_t **file_out);
ext2_error_t ext2_file_open_inode(ext2_fs_t *fs, uint32_t ino, uint32_t flags, ext2_file_t **file_out);
ext2_error_t ext2_file_close(ext2_file_t *file);
ext2_error_t ext2_file_read(ext2_file_t *file, void *buf, size_t count, size_t *bytes_read);
ext2_error_t ext2_file_write(ext2_file_t *file, const void *buf, size_t count, size_t *bytes_written);
ext2_error_t ext2_file_seek(ext2_file_t *file, int64_t offset, int whence, uint64_t *new_pos);
uint64_t     ext2_file_tell(ext2_file_t *file);
ext2_error_t ext2_file_truncate(ext2_file_t *file, uint64_t new_size);
ext2_error_t ext2_file_sync(ext2_file_t *file);*/
