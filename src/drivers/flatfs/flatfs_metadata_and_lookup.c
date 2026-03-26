#include "drivers/flatfs/flatfs_driver.h"
#include "utils.h"

flatfs_err_t flatfs_stat(flatfs_t *fs,
                         const char *name,
                         flatfs_inode_t *out) {
    if (!fs || !name || !out)
        return FLATFS_ERR_INVALID;

    uint32_t inode_idx;
    flatfs_err_t err = flatfs_find(fs, name, &inode_idx);
    if (err != FLATFS_OK)
        return err;

    return flatfs_get_inode_by_index(fs, inode_idx, out);
}

flatfs_err_t flatfs_find(flatfs_t *fs,
                         const char *name,
                         uint32_t *inode_idx) {
    if (!fs || !name || !inode_idx)
        return FLATFS_ERR_INVALID;

    flatfs_inode_t inode;

    for (uint32_t i = 0; i < fs->sb.total_inodes; i++) {
        if (bitmap_get(fs->inode_bitmap, i) == 0)
            continue;

        if (ata_read28_request(fs->drive,
                               FLATFS_SECTOR_INODE_TABLE + i,
                               1,
                               (uint8_t *)&inode) != 0)
            return FLATFS_ERR_IO;

        if (inode.in_use && strncmp(inode.name, name, FLATFS_NAME_MAX) == 0) {
            *inode_idx = i;
            return FLATFS_OK;
        }
    }

    return FLATFS_ERR_NOT_FOUND;
}

flatfs_err_t flatfs_get_inode_by_index(flatfs_t *fs,
                                       uint32_t inode_idx,
                                       flatfs_inode_t *inode) {
    if (!fs || !inode)
        return FLATFS_ERR_INVALID;

    if (inode_idx >= fs->sb.total_inodes)
        return FLATFS_ERR_INVALID;

    if (bitmap_get(fs->inode_bitmap, inode_idx) == 0)
        return FLATFS_ERR_NOT_FOUND;

    if (ata_read28_request(fs->drive,
                           FLATFS_SECTOR_INODE_TABLE + inode_idx,
                           1,
                           (uint8_t *)inode) != 0)
        return FLATFS_ERR_IO;

    return FLATFS_OK;
}