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
ext2_error_t ext2_rmdir_r(ext2_fs_t *fs, const char *path);*/

ext2_error_t ext2_dir_open(ext2_fs_t *fs, const char *path, ext2_dir_t **dir_out) {
    if (!fs || !path || !dir_out)
        return EXT2_ERR_INVALID;
    
    uint32_t dir_ino;

    ext2_error_t err = ext2_lookup(fs, path, &dir_ino);
    if (err != EXT2_OK)
        return err;
    
    return ext2_dir_open_inode(fs, dir_ino, dir_out);
}

ext2_error_t ext2_dir_open_inode(ext2_fs_t *fs, uint32_t ino, ext2_dir_t **dir_out) {
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
    memset(dir_h->block_buf, 0, EXT2_MAX_BLOCK_SIZE);
    dir_h->logical_block_idx = 0;
    dir_h->byte_offset       = 0;
    dir_h->buf_block_no      = 0;
    dir_h->buf_valid         = 0;

    *dir_out = dir_h;
    return EXT2_OK;
}

ext2_error_t ext2_dir_read(ext2_dir_t *dir, ext2_dirent_t *entry) {
    if (!dir || !entry)
        return EXT2_ERR_INVALID;

    ext2_error_t err;

next_entry:
    /* ------------------------------------------------------------------ */
    /* End of directory check                                              */
    /* ------------------------------------------------------------------ */
    uint64_t current_pos = (uint64_t)dir->logical_block_idx
                             * dir->fs->block_size
                             + dir->byte_offset;

    if (current_pos >= dir->dir_size)
        return EXT2_ERR_NOT_FOUND;

    /* ------------------------------------------------------------------ */
    /* Load block into buffer if not valid                                 */
    /* ------------------------------------------------------------------ */
    if (!dir->buf_valid) {
        uint32_t phys_block;
        err = ext2_inode_resolve_block(dir->fs, &dir->inode,
                                       dir->logical_block_idx, &phys_block);
        if (err != EXT2_OK)
            return err;

        err = ext2_block_read(dir->fs, phys_block, dir->block_buf);
        if (err != EXT2_OK)
            return err;

        dir->buf_block_no = phys_block;
        dir->buf_valid    = 1;
    }

    /* ------------------------------------------------------------------ */
    /* Read entry at current byte_offset                                   */
    /* ------------------------------------------------------------------ */
    ext2_dir_entry_t *raw = (ext2_dir_entry_t *)(dir->block_buf
                                                  + dir->byte_offset);

    /* ------------------------------------------------------------------ */
    /* Advance position by rec_len (variable length entries)              */
    /* ------------------------------------------------------------------ */
    dir->byte_offset += raw->rec_len;

    if (dir->byte_offset >= dir->fs->block_size) {
        dir->logical_block_idx++;
        dir->byte_offset = 0;
        dir->buf_valid   = 0;
    }

    /* ------------------------------------------------------------------ */
    /* Skip deleted entries (inode == 0)                                   */
    /* ------------------------------------------------------------------ */
    if (raw->inode == 0)
        goto next_entry;

    /* ------------------------------------------------------------------ */
    /* Populate output entry                                               */
    /* ------------------------------------------------------------------ */
    entry->d_ino      = raw->inode;
    entry->d_type     = raw->file_type;
    entry->d_name_len = raw->name_len;
    memcpy(entry->d_name, raw->name, raw->name_len);
    entry->d_name[raw->name_len] = '\0';

    return EXT2_OK;
}

ext2_error_t ext2_dir_rewind(ext2_dir_t *dir) {
    if (!dir)
        return EXT2_ERR_INVALID;

    dir->logical_block_idx = 0;
    dir->byte_offset       = 0;
    dir->buf_valid         = 0;

    return EXT2_OK;
}

ext2_error_t ext2_dir_close(ext2_dir_t *dir) {
    if (!dir)
        return EXT2_ERR_INVALID;
    
    kfree(dir);
    return EXT2_OK;
}