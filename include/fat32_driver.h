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

// =======================================================
//                  FAT32 Boot Parameter Block (BPB)
// =======================================================

/**
 * @struct fat32_bpb_struct
 * @brief  Represents the FAT32 Boot Parameter Block located in the boot sector (LBA 0 of the partition).
 *         This block defines layout information of the filesystem on disk.
 */
typedef struct __attribute__((packed)) fat32_bpb_struct {
    uint8_t unrelated[3+8];                // stuff for bootloader or something (boot record)
    uint16_t bytes_per_sector;             // The number of Bytes per sector
    uint8_t sector_per_cluster;            // Number of sectors per cluster
    uint16_t reserved_sectors;             // Number of reserved sectors (The boot record sectors are included in this value)
    uint8_t fat_count;                     // Number of File Allocation Tables (FAT's) on the storage media
    uint16_t root_directory_entries_count; // 	Number of root directory entries (must be set so that the root directory occupies entire sectors, FAT12/16 not 32).
    uint16_t total_sectors_count;          // The total sectors in the logical volume. If this value is 0, it means there are more than 65535 sectors in the volume, and the actual count is stored in the Large Sector Count entry at 0x20
    uint8_t media_descriptor_type;         // media descriptor type (https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#BPB20_OFS_0Ah)
    uint16_t sectors_per_fat;              // Number of sectors per FAT. FAT12/FAT16 only.
    uint16_t sectors_per_track;            // Number of sectors per track.
    uint16_t heads_on_storage_media_count; // Number of heads or sides on the storage media.
    uint32_t hidden_sectors_count;         // Number of hidden sectors. (i.e. the LBA of the beginning of the partition.)
    uint32_t large_sector_count;           // Large sector count. This field is set if there are more than 65535 sectors in the volume, resulting in a value which does not fit in the Number of Sectors entry at 0x13.

    // FAT32-only fields
    uint32_t sectors_per_fat_32; // Sectors per FAT. The size of the FAT in sectors.
    uint16_t ext_flags;          // Flags.
    uint16_t fs_version;         // FAT version number. The high byte is the major version and the low byte is the minor version. FAT drivers should respect this field.
    uint32_t root_cluster;       // The cluster number of the root directory. Often this field is set to 2.
    uint16_t fs_info;            // The sector number of the FSInfo structure.
    uint16_t backup_boot_sector; // The sector number of the backup boot sector.
    uint8_t reserved[12];        // Reserved. When the volume is formated these bytes should be zero.
    uint8_t unused_for_us[448];  // unused for us
} fat32_bpb_t;

// =======================================================
//                  FAT32 FSInfo Structure
// =======================================================

/**
 * @struct fat32_fsinfo_struct
 * @brief  Contains volume runtime hints like "free cluster count" and "next free cluster index".
 *         Located in a sector defined by BPB.fs_info_sector
 */
typedef struct __attribute__((packed)) fat32_fsinfo_struct {
    uint32_t lead_signature;       // Must be FAT32_FSINFO_SIGNATURE1 (0x41615252)
    uint8_t  reserved1[480];       // Reserved bytes, should be ignored by driver.
    uint32_t struct_signature;     // Must be FAT32_FSINFO_SIGNATURE2 (0x61417272)
    uint32_t free_cluster_count;   // Last known count of free clusters, or 0xFFFFFFFF if unknown.
    uint32_t next_free_cluster;    // Cluster index to start searching for free clusters, or 0xFFFFFFFF if unknown.
    uint8_t  reserved2[12];        // Reserved, should be ignored by driver.
    uint32_t trail_signature;      // Must be FAT32_FSINFO_SIGNATURE3 (0xAA550000)
} fat32_fsinfo_t;

typedef struct __attribute__((packed)) fat32_directory_entry_struct {
    char     name[11];       // The name of the directory in a 8.3 format
    uint8_t  attr;           // The atribute of the directory
    uint8_t  windows_nt;     // Ceserved for windows nt 
    uint8_t  create_time;    // Creation time in hundredths of a second
    uint32_t first_cluster;  // the first cluster of the directory (e.g. the current cluster)
    uint32_t file_size;      // the size of the directory in with the header
} fat32_directory_entry_t;

