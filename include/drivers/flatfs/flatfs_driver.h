#ifndef FLATFS_H
#define FLATFS_H

#include "drivers/ata_driver.h"  /* ata_drive_t, ata_read28_request, etc. */

/* ─── tunables ─────────────────────────────────────────────────────────────── */

#define FLATFS_BLOCK_SIZE     ATA_SECTOR_SIZE  /* 1 block = 1 ATA sector (512 B)  */
#define FLATFS_MAX_FILES      128              /* maximum number of files          */
#define FLATFS_MAX_BLOCKS     4096             /* total data blocks on the drive   */
#define FLATFS_DIRECT_BLOCKS  8               /* block pointers per inode         */
#define FLATFS_NAME_MAX       32              /* max filename length (incl. NUL)   */

#define FLATFS_MAGIC          0x464C5453U     /* "FLTS" */

/*
 * Sector layout on disk
 * ─────────────────────────────────────────────────────────────────
 *  Sector 0          │ Superblock
 *  Sector 1          │ Inode bitmap   (1 bit per inode slot)
 *  Sector 2          │ Block bitmap   (1 bit per data block)
 *  Sectors 3 … N-1   │ Inode table    (one inode per sector)
 *  Sectors N … end   │ Data region    (raw file data)
 * ─────────────────────────────────────────────────────────────────
 *  N = 3 + FLATFS_MAX_FILES
 *
 * Every structure that is read or written is exactly one sector wide
 * so we never need to do sub-sector I/O.
 */

#define FLATFS_SECTOR_SUPERBLOCK   0
#define FLATFS_SECTOR_INODE_BITMAP 1
#define FLATFS_SECTOR_BLOCK_BITMAP 2
#define FLATFS_SECTOR_INODE_TABLE  3                            /* first inode sector */
#define FLATFS_SECTOR_DATA_START   (3 + FLATFS_MAX_FILES)      /* first data sector  */

/* ─── on-disk / in-memory structures ──────────────────────────────────────── */

/*
 * flatfs_superblock_t
 * Stored at sector FLATFS_SECTOR_SUPERBLOCK.
 * Padded to exactly ATA_SECTOR_SIZE bytes so a single sector read fills it.
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;           /* must equal FLATFS_MAGIC                       */
    uint32_t total_blocks;    /* usable data blocks (≤ FLATFS_MAX_BLOCKS)      */
    uint32_t total_inodes;    /* inode slots in the table (≤ FLATFS_MAX_FILES) */
    uint32_t free_blocks;     /* number of currently free data blocks          */
    uint32_t free_inodes;     /* number of currently free inode slots          */
    uint8_t  _pad[ATA_SECTOR_SIZE - 5 * sizeof(uint32_t)];
} flatfs_superblock_t;

/*
 * flatfs_inode_t
 * One entry per file; each inode occupies exactly one sector so it can be
 * read and written with a single ata_read28_request / ata_write28_request call.
 * Sector on disk = FLATFS_SECTOR_INODE_TABLE + inode_idx.
 */
typedef struct __attribute__((packed)) {
    char     name[FLATFS_NAME_MAX]; /* filename (NUL-terminated)               */
    uint8_t  in_use;                /* 1 = slot taken, 0 = free                */
    uint8_t  permissions;           /* rwx bit-field: bits 2=r 1=w 0=x        */
    uint16_t _pad0;
    uint32_t size;                  /* file size in bytes                      */
    uint32_t blocks[FLATFS_DIRECT_BLOCKS]; /* indices into the data region     */
    uint32_t block_count;           /* number of entries used in blocks[]      */
    uint32_t created_at;            /* unix timestamp                          */
    uint32_t modified_at;           /* unix timestamp                          */
    uint8_t  _pad1[ATA_SECTOR_SIZE
                   - FLATFS_NAME_MAX       /* name                             */
                   - 1                    /* in_use                            */
                   - 1                    /* permissions                       */
                   - 2                    /* _pad0                             */
                   - sizeof(uint32_t)     /* size                              */
                   - FLATFS_DIRECT_BLOCKS * sizeof(uint32_t) /* blocks[]       */
                   - sizeof(uint32_t)     /* block_count                       */
                   - sizeof(uint32_t)     /* created_at                        */
                   - sizeof(uint32_t)];   /* modified_at                       */
} flatfs_inode_t;

/* ─── error codes ──────────────────────────────────────────────────────────── */

typedef enum {
    FLATFS_OK            =  0,
    FLATFS_ERR_NO_SPACE  = -1,  /* disk or inode table is full               */
    FLATFS_ERR_NOT_FOUND = -2,  /* file with that name does not exist        */
    FLATFS_ERR_EXISTS    = -3,  /* a file with that name already exists      */
    FLATFS_ERR_BAD_MAGIC = -4,  /* superblock magic mismatch (not formatted) */
    FLATFS_ERR_INVALID   = -5,  /* NULL pointer or out-of-range argument     */
    FLATFS_ERR_IO        = -6,  /* ata_read28_request / ata_write28_request failed */
    FLATFS_ERR_CORRUPT   = -7,  /* on-disk data looks inconsistent           */
    FLATFS_ERR_NO_DRIVE  = -8,  /* ata_drive_t has exists == 0               */
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
    uint8_t inode_bitmap[(FLATFS_MAX_FILES  + 7) / 8]; /* 1 bit per inode    */
    uint8_t block_bitmap[(FLATFS_MAX_BLOCKS + 7) / 8]; /* 1 bit per block    */
    uint8_t sb_dirty;                 /* use to indicate if the superblock got dirty */
} flatfs_t;

/* ─── lifecycle ────────────────────────────────────────────────────────────── */

/*
 * flatfs_format
 * Write a fresh superblock, zeroed bitmaps, and an empty inode table to the
 * drive starting at sector 0. Destroys all existing data. Call once before
 * first use.
 *
 * `total_blocks` must be <= FLATFS_MAX_BLOCKS and must fit on the drive
 * (drive->size_in_sectors >= FLATFS_SECTOR_DATA_START + total_blocks).
 */
flatfs_err_t flatfs_format(ata_drive_t *drive,
                           uint32_t total_blocks,
                           uint32_t total_inodes);

/*
 * flatfs_mount
 * Read the superblock and bitmaps from the drive into `fs`.
 * Must be called before any other API function.
 */
flatfs_err_t flatfs_mount(flatfs_t *fs, ata_drive_t *drive);

/*
 * flatfs_unmount
 * Flush any dirty metadata to disk via ata_flush_cache, then invalidate `fs`.
 */
flatfs_err_t flatfs_unmount(flatfs_t *fs);

/* ─── file operations ──────────────────────────────────────────────────────── */

/*
 * flatfs_create
 * Allocate a new inode with the given name and write it to disk.
 * Returns FLATFS_ERR_EXISTS if a file with that name is already present.
 * On success, writes the inode index into *inode_idx if non-NULL.
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

#endif /* FLATFS_H */