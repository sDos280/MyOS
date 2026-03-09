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
ext2_error_t ext2_dir_open(ext2_fs_t *fs, const char *path, ext2_dir_t **dir_out);
*/
ext2_error_t ext2_dir_open_inode(ext2_fs_t *fs, uint32_t ino, ext2_dir_t **dir_out)
{
    if (!fs || !dir_out)
        return EXT2_ERR_INVALID;

    ext2_inode_t dir_inode;
    ext2_error_t err = ext2_inode_read(fs, ino, &dir_inode);
    if (err != EXT2_OK)
        return err;

    if (!(dir_inode.i_mode & EXT2_S_IFDIR))
        return EXT2_ERR_NOT_DIR;

    ext2_dir_t *dir_h = (ext2_dir_t *)kmalloc(sizeof(ext2_dir_t));
    if (!dir_h)
        return EXT2_ERR_NO_MEM;

    dir_h->fs                = fs;
    dir_h->ino               = ino;
    memcpy(&dir_h->inode, &dir_inode, sizeof(ext2_inode_t));
    dir_h->dir_size          = ext2_inode_get_size(fs, &dir_inode);
    dir_h->logical_block_idx = 0;
    dir_h->byte_offset       = 0;
    dir_h->buf_block_no      = 0;
    dir_h->buf_valid         = 0;

    *dir_out = dir_h;
    return EXT2_OK;
}
/*ext2_error_t ext2_dir_read(ext2_dir_t *dir, ext2_dirent_t *entry);
ext2_error_t ext2_dir_rewind(ext2_dir_t *dir);
ext2_error_t ext2_dir_close(ext2_dir_t *dir);*/