// =======================================================
//                  Standard 8.3 Directory Entry
// =======================================================

/**
 * @struct fat32_directory_entry_struct
 * @brief  Represents a classic FAT directory entry (32 bytes) for a file or subdirectory.
 */
typedef struct __attribute__((packed)) fat32_directory_entry_struct {
    char     name[11];              // File name in 8.3 format, padded with spaces, no NULL terminator.
    uint8_t  attr;                  // Attribute flags (see FAT32_ATTR_* macros).
    uint8_t  nt_reserved;           // Reserved for Windows NT compatibility, keep but don't rely on it.
    uint8_t  created_time_tenths;   // Sub-second precision for creation time, 0-199 valid.
    uint16_t created_time;          // File creation time (FAT time format).
    uint16_t created_date;          // File creation date (FAT date format).
    uint16_t accessed_date;         // Last access date (no time stored for this).
    uint16_t first_cluster_high;    // Upper 16 bits of first cluster index.
    uint16_t modified_time;         // Last modified time (FAT time format).
    uint16_t modified_date;         // Last modified date (FAT date format).
    uint16_t first_cluster_low;     // Lower 16 bits of first cluster index.
    uint32_t file_size;             // File size in bytes. 0 for directories.
} fat32_dir_entry_t;

// =======================================================
//                  Long File Name Entry (LFN)
// =======================================================

/**
 * @struct fat32_lfn_entry_struct
 * @brief  Special directory entry used for long file names (13 UTF-16 characters per entry).
 */
typedef struct __attribute__((packed)) fat32_lfn_entry_struct {
    uint8_t  order;             // Index order of LFN entry sequence.
    uint16_t name_part1[5];     // First 5 UTF-16 characters of the name.
    uint8_t  attr;              // Always 0x0F for LFN entries.
    uint8_t  type;              // Entry type, usually 0x00.
    uint8_t  checksum;          // Checksum of the associated short 8.3 name entry.
    uint16_t name_part2[6];     // Next 6 UTF-16 characters.
    uint16_t first_cluster_low; // Always 0x0000 in LFN entries.
    uint16_t name_part3[2];     // Final 2 UTF-16 characters.
} fat32_lfn_entry_t;

// =======================================================
//                  Mounted Volume State
// =======================================================

/**
 * @struct fat32_volume
 * @brief  Stores computed runtime layout values after mounting (not stored directly on disk).
 */
typedef struct fat32_volume {
    fat32_bpb_t  bpb;              // BPB structure read from the volume boot sector.
    fat32_fsinfo_t fsinfo;         // FSInfo sector info read during mount.

    uint32_t fat_size_sectors;     // Cached size of FAT region in sectors (bpb.sectors_per_fat_32).
    uint32_t cluster_size_bytes;   // FAT32 cluster size in bytes (bytes_per_sector * sectors_per_cluster).
    uint32_t first_data_sector;    // First sector where actual cluster data begins.
    uint32_t total_clusters;       // Computed cluster count on volume.

    ata_drive_t disk_private;      // disk context
} fat32_volume_t;

/**
 * @brief  Mounts a FAT32 volume and computes layout values.
 * @param  vol Pointer to volume state object to fill during mount.
 * @return 0 on success, negative value on failure (invalid signature, read error, etc).
 */
int fat32_mount(fat32_volume_t *vol);

/**
 * @brief  Unmounts the FAT32 volume (flushes caches if implemented).
 * @param  vol Pointer to an already-mounted volume object.
 */
void fat32_unmount(fat32_volume_t *vol);

/**
 * @brief  Reads a FAT entry corresponding to a cluster.
 * @param  vol     Mounted volume object.
 * @param  cluster Cluster index to look up in the FAT table.
 * @return FAT table value stored for that cluster (next cluster index or status flag).
 */
uint32_t fat32_read_fat_entry(fat32_volume_t *vol, uint32_t cluster);

