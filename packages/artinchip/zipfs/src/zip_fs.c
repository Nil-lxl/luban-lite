/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#include <rtthread.h>
#include <dfs_posix.h>
#include <aic_errno.h>
#include "zip_reader.h"
#include "stdbool.h"
#include "zip_fs.h"

static const struct dfs_file_ops zipfs_fops;

static int zipfs_ops_mount(struct dfs_filesystem *fs, unsigned long rwflag, const void *data)
{
    fs->data = (void *)data;
    return 0;
}

static int zipfs_ops_unmount(struct dfs_filesystem *fs)
{
    struct aic_zip_reader *reader = (struct aic_zip_reader *)fs->data;
    aic_zip_reader_close(reader);
    return 0;
}

static int zipfs_ops_stat(struct dfs_filesystem *fs, const char *path, struct stat *st)
{
    struct aic_zip_reader *reader = (struct aic_zip_reader *)fs->data;
    int i = 0;

    for (i = 0; i < reader->file_count; i++) {
        size_t file_size = reader->file_info[i].size;
        char *file_name = reader->file_info[i].name;
        int j = 0;

        for (j = 0; j < strlen(path) && path[j] == file_name[j]; j++) {
        }

        if (j == strlen(path)) {
            if (file_name[j] == '/') {
                st->st_mode = S_IFDIR;
                st->st_size = 0;
            } else if (file_name[j] == '\0') {
                st->st_mode = S_IFREG;
                st->st_size = file_size;
            } else {
                continue;
            }
            return 0;
        }
    }

    return -ENOENT;
}

static const struct dfs_filesystem_ops zipfs_ops = {
    FS_NAME,
    DFS_FS_FLAG_DEFAULT,
    &zipfs_fops,
    zipfs_ops_mount,   /* mount */
    zipfs_ops_unmount, /* unmount */
    NULL,              /* mkfs */
    NULL,              /* statfs */
    NULL,              /* unlink */
    zipfs_ops_stat,    /* stat */
    NULL,              /* rename */
};

static int zipfs_fopen(struct dfs_fd *fd)
{
    struct aic_zip_reader *reader = (struct aic_zip_reader *)fd->fs->data;
    int i = 0;

    if ((fd->flags & O_ACCMODE) != O_RDONLY)
        return -EINVAL;

    if (strcmp(fd->path, "/") == 0)
        return 0;

    for (i = 0; i < reader->file_count; i++) {
        char *file_name = reader->file_info[i].name;
        int j = 0;

        for (j = 0; j < strlen(fd->path) && fd->path[j] == file_name[j]; j++) {
        }

        if (j == strlen(fd->path)) {
            if (file_name[j] == '/' && fd->flags == O_DIRECTORY) {
                return 0;
            } else if (file_name[j] == '\0' && fd->flags != O_DIRECTORY) {
                fd->size = reader->file_info[i].size;
                fd->data = mz_zip_reader_extract_file_iter_new(&reader->archive, file_name + 1, 0);
                return 0;
            } else {
                continue;
            }
        }
    }

    return -ENOENT;
}

static int zipfs_fread(struct dfs_fd *fd, void *buf, size_t count)
{
    struct aic_zip_reader *reader = (struct aic_zip_reader *)fd->fs->data;
    rt_size_t length;
    size_t read_size = 0;

    length = count > (fd->size - fd->pos) ? (fd->size - fd->pos) : count;

    if (length > 0) {
        for (int i = 0; i < reader->file_count; i++) {
            char *file_name = reader->file_info[i].name;
            if (!strcmp(fd->path, file_name)) {
                mz_zip_reader_extract_iter_state *iter =
                    (mz_zip_reader_extract_iter_state *)fd->data;

                read_size = mz_zip_reader_extract_iter_read(iter, buf, length);
                break;
            }
        }
    }

    fd->pos += read_size;

    return read_size;
}

