/**
 * ext2.h - Ext2 Filesystem Library
 *
 * Provides structures and functions for working with Ext2 filesystems,
 * including mounting, file/directory creation, deletion, reading, writing,
 * and metadata management.
 *
 * Usage:
 *   ext2_fs_t *fs = ext2_mount("/dev/sda1", EXT2_MOUNT_RW);
 *   ext2_file_t *f = ext2_file_open(fs, "/hello.txt", EXT2_O_CREAT | EXT2_O_WRONLY);
 *   ext2_file_write(f, "Hello, world!\n", 14);
 *   ext2_file_close(f);
 *   ext2_unmount(fs);
 */

#ifndef EXT2_DRIVER_H
#define EXT2_DRIVER_H

#include "drivers/ata_driver.h"
#include "types.h"

/* =========================================================================
 * CONSTANTS & MAGIC VALUES
 * ========================================================================= */

#define EXT2_SUPER_MAGIC            0xEF53  /* Ext2 superblock magic number */
#define EXT2_SUPERBLOCK_OFFSET      1024    /* Byte offset of superblock from partition start */
#define EXT2_GOOD_OLD_INODE_SIZE    128     /* Default inode size in revision 0 */
#define EXT2_GOOD_OLD_FIRST_INO     11      /* First non-reserved inode in revision 0 */
#define EXT2_ROOT_INO               2       /* Inode number of the root directory */
#define EXT2_MAX_NAME_LEN           255     /* Maximum filename length */
#define EXT2_MAX_PATH_LEN           4096    /* Maximum path length */
#define EXT2_NDIR_BLOCKS            12      /* Number of direct block pointers */
#define EXT2_IND_BLOCK              12      /* Index of singly indirect block pointer */
#define EXT2_DIND_BLOCK             13      /* Index of doubly indirect block pointer */
#define EXT2_TIND_BLOCK             14      /* Index of triply indirect block pointer */
#define EXT2_N_BLOCKS               15      /* Total number of block pointers in inode */


/* =========================================================================
 * ERROR CODES
 * ========================================================================= */

typedef enum ext2_error {
    EXT2_OK                 =  0,   /* Success */
    EXT2_ERR_IO             = -1,   /* I/O error reading/writing device */
    EXT2_ERR_CORRUPT        = -2,   /* Filesystem appears corrupted */
    EXT2_ERR_INVALID        = -3,   /* Invalid argument */
    EXT2_ERR_NOT_FOUND      = -4,   /* File or directory not found */
    EXT2_ERR_EXISTS         = -5,   /* File or directory already exists */
    EXT2_ERR_NO_SPACE       = -6,   /* No space left on device */
    EXT2_ERR_NO_INODES      = -7,   /* No free inodes available */
    EXT2_ERR_NOT_DIR        = -8,   /* Not a directory */
    EXT2_ERR_IS_DIR         = -9,   /* Is a directory */
    EXT2_ERR_NOT_EMPTY      = -10,  /* Directory is not empty */
    EXT2_ERR_PERM           = -11,  /* Permission denied */
    EXT2_ERR_READ_ONLY      = -12,  /* Filesystem is mounted read-only */
    EXT2_ERR_NOT_MOUNTED    = -13,  /* Filesystem is not mounted */
    EXT2_ERR_ALREADY_MOUNTED= -14,  /* Filesystem is already mounted */
    EXT2_ERR_BAD_MAGIC      = -15,  /* Bad magic number (not an ext2 filesystem) */
    EXT2_ERR_UNSUPPORTED    = -16,  /* Unsupported filesystem feature */
    EXT2_ERR_OVERFLOW       = -17,  /* Value would overflow */
    EXT2_ERR_SYMLINK_LOOP   = -18,  /* Too many symbolic link levels */
    EXT2_ERR_NAME_TOO_LONG  = -19,  /* Filename too long */
    EXT2_ERR_NO_MEM         = -20,  /* Out of memory */
} ext2_error_t;

/** Returns a human-readable string describing the given error code. */
const char *ext2_strerror(ext2_error_t err);


/* =========================================================================
 * ON-DISK STRUCTURES
 * ========================================================================= */

/**
 * Superblock Structure (on-disk layout)
 * Always located at byte offset 1024 from the start of the partition.
 * Total size: 1024 bytes.
 */
