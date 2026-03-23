#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

static char * str_get_next_forward_slash_addr(const char * str);

/* =========================================================================
 * UTILITIES
 * ========================================================================= */

/*ext2_error_t ext2_statvfs(ext2_fs_t *fs, uint32_t *total_blocks, uint32_t *free_blocks, uint32_t *total_inodes, uint32_t *free_inodes);*/

ext2_error_t ext2_lookup(ext2_fs_t *fs, const char *path, uint32_t *ino_out) {
    if (!fs || !path || !ino_out) 
        return EXT2_ERR_INVALID;
    
    /* must be an absolute path */
    if (path[0] != '/')
        return EXT2_ERR_INVALID;

    /* start at root */
    uint32_t current_ino = EXT2_ROOT_INO;

    /* skip leading slash */
    const char *p = path + 1;

    /* if path was just "/" return root */
    if (*p == '\0') {
        *ino_out = current_ino;
        return EXT2_OK;
    }

    ext2_error_t err;
    ext2_dir_t * dir;

    while (*path) {
        /* extract current component length */
        const char *next_slash    = str_get_next_forward_slash_addr(p);
        size_t      component_len = next_slash ? (size_t)(next_slash - p)
                                               : strlen(p);

        /* open current directory */
        err = ext2_dir_open_inode(fs, current_ino, &dir);
        if (err != EXT2_OK)
            return err;
        
        /* scan entries for a name match */
        ext2_dirent_t entry;
        uint8_t found = 0;

        while ((err = ext2_dir_read(dir, &entry)) == EXT2_OK) {
            if (component_len == entry.d_name_len &&
                strncmp(p, entry.d_name, component_len) == 0) {
                current_ino = entry.d_ino;
                found       = 1;
                break;
            }
        }

        err = ext2_dir_close(dir);
        if (err != EXT2_OK)
            return err;

        if (!found)
            return EXT2_ERR_NOT_FOUND;

        /* advance past component and slash */
        p += component_len;
        if (*p == '/')
            p++;
    }
    
    *ino_out = current_ino;
    return EXT2_OK;
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
 * Find the address of the first occasion of a forward slash in the string
 */
static char * str_get_next_forward_slash_addr(const char * str) {
    if (!str)
        return NULL;

    while (*str) {
        if (*str == '/')
            return (char *)str;
        str++;
    }

    return NULL;
}