static int zipfs_fclose(struct dfs_fd *fd)
{
    if (fd->data != NULL) {
        mz_zip_reader_extract_iter_free(fd->data);
        fd->data = NULL;
    }

    return 0;
}

static int zipfs_lseek(struct dfs_fd *fd, off_t offset)
{
    mz_zip_reader_extract_iter_state *iter = NULL;
    uint8_t *temp_buf = NULL;

    if (offset < 0 || offset > fd->size)
        return -EINVAL;

    if (offset == fd->pos)
        return offset;
    if (offset < fd->pos) {
        struct aic_zip_reader *reader = (struct aic_zip_reader *)fd->fs->data;
        iter = (mz_zip_reader_extract_iter_state *)fd->data;
        uint32_t file_index = iter->file_stat.m_file_index;

        mz_zip_reader_extract_iter_free(iter);
        fd->data = mz_zip_reader_extract_iter_new(&reader->archive, file_index, 0);
        fd->pos = 0;
    }

    temp_buf = malloc(offset);
    iter = (mz_zip_reader_extract_iter_state *)fd->data;
    mz_zip_reader_extract_iter_read(iter, temp_buf, offset);
    free(temp_buf);
    return offset;
}

static int zipfs_getdents(struct dfs_fd *fd, struct dirent *dirp, uint32_t count)
{
    struct aic_zip_reader *reader = (struct aic_zip_reader *)fd->fs->data;
    const char *path = fd->path;
    int index = 0;
    uint32_t i = 0;
    uint32_t entries_processed = fd->pos;

    const uint32_t max_entries = count / sizeof(struct dirent);
    if (max_entries == 0)
        return -EINVAL;

    for (i = entries_processed; i < reader->file_count && index < max_entries; i++) {
        const char *file_name = reader->file_info[i].name;
        uint32_t l = 0, r = 0;
        for (l = 0; l < strlen(path) && path[l] == file_name[l]; l++) {
        }

        if (l == 1)
            l = 0;
        else if (l == 0 || (file_name[l] != '\0' && file_name[l] != '/'))
            continue;

        for (r = l + 1; r < strlen(file_name) && file_name[r] != '\0' && file_name[r] != '/'; r++) {
        }

        if (r == (l + 1))
            continue;

        bool duplicate = false;
        for (int k = 0; k < entries_processed; k++) {
            if (strncmp(dirp[k].d_name, file_name + l + 1, r - l - 1) == 0) {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
            continue;

        dirp[index].d_type = (file_name[r] == '/') ? DT_DIR : DT_REG;
        dirp[index].d_namlen = r - l;
        dirp[index].d_reclen = (uint16_t)sizeof(struct dirent);
        strncpy(dirp[index].d_name, file_name + l + 1, r - l - 1);
        dirp[index].d_name[r - l] = '\0';
        index++;
    }

    fd->pos = i;
    return index * sizeof(struct dirent);
}

static const struct dfs_file_ops zipfs_fops = {
    zipfs_fopen,    /* open */
    zipfs_fclose,   /* close */
    NULL,           /* ioctl */
    zipfs_fread,    /* read */
    NULL,           /* write */
    NULL,           /* flush */
    zipfs_lseek,    /* lseek */
    zipfs_getdents, /* getdents */
    NULL,           /* poll */
};

int zipfs_register()
{
    if (dfs_register(&zipfs_ops) != 0)
        return -1;

    return 0;
}

int zipfs_mount(const char *source, const char *mount_point)
{
    struct aic_zip_reader *reader = aic_zip_reader_open(source);
    if (reader == NULL)
        return -1;

    if (dfs_mount(RT_NULL, mount_point, FS_NAME, 0, reader) != 0) {
        aic_zip_reader_close(reader);
        return -1;
    }

    printf("ZIP mounted at %s\n", mount_point);
    return 0;
}

int zipfs_unmount(const char *mount_point)
{
    if (dfs_unmount(mount_point) != 0)
        return -1;

    return 0;
}

INIT_COMPONENT_EXPORT(zipfs_register);