typedef struct __attribute__((packed)) ext2_superblock {
    uint32_t s_inodes_count;            /* Total number of inodes */
    uint32_t s_blocks_count;            /* Total number of blocks */
    uint32_t s_r_blocks_count;          /* Blocks reserved for superuser */
    uint32_t s_free_blocks_count;       /* Total free blocks (incl. reserved) */
    uint32_t s_free_inodes_count;       /* Total free inodes */
    uint32_t s_first_data_block;        /* Block ID containing the superblock */
    uint32_t s_log_block_size;          /* Block size = 1024 << s_log_block_size */
    uint32_t s_log_frag_size;           /* Fragment size */
    uint32_t s_blocks_per_group;        /* Blocks per block group */
    uint32_t s_frags_per_group;         /* Fragments per block group */
    uint32_t s_inodes_per_group;        /* Inodes per block group */
    uint32_t s_mtime;                   /* Unix time of last mount */
    uint32_t s_wtime;                   /* Unix time of last write */
    uint16_t s_mnt_count;               /* Mounts since last full check */
    uint16_t s_max_mnt_count;           /* Max mounts before full check */
    uint16_t s_magic;                   /* Magic number: 0xEF53 */
    uint16_t s_state;                   /* Filesystem state */
    uint16_t s_errors;                  /* Error handling method */
    uint16_t s_minor_rev_level;         /* Minor revision level */
    uint32_t s_lastcheck;               /* Unix time of last check */
    uint32_t s_checkinterval;           /* Max interval between checks */
    uint32_t s_creator_os;              /* OS that created the filesystem */
    uint32_t s_rev_level;               /* Revision level */
    uint16_t s_def_resuid;              /* Default UID for reserved blocks */
    uint16_t s_def_resgid;              /* Default GID for reserved blocks */
    /* EXT2_DYNAMIC_REV specific */
    uint32_t s_first_ino;               /* First usable inode number */
    uint16_t s_inode_size;              /* Size of inode structure in bytes */
    uint16_t s_block_group_nr;          /* Block group hosting this superblock */
    uint32_t s_feature_compat;          /* Compatible feature bitmask */
    uint32_t s_feature_incompat;        /* Incompatible feature bitmask */
    uint32_t s_feature_ro_compat;       /* Read-only compatible feature bitmask */
    uint8_t  s_uuid[16];                /* 128-bit volume UUID */
    char     s_volume_name[16];         /* Volume name (null-terminated) */
    char     s_last_mounted[64];        /* Last mount path (null-terminated) */
    uint32_t s_algo_bitmap;             /* Compression algorithm bitmap */
    /* Performance hints */
    uint8_t  s_prealloc_blocks;         /* Blocks to preallocate for files */
    uint8_t  s_prealloc_dir_blocks;     /* Blocks to preallocate for dirs */
    uint16_t _padding;
    /* Journaling support */
    uint8_t  s_journal_uuid[16];        /* UUID of journal superblock */
    uint32_t s_journal_inum;            /* Inode number of journal file */
    uint32_t s_journal_dev;             /* Device number of journal file */
    uint32_t s_last_orphan;             /* First inode in orphan deletion list */
    /* Directory indexing */
    uint32_t s_hash_seed[4];            /* Hash seeds for directory indexing */
    uint8_t  s_def_hash_version;        /* Default hash version */
    uint8_t  _reserved_padding[3];
    /* Other options */
    uint32_t s_default_mount_options;   /* Default mount options */
    uint32_t s_first_meta_bg;           /* Block group ID of first meta block group */
    uint8_t  _unused[760];              /* Reserved for future revisions */
} ext2_superblock_t;

/** s_state values */
#define EXT2_VALID_FS   1   /* Cleanly unmounted */
#define EXT2_ERROR_FS   2   /* Errors detected */

/** s_errors values */
#define EXT2_ERRORS_CONTINUE    1   /* Continue as if nothing happened */
#define EXT2_ERRORS_RO          2   /* Remount read-only */
#define EXT2_ERRORS_PANIC       3   /* Kernel panic */

/** s_creator_os values */
#define EXT2_OS_LINUX   0
#define EXT2_OS_HURD    1
#define EXT2_OS_MASIX   2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_LITES   4

/** s_rev_level values */
#define EXT2_GOOD_OLD_REV   0   /* Revision 0 */
#define EXT2_DYNAMIC_REV    1   /* Revision 1 (variable inode sizes, ext attrs, etc.) */

/** s_feature_compat bitmask values */
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC    0x0001  /* Block pre-allocation for dirs */
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES   0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL     0x0004  /* Ext3 journal exists */
#define EXT2_FEATURE_COMPAT_EXT_ATTR        0x0008  /* Extended inode attributes */
#define EXT2_FEATURE_COMPAT_RESIZE_INO      0x0010  /* Non-standard inode size */
#define EXT2_FEATURE_COMPAT_DIR_INDEX       0x0020  /* Directory HTree indexing */

/** s_feature_incompat bitmask values */
#define EXT2_FEATURE_INCOMPAT_COMPRESSION   0x0001  /* Disk/file compression */
#define EXT2_FEATURE_INCOMPAT_FILETYPE      0x0002  /* Directory entries store file type */
#define EXT3_FEATURE_INCOMPAT_RECOVER       0x0004  /* Journal recovery needed */
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV   0x0008  /* Journal on external device */
#define EXT2_FEATURE_INCOMPAT_META_BG       0x0010  /* Meta block groups */

/** s_feature_ro_compat bitmask values */
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001  /* Sparse superblock copies */
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002  /* 64-bit file size support */
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR    0x0004  /* B-tree sorted directories */


/**
 * Block Group Descriptor Structure (on-disk layout)
 * One per block group. The table starts on the block immediately after
 * the superblock.
 * Total size: 32 bytes.
 */
typedef struct __attribute__((packed)) ext2_block_group_descriptor {
    uint32_t bg_block_bitmap;       /* Block ID of the block usage bitmap */
    uint32_t bg_inode_bitmap;       /* Block ID of the inode usage bitmap */
    uint32_t bg_inode_table;        /* Block ID of the first inode table block */
    uint16_t bg_free_blocks_count;  /* Number of free blocks in this group */
    uint16_t bg_free_inodes_count;  /* Number of free inodes in this group */
    uint16_t bg_used_dirs_count;    /* Number of inodes allocated to directories */
    uint16_t bg_pad;                /* Padding for 32-bit alignment */
    uint8_t  bg_reserved[12];       /* Reserved for future use */
} ext2_block_group_descriptor_t;


