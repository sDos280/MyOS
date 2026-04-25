#ifndef FLATFS_H
#define FLATFS_H

#include "drivers/ata_driver.h"  /* ata_drive_t, ata_read28_request, etc. */

/* ─── constants ───────────────────────────────────────────────────────────── */

#define FLATFS_MAGIC          0x464C5453U   /* "FLTS" */

#define FLATFS_DIRECT_BLOCKS  8
#define FLATFS_NAME_MAX       32

#define FLATFS_PERMISSION_X   1
#define FLATFS_PERMISSION_W   2
#define FLATFS_PERMISSION_R   4

#define FLATFS_BLOCK_SUPERBLOCK 0

/*
 * Sector layout on disk
 * ─────────────────────────────────────────────────────────────────
 *  [ superblock ] Block  0 (and one block size)
 *  [ inode bitmap ]
 *  [ block bitmap ]
 *  [ inode table ]
 *  [ data blocks ]
 * ─────────────────────────────────────────────────────────────────

/* ─── helpers ─────────────────────────────────────────────────────────────── */

#define FLATFS_DIV_ROUND_UP(x, y) (((x) + (y) - 1) / (y))

#define FLATFS_BLOCK_SIZE(sb) \
    ((sb)->sectors_per_block * ATA_SECTOR_SIZE)

#define FLATFS_BITMAP_BYTES(bits) \
    FLATFS_DIV_ROUND_UP((bits), 8)

#define FLATFS_BITMAP_BLOCKS(bits, sb) \
    FLATFS_DIV_ROUND_UP(FLATFS_BITMAP_BYTES(bits), FLATFS_BLOCK_SIZE(sb))

#define FLATFS_INODE_TABLE_BYTES(sb) \
    ((sb)->total_inodes * sizeof(flatfs_inode_t))

#define FLATFS_INODE_TABLE_BLOCKS(sb) \
    FLATFS_DIV_ROUND_UP(FLATFS_INODE_TABLE_BYTES(sb), FLATFS_BLOCK_SIZE(sb))

#define FLATFS_BLOCK_INODE_BITMAP(sb) \
    ((sb)->inode_bitmap_start)

#define FLATFS_BLOCK_BLOCK_BITMAP(sb) \
    ((sb)->block_bitmap_start)

#define FLATFS_BLOCK_INODE_TABLE(sb) \
    ((sb)->inode_table_start)

#define FLATFS_BLOCK_DATA_START(sb) \
    ((sb)->data_start_block)

#define FLATFS_DATA_BLOCK_SECTOR(sb, block_idx) \
    ((sb)->data_start_block + block_idx)

/* ─── on-disk / in-memory structures ──────────────────────────────────────── */

/*
 * flatfs_superblock_t
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;              /* must equal FLATFS_MAGIC                    */
    uint32_t sectors_per_block;  /* the amound of sectors per block            */
    uint32_t total_sectors;      /* drive size in sectors                      */
    uint32_t total_blocks;       /* usable data blocks                         */
    uint32_t total_inodes;       /* inode slots in the inode table             */
    uint32_t free_blocks;        /* number of currently free data blocks       */
    uint32_t free_inodes;        /* number of currently free inode slots       */
    uint32_t inode_bitmap_start;         /* block index of inode bitmap */
    uint32_t inode_bitmap_block_count;   /* block count of inode bitmap */
    uint32_t block_bitmap_start;         /* block index of block bitmap */
    uint32_t block_bitmap_block_count;   /* block count of block bitmap */
    uint32_t inode_table_start;          /* block index of inode table  */
    uint32_t inode_table_block_count;    /* block count of inode table  */
    uint32_t data_start_block;           /* index of fist data block    */
    uint8_t  _pad[ATA_SECTOR_SIZE - 14 * sizeof(uint32_t)];
} flatfs_superblock_t;

/*
 * flatfs_inode_t
 * One entry per file;
 */
typedef struct __attribute__((packed)) {
    char     name[FLATFS_NAME_MAX]; /* filename (NUL-terminated)               */
    uint8_t  in_use;                /* 1 = slot taken, 0 = free                */
    uint8_t  permissions;           /* rwx bit-field: bits 2=r 1=w 0=x        */
    uint16_t _pad0;
    uint32_t size;                  /* file size in bytes                      */
    uint32_t blocks[FLATFS_DIRECT_BLOCKS]; /* indices into the data region 0 if not allocated */
    uint32_t block_count;           /* number of entries used in blocks[]      */
    uint32_t created_at;            /* unix timestamp                          */
    uint32_t modified_at;           /* unix timestamp                          */
} flatfs_inode_t;

