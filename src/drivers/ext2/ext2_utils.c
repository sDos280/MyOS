#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

/* =========================================================================
 * UTILITIES
 * ========================================================================= */

/*ext2_error_t ext2_statvfs(ext2_fs_t *fs, uint32_t *total_blocks, uint32_t *free_blocks, uint32_t *total_inodes, uint32_t *free_inodes);*/

ext2_error_t ext2_lookup(ext2_fs_t *fs, const char *path, uint32_t *ino_out) {
    if (!fs || !path || !ino_out) 
        return EXT2_ERR_INVALID;
    
    ext2_inode_t temp;
    
    /* read the root directory */
    ext2_error_t err = ext2_inode_read(fs, EXT2_ROOT_INO, &temp);
    if (err != EXT2_OK)
        return err;
    
    char * next_backslash = str_get_next_backslash_addr(path);

    /* NOT FINISHED */
}

/*int          ext2_path_exists(ext2_fs_t *fs, const char *path);
int          ext2_is_dir(ext2_fs_t *fs, const char *path);
int          ext2_is_file(ext2_fs_t *fs, const char *path);
int          ext2_is_symlink(ext2_fs_t *fs, const char *path);
const char  *ext2_strerror(ext2_error_t err);*/

/* =========================================================================
 * INTERNAL HELPERS
 * ========================================================================= */
/*
 * Find the address of the first occasion of a backslash in the string
 */
static char * str_get_next_backslash_addr(const char * str) {
    while (str) {
        if (*str == '\\') return str;
        str++;
    }

    return NULL;
}