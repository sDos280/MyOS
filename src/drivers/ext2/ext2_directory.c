#include "mm/kheap.h"
#include "drivers/ext2/ext2_driver.h"
#include "bitmap_util.h"
#include "utils.h"

/* =========================================================================
 * DIRECTORY OPERATIONS
 * ========================================================================= */
ext2_error_t ext2_mkdir(ext2_fs_t *fs, const char *path, uint16_t mode) {
    if (!fs || !path)
        return EXT2_ERR_INVALID;

    if (fs->read_only)
        return EXT2_ERR_READ_ONLY;

    if (path[0] != '/')
        return EXT2_ERR_INVALID;

    /* ------------------------------------------------------------------ */
    /* 1. Check the directory doesn't already exist                        */
    /* ------------------------------------------------------------------ */
    uint32_t temp_ino;
    ext2_error_t err = ext2_lookup(fs, path, &temp_ino);
    if (err == EXT2_OK)
        return EXT2_ERR_EXISTS;

    /* ------------------------------------------------------------------ */
    /* 2. Split path into parent path and new directory name               */
    /* ------------------------------------------------------------------ */
    char path_copy[EXT2_MAX_PATH_LEN];
    strncpy(path_copy, path, EXT2_MAX_PATH_LEN - 1);
    path_copy[EXT2_MAX_PATH_LEN - 1] = '\0';

    char *last_slash = str_get_last_forward_slash_addr(path_copy);
    *last_slash = '\0';

    const char *dir_name   = last_slash + 1;
    const char *parent_path = (path_copy[0] == '\0') ? "/" : path_copy;

    /* ------------------------------------------------------------------ */
    /* 3. Resolve parent path                                              */
    /* ------------------------------------------------------------------ */
    uint32_t parent_ino;
    err = ext2_lookup(fs, parent_path, &parent_ino);
    if (err != EXT2_OK)
        return err;

    /* ------------------------------------------------------------------ */
    /* 4. Read parent inode                                                */
    /* ------------------------------------------------------------------ */
    ext2_inode_t parent_inode;
    err = ext2_inode_read(fs, parent_ino, &parent_inode);
    if (err != EXT2_OK)
        return err;

    /* ------------------------------------------------------------------ */
    /* 5. Allocate new inode                                               */
    /* ------------------------------------------------------------------ */
    uint32_t new_ino;
    uint32_t parent_group = (parent_ino - 1) / fs->superblock.s_inodes_per_group;
    err = ext2_inode_alloc(fs, parent_group, &new_ino);
    if (err != EXT2_OK)
        return err;

    /* ------------------------------------------------------------------ */
    /* 6. Allocate data block for new directory                            */
    /* ------------------------------------------------------------------ */
    uint32_t new_block_no;
    err = ext2_block_alloc(fs, parent_group, &new_block_no);
    if (err != EXT2_OK) {
        ext2_inode_free(fs, new_ino);
        return err;
    }

    /* ------------------------------------------------------------------ */
    /* 7. Initialize new directory inode                                   */
    /* ------------------------------------------------------------------ */
    ext2_inode_t new_inode;
    memset(&new_inode, 0, sizeof(ext2_inode_t));
    new_inode.i_mode        = EXT2_S_IFDIR | mode;
    new_inode.i_uid         = 0;
    new_inode.i_size        = fs->block_size;
    new_inode.i_links_count = 2;        /* one for parent entry, one for "." */
    new_inode.i_blocks      = fs->sectors_per_block;
    new_inode.i_block[0]    = new_block_no;

    /* ------------------------------------------------------------------ */
    /* 8. Write "." and ".." entries into the new block                    */
    /*    "."  rec_len = 12  (smallest aligned size for 1-char name)       */
    /*    ".." rec_len = block_size - 12  (fills rest of block)            */
    /* ------------------------------------------------------------------ */
    uint8_t *block_buf = (uint8_t *)kmalloc(fs->block_size);
    if (!block_buf) {
        ext2_inode_free(fs, new_ino);
        ext2_block_free(fs, new_block_no);
        return EXT2_ERR_NO_MEM;
    }
    memset(block_buf, 0, fs->block_size);

    /* "." entry */
    ext2_dir_entry_t *dot = (ext2_dir_entry_t *)block_buf;
    dot->inode     = new_ino;
    dot->rec_len   = 12;
    dot->name_len  = 1;
    dot->file_type = EXT2_FT_DIR;
    dot->name[0]   = '.';

    /* ".." entry — rec_len fills the rest of the block */
    ext2_dir_entry_t *dot_dot = (ext2_dir_entry_t *)(block_buf + 12);
    dot_dot->inode     = parent_ino;
    dot_dot->rec_len   = fs->block_size - 12;
    dot_dot->name_len  = 2;
    dot_dot->file_type = EXT2_FT_DIR;
    dot_dot->name[0]   = '.';
    dot_dot->name[1]   = '.';

    err = ext2_block_write(fs, new_block_no, block_buf);
    kfree(block_buf);
    if (err != EXT2_OK) {
        ext2_inode_free(fs, new_ino);
        ext2_block_free(fs, new_block_no);
        return err;
    }

    /* ------------------------------------------------------------------ */
    /* 9. Write new inode to disk                                          */
    /* ------------------------------------------------------------------ */
    err = ext2_inode_write(fs, new_ino, &new_inode);
    if (err != EXT2_OK) {
        ext2_inode_free(fs, new_ino);
        ext2_block_free(fs, new_block_no);
        return err;
    }

    /* ------------------------------------------------------------------ */
    /* 10. Add entry for new directory in parent                           */
    /* ------------------------------------------------------------------ */
    err = ext2_dir_add_entry(fs, parent_ino, new_ino, dir_name, EXT2_FT_DIR);
    if (err != EXT2_OK) {
        ext2_inode_free(fs, new_ino);
        ext2_block_free(fs, new_block_no);
        return err;
    }

    /* ------------------------------------------------------------------ */
    /* 11. Update parent inode link count                                  */
    /* ------------------------------------------------------------------ */
    parent_inode.i_links_count++;
    err = ext2_inode_write(fs, parent_ino, &parent_inode);
    if (err != EXT2_OK)
        return err;

    /* ------------------------------------------------------------------ */
    /* 12. Update block group used_dirs count                              */
    /* ------------------------------------------------------------------ */
    uint32_t new_group = (new_ino - 1) / fs->superblock.s_inodes_per_group;
    fs->bgdt[new_group].bg_used_dirs_count++;
    fs->bgdt_dirty = 1;

    return EXT2_OK;
}

/*ext2_error_t ext2_mkdir_p(ext2_fs_t *fs, const char *path, uint16_t mode);
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

    ext2_dir_t *dir_h = (ext2_dir_t *)kalloc(sizeof(ext2_dir_t));
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

/* =========================================================================
 * INTERNAL HELPERS
 * ========================================================================= */
/*
 * Find the address of the last occasion of a forward slash in the string
 */
static char * str_get_last_forward_slash_addr(const char * str) {
    if (!str)
        return NULL;

    char * last = NULL; /* the first index should be */
    while (*str) {
        if (*str == '/')
            last = str;
        str++;
    }

    return last;
}