/**
 * Inode Structure (on-disk layout)
 * Describes a single file, directory, symlink, or special file.
 * Base size: 128 bytes (EXT2_GOOD_OLD_INODE_SIZE).
 */
typedef struct __attribute__((packed)) ext2_inode {
    uint16_t i_mode;            /* File type and access rights */
    uint16_t i_uid;             /* Lower 16 bits of owner UID */
    uint32_t i_size;            /* File size in bytes (lower 32 bits) */
    uint32_t i_atime;           /* Unix time of last access */
    uint32_t i_ctime;           /* Unix time of creation / last metadata change */
    uint32_t i_mtime;           /* Unix time of last data modification */
    uint32_t i_dtime;           /* Unix time of deletion (0 if not deleted) */
    uint16_t i_gid;             /* Lower 16 bits of owning group ID */
    uint16_t i_links_count;     /* Number of hard links to this inode */
    uint32_t i_blocks;          /* Number of 512-byte blocks allocated */
    uint32_t i_flags;           /* Inode behaviour flags */
    uint8_t  i_osd1[4];         /* OS-dependent value #1 */
    uint32_t i_block[15];       /* Block pointers (12 direct, 1 ind, 1 dind, 1 tind) */
    uint32_t i_generation;      /* File version (used by NFS) */
    uint32_t i_file_acl;        /* Block number of extended attributes block */
    uint32_t i_dir_acl;         /* Upper 32 bits of file size (regular files) or dir ACL */
    uint32_t i_faddr;           /* Fragment address (obsolete) */
    uint8_t  i_osd2[12];        /* OS-dependent value #2 */
} ext2_inode_t;

/** i_mode file type masks */
#define EXT2_S_IFSOCK   0xC000  /* Socket */
#define EXT2_S_IFLNK    0xA000  /* Symbolic link */
#define EXT2_S_IFREG    0x8000  /* Regular file */
#define EXT2_S_IFBLK    0x6000  /* Block device */
#define EXT2_S_IFDIR    0x4000  /* Directory */
#define EXT2_S_IFCHR    0x2000  /* Character device */
#define EXT2_S_IFIFO    0x1000  /* FIFO / named pipe */

/** i_mode access right masks */
#define EXT2_S_ISUID    0x0800  /* Set-UID on execution */
#define EXT2_S_ISGID    0x0400  /* Set-GID on execution */
#define EXT2_S_ISVTX    0x0200  /* Sticky bit */
#define EXT2_S_IRUSR    0x0100  /* Owner read */
#define EXT2_S_IWUSR    0x0080  /* Owner write */
#define EXT2_S_IXUSR    0x0040  /* Owner execute */
#define EXT2_S_IRGRP    0x0020  /* Group read */
#define EXT2_S_IWGRP    0x0010  /* Group write */
#define EXT2_S_IXGRP    0x0008  /* Group execute */
#define EXT2_S_IROTH    0x0004  /* Others read */
#define EXT2_S_IWOTH    0x0002  /* Others write */
#define EXT2_S_IXOTH    0x0001  /* Others execute */

/** i_flags values */
#define EXT2_SECRM_FL           0x00000001  /* Secure deletion */
#define EXT2_UNRM_FL            0x00000002  /* Record for undelete */
#define EXT2_COMPR_FL           0x00000004  /* Compressed file */
#define EXT2_SYNC_FL            0x00000008  /* Synchronous updates */
#define EXT2_IMMUTABLE_FL       0x00000010  /* Immutable file */
#define EXT2_APPEND_FL          0x00000020  /* Append-only file */
#define EXT2_NODUMP_FL          0x00000040  /* Do not dump/delete */
#define EXT2_NOATIME_FL         0x00000080  /* Do not update atime */
#define EXT2_DIRTY_FL           0x00000100  /* Dirty (modified) */
#define EXT2_COMPRBLK_FL        0x00000200  /* Compressed blocks */
#define EXT2_NOCOMPR_FL         0x00000400  /* Access raw compressed data */
#define EXT2_ECOMPR_FL          0x00000800  /* Compression error */
#define EXT2_BTREE_FL           0x00001000  /* B-tree format directory */
#define EXT2_INDEX_FL           0x00001000  /* Hash-indexed directory */
#define EXT2_IMAGIC_FL          0x00002000  /* AFS directory */
#define EXT3_JOURNAL_DATA_FL    0x00004000  /* Journal file data */
#define EXT2_RESERVED_FL        0x80000000  /* Reserved for Ext2 library */


/**
 * Directory Entry Structure (on-disk layout)
 * Variable-length; entries are packed sequentially within directory blocks.
 * Each entry is aligned to a 4-byte boundary.
 */
typedef struct __attribute__((packed)) ext2_dir_entry {
    uint32_t inode;             /* Inode number of the referenced file (0 = unused) */
    uint16_t rec_len;           /* Length of this directory entry (including padding) */
    uint8_t  name_len;          /* Length of the filename (not null-terminated on disk) */
    uint8_t  file_type;         /* File type indicator (if INCOMPAT_FILETYPE feature set) */
    char     name[EXT2_MAX_NAME_LEN]; /* Filename (NOT null-terminated on disk) */
} ext2_dir_entry_t;