/* ─── error codes ──────────────────────────────────────────────────────────── */

typedef enum {
    FLATFS_OK            =  0,
    FLATFS_ERR_NO_SPACE  = -1,  /* disk or inode table is full               */
    FLATFS_ERR_NO_MEM    = -2,  /* no more free memory                       */
    FLATFS_ERR_NOT_FOUND = -3,  /* file with that name does not exist        */
    FLATFS_ERR_EXISTS    = -4,  /* a file with that name already exists      */
    FLATFS_ERR_BAD_MAGIC = -5,  /* superblock magic mismatch (not formatted) */
    FLATFS_ERR_INVALID   = -6,  /* NULL pointer or out-of-range argument     */
    FLATFS_ERR_IO        = -7,  /* ata_read28_request / ata_write28_request failed */
    FLATFS_ERR_CORRUPT   = -8,  /* on-disk data looks inconsistent           */
    FLATFS_ERR_NO_DRIVE  = -9,  /* ata_drive_t has exists == 0               */
} flatfs_err_t;

/* ─── file-system handle ───────────────────────────────────────────────────── */

/*
 * flatfs_t
 * In-memory state for a mounted flat file system.
 * All I/O goes through the embedded ata_drive_t pointer using
 * ata_read28_request / ata_write28_request / ata_flush_cache.
 */
typedef struct {
    ata_drive_t        *drive;        /* the ATA drive backing this FS        */
    flatfs_superblock_t sb;           /* cached superblock (sector 0)         */
    uint8_t *inode_bitmap;            /* 1 bit per inode    */
    uint8_t *block_bitmap;            /* 1 bit per block    */
} flatfs_t;

/* ─── lifecycle ────────────────────────────────────────────────────────────── */

/*
 * Formats the drive.
 *
 * total_inodes:
 *     number of inode slots to create.
 *
 * sectors_per_block:
 *     number of ATA sectors per FlatFS data block.
 *
 * total_blocks is no longer passed in. It should be computed from drive size:
 *
 * metadata sectors =
 *     1
 *   + inode_bitmap_sectors
 *   + block_bitmap_sectors
 *   + inode_table_sectors
 *
 * data sectors =
 *     drive->size_in_sectors - metadata sectors
 *
 * total_blocks =
 *     data sectors / sectors_per_block
 *
 * Because block_bitmap_sectors depends on total_blocks, the formatter should
 * compute this iteratively until stable.
 */
flatfs_err_t flatfs_format(ata_drive_t *drive,
                           uint32_t total_inodes,
                           uint32_t sectors_per_block);

/*
 * flatfs_mount
 * Read the superblock and bitmaps from the drive into `fs`.
 * Must be called before any other API function.
 */
flatfs_err_t flatfs_mount(flatfs_t *fs, ata_drive_t *drive);

/*
 * flatfs_unmount
 * Flush any dirty metadata to disk via ata_flush_cache.
 */
flatfs_err_t flatfs_unmount(flatfs_t *fs);

/* ─── file operations ──────────────────────────────────────────────────────── */

/*
 * flatfs_create
 * Allocate a new inode with the given name and write it to disk.
 * Returns FLATFS_ERR_EXISTS if a file with that name is already present.
 * On success, writes the inode index into *inode_idx.
 */
flatfs_err_t flatfs_create(flatfs_t *fs,
                           const char *name,
                           uint8_t permissions,
                           uint32_t *inode_idx);

/*
 * flatfs_delete
 * Free the inode and all data sectors belonging to the named file.
 * Calls ata_flush_cache after updating metadata.
 */
flatfs_err_t flatfs_delete(flatfs_t *fs, const char *name);

/*
 * flatfs_write
 * Write `size` bytes from `buf` into the file at byte offset `offset`.
 * Allocates new data sectors (each ATA_SECTOR_SIZE bytes) as needed.
 * Growing past FLATFS_DIRECT_BLOCKS * ATA_SECTOR_SIZE returns FLATFS_ERR_NO_SPACE.
 *
 * Each sector is written with a single ata_write28_request call.
 */
