#ifndef FAT32_H
#define FAT32_H

#include "types.h"
#include "ata_driver.h"

// FAT32 signature values
#define FAT32_BOOT_SIGNATURE      0x29
#define FAT32_FSINFO_SIGNATURE1   0x41615252
#define FAT32_FSINFO_SIGNATURE2   0x61417272
#define FAT32_FSINFO_SIGNATURE3   0xAA550000
#define FAT32_END_OF_CHAIN        0x0FFFFFF8
#define FAT32_BAD_CLUSTER         0x0FFFFFF7

// Attributes for directory entries
#define FAT32_ATTR_READ_ONLY  0x01
#define FAT32_ATTR_HIDDEN     0x02
#define FAT32_ATTR_SYSTEM     0x04
#define FAT32_ATTR_VOLUME_ID  0x08
#define FAT32_ATTR_DIRECTORY  0x10
#define FAT32_ATTR_ARCHIVE    0x20
#define FAT32_ATTR_LFN       (FAT32_ATTR_READ_ONLY | FAT32_ATTR_HIDDEN | FAT32_ATTR_SYSTEM | FAT32_ATTR_VOLUME_ID)

#define FAT32_SECTOR_SIZE 512
#define FAT32_CLUSTER_SIZE (FAT32_SECTOR_SIZE * 4)


typedef struct __attribute__((packed)) bpb_struct {
    uint8_t unrelated[3+8];  // stuff for bootloader or something (boot record)
    uint16_t bytes_per_sector; // The number of Bytes per sector
    uint8_t sector_per_cluster; // Number of sectors per cluster
    uint16_t reserved_sectors; // Number of reserved sectors (The boot record sectors are included in this value)
    uint8_t fat_count;  // Number of File Allocation Tables (FAT's) on the storage media
    uint16_t root_directory_entries_count; // 	Number of root directory entries (must be set so that the root directory occupies entire sectors, FAT12/16 not 32).
    uint16_t total_sectors_count; // The total sectors in the logical volume. If this value is 0, it means there are more than 65535 sectors in the volume, and the actual count is stored in the Large Sector Count entry at 0x20
    uint8_t media_descriptor_type; // media descriptor type (https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#BPB20_OFS_0Ah)
    uint16_t sectors_per_fat; // Number of sectors per FAT. FAT12/FAT16 only.
    uint16_t sectors_per_track; // Number of sectors per track.
    uint16_t heads_on_storage_media_count; // Number of heads or sides on the storage media.
    uint32_t hidden_sectors_count; // Number of hidden sectors. (i.e. the LBA of the beginning of the partition.)
    uint32_t large_sector_count; // Large sector count. This field is set if there are more than 65535 sectors in the volume, resulting in a value which does not fit in the Number of Sectors entry at 0x13.

    // FAT32-only fields
    uint32_t sectors_per_fat_32; // Sectors per FAT. The size of the FAT in sectors.
    uint16_t ext_flags;  // Flags.
    uint16_t fs_version;  // FAT version number. The high byte is the major version and the low byte is the minor version. FAT drivers should respect this field.
    uint32_t root_cluster; // The cluster number of the root directory. Often this field is set to 2.
    uint16_t fs_info;  // The sector number of the FSInfo structure.
    uint16_t backup_boot_sector; // The sector number of the backup boot sector.
    uint8_t reserved[12];  // Reserved. When the volume is formated these bytes should be zero.
    uint8_t unused_for_us[448]; // unused for us (our os)
} bpb_t;

typedef struct __attribute__((packed)) fat32_fsinfo_struct {
    uint32_t lead_signature;       // Lead signature (must be 0x41615252)
    uint8_t  reserved1[480];
    uint32_t struct_signature;     // Another signature (must be 0x61417272)
    uint32_t free_cluster_count;   // Contains the last known free cluster count on the volume. 0xFFFFFFFF = unknown
    uint32_t next_free_cluster;    // Indicates the cluster number at which the filesystem driver should start looking for available clusters. 0xFFFFFFFF = unknown
    uint8_t  reserved2[12];
    uint32_t trail_signature;      // Trail signature (must be 0xAA550000)
} fat32_fsinfo_t;