/** file_type values in directory entries */
#define EXT2_FT_UNKNOWN     0   /* Unknown type */
#define EXT2_FT_REG_FILE    1   /* Regular file */
#define EXT2_FT_DIR         2   /* Directory */
#define EXT2_FT_CHRDEV      3   /* Character device */
#define EXT2_FT_BLKDEV      4   /* Block device */
#define EXT2_FT_FIFO        5   /* FIFO / named pipe */
#define EXT2_FT_SOCK        6   /* Socket */
#define EXT2_FT_SYMLINK     7   /* Symbolic link */


/* =========================================================================
 * OPAQUE HANDLE TYPES
 * ========================================================================= */
#define EXT2_MAX_BLOCK_SIZE     4096
#define EXT2_SECTOR_SIZE        512


struct ext2_fs {
    /* --- ATA backing device --- */
    ata_drive_t     *drive;

    /* --- Superblock --- */
    ext2_superblock_t superblock;
    uint8_t           sb_dirty;

    /* --- Derived geometry --- */
    uint32_t          block_size;
    uint32_t          sectors_per_block;
    uint32_t          num_groups;
    uint32_t          inodes_per_block;
    uint32_t          ptrs_per_block;

    /* --- Block Group Descriptor Table --- */
    ext2_block_group_descriptor_t *bgdt;
    uint8_t                        bgdt_dirty;

    /* --- Mount state --- */
    uint32_t  mount_flags;
    uint8_t   read_only;
    uint32_t  last_error;
};


struct ext2_file {
    /* --- Filesystem and identity --- */
    ext2_fs_t   *fs;
    uint32_t     ino;
    ext2_inode_t inode;
    uint8_t      inode_dirty;

    /* --- File position --- */
    uint64_t     pos;
    uint64_t     size;

    /* --- Open flags --- */
    uint32_t     flags;

    /* --- Block I/O buffer --- */
    uint8_t      block_buf[EXT2_MAX_BLOCK_SIZE];
    uint32_t     buf_block_no;
    uint8_t      buf_valid;
    uint8_t      buf_dirty;
};


struct ext2_dir {
    /* --- Filesystem and identity --- */
    ext2_fs_t   *fs;
    uint32_t     ino;
    ext2_inode_t inode;
    uint64_t     dir_size;

    /* --- Iteration state --- */
    uint32_t     logical_block_idx;
    uint32_t     byte_offset;

    /* --- Block buffer --- */
    uint8_t      block_buf[EXT2_MAX_BLOCK_SIZE];
    uint32_t     buf_block_no;
    uint8_t      buf_valid;
};

/** Represents a mounted Ext2 filesystem. Obtained via ext2_mount(). */
typedef struct ext2_fs ext2_fs_t;

/** Represents an open file within a mounted filesystem. */
typedef struct ext2_file ext2_file_t;

/** Represents an open directory for iteration. */
typedef struct ext2_dir ext2_dir_t;

/**
 * Stat-like structure populated by ext2_stat() and ext2_fstat().
 * Mirrors the information stored in an inode.
 */
typedef struct ext2_stat {
    uint32_t st_ino;        /* Inode number */
    uint16_t st_mode;       /* File type and permissions (i_mode) */
    uint16_t st_uid;        /* Owner user ID */
    uint16_t st_gid;        /* Owner group ID */
    uint16_t st_nlink;      /* Number of hard links */
    uint64_t st_size;       /* File size in bytes (64-bit) */
    uint32_t st_blocks;     /* Number of 512-byte blocks allocated */
    uint32_t st_atime;      /* Last access time (Unix timestamp) */
    uint32_t st_ctime;      /* Creation / metadata change time */
    uint32_t st_mtime;      /* Last modification time */
    uint32_t st_dtime;      /* Deletion time (0 if not deleted) */
    uint32_t st_flags;      /* Inode flags */
} ext2_stat_t;

/**
 * Directory entry as returned by ext2_dir_read().
 * The name is null-terminated for convenience.
 */
typedef struct ext2_dirent {
    uint32_t d_ino;                     /* Inode number */
    uint8_t  d_type;                    /* File type (EXT2_FT_* constant) */
    uint16_t d_name_len;                /* Length of filename */
    char     d_name[EXT2_MAX_NAME_LEN + 1]; /* Null-terminated filename */
} ext2_dirent_t;


/* =========================================================================
 * MOUNT FLAGS
 * ========================================================================= */

#define EXT2_MOUNT_RO           0x0001  /* Mount read-only */
#define EXT2_MOUNT_RW           0x0002  /* Mount read-write */
#define EXT2_MOUNT_NOATIME      0x0004  /* Do not update access times */
#define EXT2_MOUNT_SYNC         0x0008  /* Synchronous writes */
#define EXT2_MOUNT_FORCE        0x0010  /* Mount even if filesystem has errors */


/* =========================================================================
 * FILE OPEN FLAGS
 * ========================================================================= */

#define EXT2_O_RDONLY   0x0001  /* Open for reading only */
#define EXT2_O_WRONLY   0x0002  /* Open for writing only */
#define EXT2_O_RDWR     0x0003  /* Open for reading and writing */
#define EXT2_O_CREAT    0x0100  /* Create file if it does not exist */
#define EXT2_O_EXCL     0x0200  /* Fail if file exists (with O_CREAT) */
#define EXT2_O_TRUNC    0x0400  /* Truncate file to zero length on open */
#define EXT2_O_APPEND   0x0800  /* Writes always append to end of file */


/* =========================================================================
 * SEEK ORIGINS (for ext2_file_seek)
 * ========================================================================= */