/**
 * @brief  Writes a FAT entry for the specified cluster.
 * @param  vol     Mounted volume object.
 * @param  cluster Cluster index to modify.
 * @param  value   New FAT value (next cluster or FAT32_END_OF_CHAIN, etc).
 * @return 0 on success, negative on disk write failure.
 */
int fat32_write_fat_entry(fat32_volume_t *vol, uint32_t cluster, uint32_t value);

/**
 * @brief  Finds a free unused cluster by scanning the FAT table.
 * @param  vol         Mounted volume state.
 * @param  out_cluster Pointer to receive the allocated cluster index.
 * @return 0 on success, negative on failure (no space, disk read error, etc).
 */
int fat32_find_free_cluster(fat32_volume_t *vol, uint32_t *out_cluster);

/**
 * @brief  Lists directory contents at the specified path.
 * @param  vol   Mounted volume.
 * @param  path  Directory path string (absolute, e.g. "/FOLDER1/FOLDER2").
 * @return 0 on success, negative on failure (not found, not a directory, disk error).
 */
int fat32_ls(fat32_volume_t *vol, const char *path);

/**
 * @brief  Creates a new directory at the specified path.
 * @param  vol   Mounted volume.
 * @param  path  Directory path to create.
 * @return 0 on success, negative on failure (exists, no space, disk write error, etc).
 */
int fat32_mkdir(fat32_volume_t *vol, const char *path);

/**
 * @brief  Removes a directory (must be empty first).
 * @param  vol Mounted volume.
 * @param  path Directory path to remove.
 * @return 0 on success, negative on failure (not empty, not found, disk error).
 */
int fat32_rmdir(fat32_volume_t *vol, const char *path);

/**
 * @brief  Creates an empty file entry in the parent directory and allocates first cluster.
 * @param  vol   Mounted volume.
 * @param  path  File path to create.
 * @return 0 on success, negative on failure (exists, no space, write error, etc).
 */
int fat32_create_file(fat32_volume_t *vol, const char *path);

/**
 * @brief  Deletes a file and frees its clusters in the FAT table.
 * @param  vol  Mounted volume.
 * @param  path File path to delete.
 * @return 0 on success, negative on failure (not found, disk error).
 */
int fat32_delete_file(fat32_volume_t *vol, const char *path);

/**
 * @brief  Reads file contents into memory.
 * @param  vol  Mounted volume.
 * @param  path File path to read.
 * @param  buffer Memory buffer to load file into.
 * @param  bytes_to_read Max bytes to read.
 * @param  out_read Bytes actually read.
 * @return 0 on success, negative on failure.
 */
int fat32_read_file(fat32_volume_t *vol, const char *path, void *buffer, size_t bytes_to_read, size_t *out_read);

/**
 * @brief  Writes file contents from memory, allocating clusters as needed.
 * @param  vol  Mounted volume.
 * @param  path File path to write.
 * @param  buffer Memory buffer containing data to write.
 * @param  bytes_to_write Byte amount to write.
 * @param  out_written Bytes actually written.
 * @return 0 on success, negative on failure.
 */
int fat32_write_file(fat32_volume_t *vol, const char *path, const void *buffer, size_t bytes_to_write, size_t *out_written);

// =======================================================
//   Disk abstraction (implemented by you, not by FAT32)
// =======================================================

/**
 * @brief  Reads a raw sector from disk using the underlying storage driver.
 * @param  vol Mounted volume containing disk context.
 * @param  lba Sector number to read.
 * @param  buffer 512-byte buffer for sector data.
 * @return 0 on success, negative on read failure.
 */
int fat32_disk_read_sector(fat32_volume_t *vol, uint32_t lba, void *buffer);

/**
 * @brief  Writes a raw sector to disk using the underlying storage driver.
 * @param  vol Mounted volume containing disk context.
 * @param  lba Sector number to write.
 * @param  buffer 512-byte buffer with sector contents to write.
 * @return 0 on success, negative on write failure.
 */
int fat32_disk_write_sector(fat32_volume_t *vol, uint32_t lba, const void *buffer);

#endif // FAT32_H