#include "drivers/flatfs/flatfs_driver.h"
#include "utils.h"

flatfs_err_t flatfs_format(ata_drive_t *drive,
                           uint32_t total_blocks,
                           uint32_t total_inodes) {
    if (!drive)
        return FLATFS_ERR_INVALID;

    if (total_blocks + FLATFS_SECTOR_DATA_START > drive->size_in_sectors)
        total_blocks = drive->size_in_sectors - FLATFS_SECTOR_DATA_START;
    if (total_blocks > FLATFS_MAX_BLOCKS)
        total_blocks = FLATFS_MAX_BLOCKS;

    if (total_inodes > FLATFS_MAX_FILES)
        total_inodes = FLATFS_MAX_FILES;

    flatfs_superblock_t sb;
    sb.magic        = FLATFS_MAGIC;
    sb.total_blocks = total_blocks;
    sb.total_inodes = total_inodes;
    sb.free_blocks  = total_blocks;
    sb.free_inodes  = total_inodes;

    uint8_t ata_err = ata_write28_request(drive, FLATFS_SECTOR_SUPERBLOCK, 1, (uint8_t *)&sb);
    if (ata_err == 1)
        return FLATFS_ERR_IO;

    uint8_t zero_sector[ATA_SECTOR_SIZE];
    memset(zero_sector, 0, ATA_SECTOR_SIZE);

    /* zero both bitmap sectors (1 and 2) */
    ata_err = ata_write28_request(drive, FLATFS_SECTOR_INODE_BITMAP, 2, zero_sector);
    if (ata_err == 1)
        return FLATFS_ERR_IO;

    /* zero the entire inode table */
    for (uint32_t i = 0; i < total_inodes; i++) {
        ata_err = ata_write28_request(drive, FLATFS_SECTOR_INODE_TABLE + i, 1, zero_sector);
        if (ata_err == 1)
            return FLATFS_ERR_IO;
    }

    return FLATFS_OK;
}

flatfs_err_t flatfs_mount(flatfs_t *fs, ata_drive_t *drive) {
    if (!fs || !drive)
        return FLATFS_ERR_INVALID;

    if (!drive->exists)
        return FLATFS_ERR_NO_DRIVE;

    flatfs_superblock_t sb;
    uint8_t ata_err = ata_read28_request(drive, FLATFS_SECTOR_SUPERBLOCK, 1, (uint8_t *)&sb);
    if (ata_err == 1)
        return FLATFS_ERR_IO;

    if (sb.magic != FLATFS_MAGIC)
        return FLATFS_ERR_BAD_MAGIC;

    fs->drive = drive;
    memcpy(&fs->sb, &sb, sizeof(flatfs_superblock_t));

    ata_err = ata_read28_request(drive, FLATFS_SECTOR_INODE_BITMAP, 1, fs->inode_bitmap);
    if (ata_err == 1)
        return FLATFS_ERR_IO;

    ata_err = ata_read28_request(drive, FLATFS_SECTOR_BLOCK_BITMAP, 1, fs->block_bitmap);
    if (ata_err == 1)
        return FLATFS_ERR_IO;

    fs->dirty = 0;
    return FLATFS_OK;
}