#define EXT2_SEEK_SET   0   /* Seek from beginning of file */
#define EXT2_SEEK_CUR   1   /* Seek from current position */
#define EXT2_SEEK_END   2   /* Seek from end of file */


/* =========================================================================
 * FILESYSTEM LIFECYCLE
 * ========================================================================= */

/**
 * ext2_mount - Mount an Ext2 filesystem from a block device or image file.
 *
 * @param device    Path to the block device (e.g. "/dev/sda1") or image file.
 * @param flags     Mount flags: EXT2_MOUNT_RO or EXT2_MOUNT_RW, optionally OR'd
 *                  with EXT2_MOUNT_NOATIME, EXT2_MOUNT_SYNC, EXT2_MOUNT_FORCE.
 * @return          Pointer to an ext2_fs_t handle on success, or NULL on failure.
 *                  Call ext2_last_error() to retrieve the error code.
 *
 * Example:
 *   ext2_fs_t *fs = ext2_mount("/dev/sda1", EXT2_MOUNT_RW);
 *   if (!fs) { fprintf(stderr, "%s\n", ext2_strerror(ext2_last_error())); }
 */
ext2_fs_t *ext2_mount(const char *device, uint32_t flags);

/**
 * ext2_unmount - Flush all pending writes and unmount the filesystem.
 *
 * @param fs    Filesystem handle returned by ext2_mount().
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 *
 * After this call the handle is invalid and must not be used.
 */
ext2_error_t ext2_unmount(ext2_fs_t *fs);

/**
 * ext2_remount - Change the mount flags of an already-mounted filesystem.
 *
 * Useful for switching between read-only and read-write without unmounting.
 *
 * @param fs    Filesystem handle.
 * @param flags New mount flags (e.g. EXT2_MOUNT_RO to remount read-only).
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_remount(ext2_fs_t *fs, uint32_t flags);

/**
 * ext2_flush - Flush all dirty in-memory data to the underlying device.
 *
 * @param fs    Filesystem handle.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_flush(ext2_fs_t *fs);

/**
 * ext2_last_error - Retrieve the last error code set by any ext2 function.
 *
 * Thread-local: each thread has its own last-error value.
 *
 * @return  The most recent ext2_error_t value for the calling thread.
 */
ext2_error_t ext2_last_error(void);

/**
 * ext2_get_superblock - Retrieve a read-only copy of the superblock.
 *
 * @param fs    Filesystem handle.
 * @param sb    Pointer to caller-allocated ext2_superblock_t to fill.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_get_superblock(ext2_fs_t *fs, ext2_superblock_t *sb);

/**
 * ext2_get_block_group_descriptor - Retrieve a block group descriptor by index.
 *
 * @param fs        Filesystem handle.
 * @param group_idx Zero-based block group index.
 * @param bgd       Pointer to caller-allocated descriptor struct to fill.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_get_block_group_descriptor(ext2_fs_t *fs,
                                              uint32_t group_idx,
                                              ext2_block_group_descriptor_t *bgd);

/**
 * ext2_block_size - Return the block size of the mounted filesystem in bytes.
 *
 * @param fs    Filesystem handle.
 * @return      Block size in bytes (e.g. 1024, 2048, 4096).
 */
uint32_t ext2_block_size(ext2_fs_t *fs);

/**
 * ext2_block_group_count - Return the total number of block groups.
 *
 * @param fs    Filesystem handle.
 * @return      Number of block groups.
 */
uint32_t ext2_block_group_count(ext2_fs_t *fs);


/* =========================================================================
 * FILE OPERATIONS
 * ========================================================================= */

/**
 * ext2_file_open - Open or create a file by absolute path.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to the file (e.g. "/home/user/file.txt").
 * @param flags Combination of EXT2_O_* flags.
 * @param mode  Permission bits (i_mode) used when creating a new file
 *              (e.g. EXT2_S_IRUSR | EXT2_S_IWUSR). Ignored if not creating.
 * @return      Pointer to ext2_file_t handle on success, or NULL on failure.
 *
 * Example:
 *   ext2_file_t *f = ext2_file_open(fs, "/data.bin",
 *                                   EXT2_O_CREAT | EXT2_O_RDWR,
 *                                   EXT2_S_IRUSR | EXT2_S_IWUSR);
 */
ext2_file_t *ext2_file_open(ext2_fs_t *fs, const char *path,
                             uint32_t flags, uint16_t mode);

/**
 * ext2_file_open_inode - Open a file directly by inode number.
 *
 * @param fs        Filesystem handle.
 * @param ino       Inode number.
 * @param flags     EXT2_O_RDONLY, EXT2_O_WRONLY, or EXT2_O_RDWR.
 * @return          File handle on success, NULL on failure.
 */
ext2_file_t *ext2_file_open_inode(ext2_fs_t *fs, uint32_t ino, uint32_t flags);

