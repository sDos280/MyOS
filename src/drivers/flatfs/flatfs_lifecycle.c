#include "mm/kheap.h"
#include "drivers/flatfs/flatfs_driver.h"
#include "utils/utils.h"

flatfs_err_t flatfs_format(ata_drive_t *drive,
                           uint32_t total_inodes,
                           uint32_t sectors_per_block) {
    if (!drive)
        return FLATFS_ERR_INVALID;

    if (!drive->exists)
        return FLATFS_ERR_NO_DRIVE;

    flatfs_superblock_t sb;

    sb.magic             = FLATFS_MAGIC;
    sb.sectors_per_block = sectors_per_block;
    sb.total_sectors     = drive->size_in_sectors;
    sb.total_inodes      = total_inodes;
    sb.free_inodes       = total_inodes;

    /* Convert drive size from sectors to FlatFS blocks */
    uint32_t total_drive_blocks = drive->size_in_sectors / sectors_per_block; 
    /* Inode bitmap starts right after superblock */
    sb.inode_bitmap_start = FLATFS_BLOCK_SUPERBLOCK + 1;
    sb.inode_bitmap_block_count = FLATFS_BITMAP_BLOCKS(total_inodes, &sb);
    /* Block bitmap starts right after inode bitmap */
    sb.block_bitmap_start = sb.inode_bitmap_start + sb.inode_bitmap_block_count;
    sb.inode_table_block_count = FLATFS_INODE_TABLE_BLOCKS(&sb);

    /*
    * total_blocks is circular: so we calculate it iteretivly
    *
    * total_blocks
    *   -> block_bitmap_block_count
    *   -> inode_table_start
    *   -> data_start_block
    *   -> total_blocks
    */
    uint32_t prev_total_blocks = 0;
    uint32_t total_blocks = 0; /* first guess: no free blocks */
    do {
        prev_total_blocks = total_blocks;

        /* Block bitmap needs 1 bit per usable data block */
        sb.block_bitmap_block_count = FLATFS_BITMAP_BLOCKS(prev_total_blocks, &sb);
        /* Inode table starts after block bitmap */
        sb.inode_table_start = sb.block_bitmap_start + sb.block_bitmap_block_count;
        /* Data starts after inode table */
        sb.data_start_block = sb.inode_table_start + sb.inode_table_block_count;

        if (sb.data_start_block >= total_drive_blocks) 
            return FLATFS_ERR_NO_SPACE;

        /* New guess: usable data blocks = all drive blocks - metadata blocks */
        total_blocks =  total_drive_blocks - sb.data_start_block;
    } while (total_blocks != prev_total_blocks);

    sb.total_blocks = total_blocks;
    sb.free_blocks  = total_blocks;

    /* set file system object */
   flatfs_t fs_local;
    memset(&fs_local, 0, sizeof(flatfs_t));
    fs_local.drive = drive;
    memcpy(&fs_local.sb, &sb, sizeof(flatfs_superblock_t));

    uint8_t *temp_block = kalloc(FLATFS_BLOCK_SIZE(&sb));
    if (!temp_block)
        return FLATFS_ERR_NO_MEM;
    memset(temp_block, 0, FLATFS_BLOCK_SIZE(&sb)); /* clear temp block */
    
    /* write superblock */
    memcpy(temp_block, &sb, sizeof(flatfs_superblock_t));
    flatfs_err_t err = flatfs_write_blocks(&fs_local, FLATFS_BLOCK_SUPERBLOCK, 1, temp_block);
    if (err != FLATFS_OK) {
        kfree(temp_block);
        return err;
    }

    /* zero both bitmap sectors (1 and 2) */
    memset(temp_block, 0, FLATFS_BLOCK_SIZE(&sb)); /* clear temp block */
    for (uint32_t i = 0; i < sb.inode_bitmap_block_count; i++) {
        err = flatfs_write_blocks(&fs_local, sb.inode_bitmap_start + i, 1, temp_block);
        if (err != FLATFS_OK) {
            kfree(temp_block);
            return err;
        }
    }
    for (uint32_t i = 0; i < sb.block_bitmap_block_count; i++) {
        err = flatfs_write_blocks(&fs_local, sb.block_bitmap_start + i, 1, temp_block);
        if (err != FLATFS_OK) {
            kfree(temp_block);
            return err;
        }
    }

    /* zero the entire inode table */
    for (uint32_t i = 0; i < sb.inode_table_block_count; i++) {
        err = flatfs_write_blocks(&fs_local, sb.inode_table_start + i, 1, temp_block);
        if (err != FLATFS_OK) {
            kfree(temp_block);
            return err;
        }
    }

    kfree(temp_block);
    return FLATFS_OK;
}