flatfs_err_t flatfs_write(flatfs_t *fs,
                          const char *name,
                          uint32_t offset,
                          const uint8_t *buf,
                          uint32_t size);

/*
 * flatfs_read
 * Copy up to `size` bytes starting at byte `offset` of the named file
 * into `buf`. Reading past EOF stops at the file boundary.
 * The number of bytes actually copied is written to *bytes_read if non-NULL.
 *
 * Each sector is read with a single ata_read28_request call.
 */
flatfs_err_t flatfs_read(flatfs_t *fs,
                         const char *name,
                         uint32_t offset,
                         uint8_t *buf,
                         uint32_t size,
                         uint32_t *bytes_read);

/*
 * flatfs_truncate
 * Shrink the file to exactly `new_size` bytes (growing is not supported).
 * Freed sectors are returned to the block bitmap and the inode is updated.
 */
flatfs_err_t flatfs_truncate(flatfs_t *fs,
                             const char *name,
                             uint32_t new_size);

/*
 * flatfs_rename
 * Update the name field of an inode in-place on disk.
 * Returns FLATFS_ERR_EXISTS if `new_name` is already taken.
 */
flatfs_err_t flatfs_rename(flatfs_t *fs,
                           const char *old_name,
                           const char *new_name);

/* ─── metadata & lookup ────────────────────────────────────────────────────── */

/*
 * flatfs_stat
 * Read the inode for the named file off disk and copy it into *out.
 */
flatfs_err_t flatfs_stat(flatfs_t *fs,
                         const char *name,
                         flatfs_inode_t *out);

/*
 * flatfs_find
 * Scan the inode table sectors to resolve a filename to its inode index.
 * Writes the index into *inode_idx. Returns FLATFS_ERR_NOT_FOUND if absent.
 */
flatfs_err_t flatfs_find(flatfs_t *fs,
                         const char *name,
                         uint32_t *inode_idx);

/*
 * flatfs_get_inode_by_index
 * Return the inode spesified by the index.
 * Writes the index into *inode_idx. Returns FLATFS_ERR_NOT_FOUND if absent.
 */
flatfs_err_t flatfs_get_inode_by_index(flatfs_t *fs,
                         uint32_t inode_idx,
                         flatfs_inode_t *inode);

/* ─── directory-style listing ──────────────────────────────────────────────── */

/*
 * flatfs_list_cb
 * Callback for flatfs_list(). Invoked once per live inode.
 *   inode     – sector-sized buffer holding the inode (valid during call only)
 *   inode_idx – position of this inode in the table
 *   userdata  – value passed to flatfs_list()
 * Return 0 to continue, non-zero to stop early.
 */
typedef int (*flatfs_list_cb)(const flatfs_inode_t *inode,
                              uint32_t inode_idx,
                              void *userdata);

/*
 * flatfs_list
 * Read each inode sector in the table and invoke `cb` for every in-use entry.
 * Uses one ata_read28_request per inode. Stops if `cb` returns non-zero.
 */
flatfs_err_t flatfs_list(flatfs_t *fs, flatfs_list_cb cb, void *userdata);

/* ─── diagnostics ──────────────────────────────────────────────────────────── */

/*
 * flatfs_free_blocks  – free data blocks remaining (from cached superblock)
 * flatfs_free_inodes  – free inode slots remaining (from cached superblock)
 * flatfs_check        – re-read every inode sector and verify bitmap/size
 *                       consistency; returns FLATFS_ERR_CORRUPT on failure
 */
uint32_t     flatfs_free_blocks(const flatfs_t *fs);
uint32_t     flatfs_free_inodes(const flatfs_t *fs);
flatfs_err_t flatfs_check(flatfs_t *fs);

/* ─── utils ──────────────────────────────────────────────────────── */

/*
 * flatfs_write_block
 * Write FlatFS block into drive memory.
 *  block_idx - block index in flatfs relative terms
 *  data      - the data to be writen (must be a block size) */
flatfs_err_t flatfs_write_block(flatfs_t *fs, uint32_t block_idx, const uint8_t *data);

/*
 * flatfs_read_block
 * Read FlatFS block into drive memory.
 *  block_idx - block index in flatfs relative terms
 *  buffer    - buffer for read data (must be a block size) */
flatfs_err_t flatfs_read_block(flatfs_t *fs, uint32_t block_idx, uint8_t *buffer);

#endif /* FLATFS_H */