/**
 * ext2_file_close - Close a file handle and release associated resources.
 *
 * @param file  File handle to close.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_file_close(ext2_file_t *file);

/**
 * ext2_file_read - Read bytes from the current file position.
 *
 * @param file      File handle.
 * @param buf       Destination buffer.
 * @param count     Maximum number of bytes to read.
 * @param bytes_read  Out: actual number of bytes read (may be less than count
 *                  at end-of-file). May be NULL if the caller does not need it.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_file_read(ext2_file_t *file, void *buf, size_t count,
                             size_t *bytes_read);

/**
 * ext2_file_write - Write bytes at the current file position.
 *
 * Extends the file if writing past the current end.
 *
 * @param file          File handle (must have been opened with write access).
 * @param buf           Source buffer.
 * @param count         Number of bytes to write.
 * @param bytes_written Out: actual number of bytes written. May be NULL.
 * @return              EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_file_write(ext2_file_t *file, const void *buf, size_t count,
                              size_t *bytes_written);

/**
 * ext2_file_seek - Reposition the file's read/write offset.
 *
 * @param file      File handle.
 * @param offset    Byte offset relative to whence.
 * @param whence    EXT2_SEEK_SET, EXT2_SEEK_CUR, or EXT2_SEEK_END.
 * @param new_pos   Out: resulting absolute offset. May be NULL.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_file_seek(ext2_file_t *file, int64_t offset, int whence,
                             uint64_t *new_pos);

/**
 * ext2_file_tell - Return the current read/write offset.
 *
 * @param file  File handle.
 * @return      Current byte offset from start of file.
 */
uint64_t ext2_file_tell(ext2_file_t *file);

/**
 * ext2_file_truncate - Resize a file to the specified length.
 *
 * If the new size is smaller, data beyond it is discarded and blocks freed.
 * If larger, the file is extended with zero-filled blocks.
 *
 * @param file      File handle (must have write access).
 * @param new_size  Desired file size in bytes.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_file_truncate(ext2_file_t *file, uint64_t new_size);

/**
 * ext2_file_sync - Flush a specific file's dirty data to the device.
 *
 * @param file  File handle.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_file_sync(ext2_file_t *file);


/* =========================================================================
 * FILE METADATA & LINKS
 * ========================================================================= */

/**
 * ext2_stat - Retrieve metadata for a file specified by path.
 *
 * Follows symbolic links (use ext2_lstat() to stat the link itself).
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to the file.
 * @param st    Pointer to ext2_stat_t to populate.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_stat(ext2_fs_t *fs, const char *path, ext2_stat_t *st);

/**
 * ext2_lstat - Like ext2_stat() but does NOT follow symbolic links.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path.
 * @param st    Pointer to ext2_stat_t to populate.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_lstat(ext2_fs_t *fs, const char *path, ext2_stat_t *st);

/**
 * ext2_fstat - Retrieve metadata for an already-open file handle.
 *
 * @param file  Open file handle.
 * @param st    Pointer to ext2_stat_t to populate.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_fstat(ext2_file_t *file, ext2_stat_t *st);

/**
 * ext2_chmod - Change the permission bits of a file.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to the file.
 * @param mode  New permission bits (lower 12 bits of i_mode).
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_chmod(ext2_fs_t *fs, const char *path, uint16_t mode);

/**
 * ext2_chown - Change the owner and/or group of a file.
 *
 * Pass (uint16_t)-1 to leave uid or gid unchanged.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to the file.
 * @param uid   New owner UID, or (uint16_t)-1 to leave unchanged.
 * @param gid   New group GID, or (uint16_t)-1 to leave unchanged.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_chown(ext2_fs_t *fs, const char *path,
                         uint16_t uid, uint16_t gid);

/**
 * ext2_utime - Update the access and modification timestamps of a file.
 *
 * Pass 0 for either timestamp to set it to the current time.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to the file.
 * @param atime New access time (Unix timestamp), or 0 for now.
 * @param mtime New modification time (Unix timestamp), or 0 for now.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_utime(ext2_fs_t *fs, const char *path,
                         uint32_t atime, uint32_t mtime);

/**
 * ext2_link - Create a hard link to an existing file.
 *
 * @param fs        Filesystem handle.
 * @param oldpath   Path to the existing file.
 * @param newpath   Path for the new hard link.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_link(ext2_fs_t *fs, const char *oldpath, const char *newpath);

/**
 * ext2_unlink - Remove a directory entry (decrement link count, free inode
 *               and blocks when link count reaches zero).
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to the file to unlink.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_unlink(ext2_fs_t *fs, const char *path);

/**
 * ext2_rename - Move or rename a file or directory.
 *
 * @param fs        Filesystem handle.
 * @param oldpath   Current path.
 * @param newpath   New path. If newpath exists and is a file, it is replaced.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_rename(ext2_fs_t *fs, const char *oldpath, const char *newpath);

/**
 * ext2_symlink - Create a symbolic link.
 *
 * @param fs        Filesystem handle.
 * @param target    The string the symlink will contain (the link target).
 * @param linkpath  Absolute path where the symlink will be created.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_symlink(ext2_fs_t *fs, const char *target, const char *linkpath);

/**
 * ext2_readlink - Read the target of a symbolic link.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to the symlink.
 * @param buf   Buffer to receive the null-terminated target string.
 * @param bufsz Size of buf in bytes.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_readlink(ext2_fs_t *fs, const char *path,
                            char *buf, size_t bufsz);


/* =========================================================================
 * DIRECTORY OPERATIONS
 * ========================================================================= */