typedef struct __attribute__((packed)) fat32_directory_entry_struct {
    char     name[11];       // the name of the directory
    char     fext[4];        // the directory extention of entry is file
    uint8_t  attr;           // the atribute of the directory
    uint32_t first_cluster;  // the first cluster of the directory (e.g. the current cluster)
    uint32_t file_size;      // the size of the directory in with the header
} fat32_directory_entry_t;

/**
 * Initialize FAT32 handling on a given drive.
 *
 * Loads and validates the on-disk Boot Sector (BPB/EBPB) and the FSInfo sector,
 * then caches key filesystem parameters in memory for later cluster/sector math.
 *
 * @param drive  The drive identifier used.
 * @return       Status code:
 *               0 = success,
 *               1 = invalid FAT32 signature,
 *               2 = unsupported sector/cluster size,
 *               3 = disk read error.
 */
uint8_t fat32_init(ata_drive_t drive);


/**
 * Read a file from disk into a buffer.
 *
 * Resolves a filesystem path (e.g. "FOLDER/FILE.TXT"), walks directory clusters
 * to locate the file’s Directory Entry, then follows its FAT32 cluster chain
 * to read the file data sectors into memory.
 *
 * Stops when:
 *  ‣ The file ends (End-of-Cluster-Chain),
 *  ‣ OR max_bytes is reached,
 *  ‣ OR a disk I/O error occurs.
 *
 * @param path       Null-terminated file path using '/' or your OS path separator.
 * @param buffer     Target RAM buffer to fill with file data.
 * @param max_bytes  Maximum number of bytes to read into the buffer.
 * @return           Status code:
 *                   0 = success,
 *                   1 = file not found in path,
 *                   2 = invalid path format,
 *                   3 = bad cluster detected,
 *                   4 = disk read error,
 *                   5 = buffer too small for requested max_bytes.
 */
uint8_t fat32_read_file(const char* path, uint8_t* buffer, uint32_t max_bytes);


/**
 * List the contents of a directory (folder).
 *
 * Opens the directory at the given path, iterates its cluster chain, and prints
 * or logs the names (both short 8.3 and long LFN if present) of each valid entry.
 *
 * This function does not return entries, it only displays them to console/log.
 *
 * @param path   Directory path to list. Use "" or "/" for the root directory.
 */
void fat32_list_directory(const char* path);


/**
 * Open a directory and prepare for reading its entries sequentially.
 *
 * Resolves the given path and loads the directory’s starting cluster,
 * then walks its cluster chain as needed when fat32_read_directory_entry() is called.
 *
 * Maintains an internal "open directory handle" (file cursor + current cluster)
 * until fat32_close_directory() is called.
 *
 * @param path   Directory path (e.g. "SYS/CONFIG").
 * @return       1  = directory opened successfully,
 *               0 = directory not found or disk I/O error.
 */
uint8_t fat32_open_directory(const char* path);


/**
 * Read the next Directory Entry from the currently opened directory.
 *
 * Retrieves the next valid 32-byte entry from disk, skipping deleted or empty slots,
 * and fills the provided struct. Supports both files and sub-directories.
 *
 * Calling this function without opening a directory first results in undefined behavior.
 *
 * @param entry  Pointer to a Directory Entry struct to fill with file/folder data.
 * @return       1  = a valid entry was read into *entry,
 *               0 = end of directory reached or disk I/O error.
 */
uint8_t fat32_read_directory_entry(fat32_directory_entry_t* entry);


/**
 * Close the currently opened directory.
 *
 * Resets internal directory handle state (cluster cursor, sector index, LFN text buffer).
 * After calling this, fat32_read_directory_entry() must not be used again
 * until a new fat32_open_directory() call is made.
 */
void fat32_close_directory();


/**
 * Get the next cluster in a cluster chain from the FAT table.
 *
 * Reads the FAT32 cluster value stored for the given cluster index, which represents
 * the next linked cluster belonging to the same file or directory.
 *
 * This is the core hardware-independent linked-list traversal function for the filesystem.
 *
 * @param cluster  Current cluster index (≥ 2).
 * @return         The next cluster index in the chain, OR special values:
 *                 0x0FFFFFF8+ = end of chain,
 *                 0x0FFFFFF7   = bad cluster,
 *                 0            = cluster 0 is invalid (never valid in FAT32).
 */
uint32_t fat32_get_next_cluster(uint32_t cluster);

#endif // FAT32_H