flatfs_err_t flatfs_mount(flatfs_t *fs, ata_drive_t *drive) {
    if (!fs || !drive)
        return FLATFS_ERR_INVALID;

    if (!drive->exists)
        return FLATFS_ERR_NO_DRIVE;

    uint8_t sector_buf[ATA_SECTOR_SIZE];
    memset(sector_buf, 0, sizeof(sector_buf));
    uint8_t ata_err = ata_read28_request(drive, FLATFS_SECTOR_SUPERBLOCK, 1, (uint8_t *)&sector_buf);
    if (ata_err != 0)
        return FLATFS_ERR_IO;
    
    flatfs_superblock_t sb;
    memcpy(&sb, sector_buf, sizeof(flatfs_superblock_t));

    if (sb.magic != FLATFS_MAGIC)
        return FLATFS_ERR_BAD_MAGIC;

    fs->drive = drive;
    memcpy(&fs->sb, &sb, sizeof(flatfs_superblock_t));
    fs->inode_bitmap = kalloc(sb.inode_bitmap_block_count * FLATFS_BLOCK_SIZE(&sb));
    if (!fs->inode_bitmap)
            return FLATFS_ERR_NO_MEM;
    fs->block_bitmap = kalloc(sb.block_bitmap_block_count * FLATFS_BLOCK_SIZE(&sb));
    if (!fs->block_bitmap) {
        kfree(fs->inode_bitmap);
        return FLATFS_ERR_NO_MEM;
    }

    /* read on-disk information about bitmaps */
    flatfs_err_t err = flatfs_read_blocks(fs, fs->sb.inode_bitmap_start, sb.inode_bitmap_block_count, fs->inode_bitmap);
    if (err != FLATFS_OK) {
        kfree(fs->inode_bitmap);
        kfree(fs->block_bitmap);
        return err;
    }

    err = flatfs_read_blocks(fs, fs->sb.block_bitmap_start, sb.block_bitmap_block_count, fs->block_bitmap);
    if (err != FLATFS_OK) {
        kfree(fs->inode_bitmap);
        kfree(fs->block_bitmap);
        return err;
    }

    return FLATFS_OK;
}

flatfs_err_t flatfs_unmount(flatfs_t *fs) {
    if (!fs)
        return FLATFS_ERR_INVALID;

    flatfs_err_t err;

    uint8_t *temp_block = (uint8_t *)kalloc(FLATFS_BLOCK_SIZE(&fs->sb));
    if (!temp_block)
        return FLATFS_ERR_NO_MEM;

    /* flush superblock */
    memset(temp_block, 0, FLATFS_BLOCK_SIZE(&fs->sb));
    memcpy(temp_block, &fs->sb, sizeof(flatfs_superblock_t));
    err = flatfs_write_blocks(fs, FLATFS_BLOCK_SUPERBLOCK, 1, temp_block);
    kfree(temp_block);
    if (err != FLATFS_OK)
        return err;

    /* flush inode bitmap */
    err = flatfs_write_blocks(fs, fs->sb.inode_bitmap_start,
                              fs->sb.inode_bitmap_block_count,
                              fs->inode_bitmap);
    if (err != FLATFS_OK)
        return err;

    /* flush block bitmap */
    err = flatfs_write_blocks(fs, fs->sb.block_bitmap_start,
                              fs->sb.block_bitmap_block_count,
                              fs->block_bitmap);
    if (err != FLATFS_OK)
        return err;

    ata_flush_cache(fs->drive);

    kfree(fs->inode_bitmap);
    kfree(fs->block_bitmap);

    fs->inode_bitmap = NULL;
    fs->block_bitmap = NULL;
    fs->drive        = NULL;

    return FLATFS_OK;
}