/**
 * ext2_mkdir - Create a new directory.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path for the new directory.
 * @param mode  Permission bits (e.g. EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IXUSR).
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_mkdir(ext2_fs_t *fs, const char *path, uint16_t mode);

/**
 * ext2_mkdir_p - Create a directory and all missing parent directories.
 *
 * Equivalent to `mkdir -p`. Silently succeeds if the directory already exists.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path for the new directory tree.
 * @param mode  Permission bits applied to each newly created directory.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_mkdir_p(ext2_fs_t *fs, const char *path, uint16_t mode);

/**
 * ext2_rmdir - Remove an empty directory.
 *
 * Fails with EXT2_ERR_NOT_EMPTY if the directory contains any entries
 * other than "." and "..".
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to the directory to remove.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_rmdir(ext2_fs_t *fs, const char *path);

/**
 * ext2_rmdir_r - Recursively remove a directory and all its contents.
 *
 * Equivalent to `rm -rf`. Use with caution.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to the directory to remove.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_rmdir_r(ext2_fs_t *fs, const char *path);

/**
 * ext2_dir_open - Open a directory for iteration.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to the directory.
 * @return      Pointer to ext2_dir_t handle on success, NULL on failure.
 */
ext2_dir_t *ext2_dir_open(ext2_fs_t *fs, const char *path);

/**
 * ext2_dir_open_inode - Open a directory for iteration by inode number.
 *
 * @param fs    Filesystem handle.
 * @param ino   Directory inode number.
 * @return      Pointer to ext2_dir_t handle on success, NULL on failure.
 */
ext2_dir_t *ext2_dir_open_inode(ext2_fs_t *fs, uint32_t ino);

/**
 * ext2_dir_read - Read the next entry from an open directory.
 *
 * @param dir   Directory handle from ext2_dir_open().
 * @param entry Pointer to caller-allocated ext2_dirent_t to populate.
 * @return      EXT2_OK and populates entry if an entry was read;
 *              EXT2_ERR_NOT_FOUND when there are no more entries;
 *              or another negative ext2_error_t on I/O failure.
 */
ext2_error_t ext2_dir_read(ext2_dir_t *dir, ext2_dirent_t *entry);

/**
 * ext2_dir_rewind - Reset the directory iterator to the first entry.
 *
 * @param dir   Directory handle.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_dir_rewind(ext2_dir_t *dir);

/**
 * ext2_dir_close - Close a directory handle and free associated resources.
 *
 * @param dir   Directory handle to close.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_dir_close(ext2_dir_t *dir);


/* =========================================================================
 * INODE-LEVEL ACCESS
 * ========================================================================= */

/**
 * ext2_inode_read - Read an inode from disk by inode number.
 *
 * @param fs    Filesystem handle.
 * @param ino   Inode number (1-based).
 * @param inode Pointer to caller-allocated ext2_inode_t to populate.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_inode_read(ext2_fs_t *fs, uint32_t ino, ext2_inode_t *inode);

/**
 * ext2_inode_write - Write a (modified) inode back to disk.
 *
 * @param fs    Filesystem handle.
 * @param ino   Inode number.
 * @param inode Pointer to the inode data to write.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_inode_write(ext2_fs_t *fs, uint32_t ino,
                               const ext2_inode_t *inode);

/**
 * ext2_inode_alloc - Allocate a new inode from the filesystem.
 *
 * @param fs        Filesystem handle.
 * @param preferred_group  Preferred block group to allocate from (hint only).
 * @param ino_out   Out: allocated inode number.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_inode_alloc(ext2_fs_t *fs, uint32_t preferred_group,
                               uint32_t *ino_out);

/**
 * ext2_inode_free - Mark an inode as free in its block group bitmap.
 *
 * Does NOT free data blocks; use ext2_inode_free_blocks() first.
 *
 * @param fs    Filesystem handle.
 * @param ino   Inode number to free.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_inode_free(ext2_fs_t *fs, uint32_t ino);

/**
 * ext2_inode_free_blocks - Free all data blocks associated with an inode.
 *
 * Resolves direct, singly indirect, doubly indirect, and triply indirect
 * block pointers and marks each block as free.
 *
 * @param fs    Filesystem handle.
 * @param inode Pointer to the inode whose blocks should be freed.
 * @return      EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_inode_free_blocks(ext2_fs_t *fs, ext2_inode_t *inode);

/**
 * ext2_inode_resolve_block - Resolve a logical block index within a file to
 *                            a physical block number on disk.
 *
 * @param fs            Filesystem handle.
 * @param inode         Pointer to the inode.
 * @param logical_block Zero-based logical block index within the file.
 * @param phys_block    Out: physical block number on disk.
 * @return              EXT2_OK on success, EXT2_ERR_NOT_FOUND if the block is
 *                      a sparse hole, or another error code on failure.
 */
ext2_error_t ext2_inode_resolve_block(ext2_fs_t *fs, const ext2_inode_t *inode,
                                       uint32_t logical_block,
                                       uint32_t *phys_block);


/* =========================================================================
 * BLOCK-LEVEL ACCESS
 * ========================================================================= */

/**
 * ext2_block_read - Read a raw block from the filesystem by block number.
 *
 * @param fs        Filesystem handle.
 * @param block_no  Absolute block number to read.
 * @param buf       Caller-allocated buffer of at least ext2_block_size() bytes.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_block_read(ext2_fs_t *fs, uint32_t block_no, void *buf);

/**
 * ext2_block_write - Write a raw block to the filesystem by block number.
 *
 * @param fs        Filesystem handle.
 * @param block_no  Absolute block number to write.
 * @param buf       Source buffer of at least ext2_block_size() bytes.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_block_write(ext2_fs_t *fs, uint32_t block_no, const void *buf);

/**
 * ext2_block_alloc - Allocate a new block from the filesystem.
 *
 * @param fs            Filesystem handle.
 * @param preferred_group  Preferred block group (hint).
 * @param block_out     Out: allocated block number.
 * @return              EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_block_alloc(ext2_fs_t *fs, uint32_t preferred_group,
                               uint32_t *block_out);

/**
 * ext2_block_free - Mark a block as free in its block group bitmap.
 *
 * @param fs        Filesystem handle.
 * @param block_no  Block number to free.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_block_free(ext2_fs_t *fs, uint32_t block_no);


/* =========================================================================
 * FILESYSTEM INFORMATION & UTILITIES
 * ========================================================================= */

