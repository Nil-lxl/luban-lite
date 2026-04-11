/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#ifndef __ZIPFS_H_
#define __ZIPFS_H_

#include <stddef.h>

#define FS_NAME "zipfs"

/**
 * @brief Mount a specified ZIP archive as a virtual filesystem at the target path.
 *        Creates an in-memory read-only view under `mount_point`, where subsequent
 *        POSIX filesystem operations will transparently access files within the ZIP.
 *
 * @param source Full path to ZIP file in RAMDisk, or block name where ZIP data in
 *               example: "/rodata/zipfs_test/much_file.zip", "blk_rodata_r"
 * @param mount_point Target mount location in the system (e.g., "/zipfs"). Must be an
 *                    empty directory not currently mounted by other filesystems.
 *
 * @return Returns 0 on success; returns -1 on failure.
 */
int zipfs_mount(const char *source, const char *mount_point);

/**
 * @brief Unmount a previously mounted ZIP filesystem, immediately releasing all resources
 *        associated with the specified mount point.
 *
 * @param mount_point The mount point of the ZIP filesystem to unmount.
 *
 * @return Returns 0 on success; returns -1 on failure.
 */
int zipfs_unmount(const char *mount_point);

#endif // __ZIPFS_H_