/**
 * ext2_statvfs - Retrieve filesystem-wide usage statistics.
 *
 * @param fs    Filesystem handle.
 * @param total_blocks  Out: total number of blocks.
 * @param free_blocks   Out: number of free blocks.
 * @param total_inodes  Out: total number of inodes.
 * @param free_inodes   Out: number of free inodes.
 * @return EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_statvfs(ext2_fs_t *fs,
                           uint32_t *total_blocks, uint32_t *free_blocks,
                           uint32_t *total_inodes, uint32_t *free_inodes);

/**
 * ext2_lookup - Resolve an absolute path to its inode number.
 *
 * Follows symbolic links up to a maximum depth of 8.
 *
 * @param fs        Filesystem handle.
 * @param path      Absolute path to look up.
 * @param ino_out   Out: inode number of the resolved path.
 * @return          EXT2_OK on success, or a negative ext2_error_t on failure.
 */
ext2_error_t ext2_lookup(ext2_fs_t *fs, const char *path, uint32_t *ino_out);

/**
 * ext2_path_exists - Test whether a path exists in the filesystem.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to test.
 * @return      1 if the path exists, 0 if it does not,
 *              or a negative ext2_error_t on I/O failure.
 */
int ext2_path_exists(ext2_fs_t *fs, const char *path);

/**
 * ext2_is_dir - Test whether a path refers to a directory.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to test.
 * @return      1 if it is a directory, 0 if not,
 *              or a negative ext2_error_t on failure.
 */
int ext2_is_dir(ext2_fs_t *fs, const char *path);

/**
 * ext2_is_file - Test whether a path refers to a regular file.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to test.
 * @return      1 if it is a regular file, 0 if not,
 *              or a negative ext2_error_t on failure.
 */
int ext2_is_file(ext2_fs_t *fs, const char *path);

/**
 * ext2_is_symlink - Test whether a path refers to a symbolic link.
 *
 * @param fs    Filesystem handle.
 * @param path  Absolute path to test (not followed).
 * @return      1 if it is a symbolic link, 0 if not,
 *              or a negative ext2_error_t on failure.
 */
int ext2_is_symlink(ext2_fs_t *fs, const char *path);

/**
 * ext2_inode_get_size - Return the full 64-bit size of a file from its inode.
 *
 * Combines i_size (lower 32 bits) and i_dir_acl (upper 32 bits) for regular
 * files on filesystems with EXT2_FEATURE_RO_COMPAT_LARGE_FILE.
 *
 * @param fs    Filesystem handle.
 * @param inode Pointer to the inode.
 * @return      File size in bytes.
 */
uint64_t ext2_inode_get_size(ext2_fs_t *fs, const ext2_inode_t *inode);


/* =========================================================================
 * HELPER MACROS
 * ========================================================================= */

/** True if the inode represents a regular file. */
#define EXT2_INODE_IS_REG(inode)    (((inode)->i_mode & 0xF000) == EXT2_S_IFREG)

/** True if the inode represents a directory. */
#define EXT2_INODE_IS_DIR(inode)    (((inode)->i_mode & 0xF000) == EXT2_S_IFDIR)

/** True if the inode represents a symbolic link. */
#define EXT2_INODE_IS_LNK(inode)    (((inode)->i_mode & 0xF000) == EXT2_S_IFLNK)

/** True if the inode represents a block device. */
#define EXT2_INODE_IS_BLK(inode)    (((inode)->i_mode & 0xF000) == EXT2_S_IFBLK)

/** True if the inode represents a character device. */
#define EXT2_INODE_IS_CHR(inode)    (((inode)->i_mode & 0xF000) == EXT2_S_IFCHR)

/** True if the inode represents a FIFO. */
#define EXT2_INODE_IS_FIFO(inode)   (((inode)->i_mode & 0xF000) == EXT2_S_IFIFO)

/** Compute block size in bytes from a superblock. */
#define EXT2_BLOCK_SIZE(sb)         (1024 << (sb)->s_log_block_size)

/** Compute the block group that contains a given inode number. */
#define EXT2_INODE_GROUP(sb, ino)   (((ino) - 1) / (sb)->s_inodes_per_group)

/** Compute the local index of an inode within its block group. */
#define EXT2_INODE_INDEX(sb, ino)   (((ino) - 1) % (sb)->s_inodes_per_group)

/** Compute the block group that contains a given block number. */
#define EXT2_BLOCK_GROUP(sb, blk)   (((blk) - (sb)->s_first_data_block) \
                                     / (sb)->s_blocks_per_group)

/** Compute number of block groups from a superblock. */
#define EXT2_NUM_GROUPS(sb)         (((sb)->s_blocks_count \
                                      - (sb)->s_first_data_block \
                                      + (sb)->s_blocks_per_group - 1) \
                                     / (sb)->s_blocks_per_group)



#endif /* EXT2_DRIVER_H */