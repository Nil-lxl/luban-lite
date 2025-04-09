/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Jianfeng Li <jianfeng.li@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <upg_internal.h>
#include <aic_core.h>
#include <aicupg.h>
#include <fatfs.h>

extern size_t config_ram_size;

static u32 image_size = 0;
static u64 write_size = 0;
static progress_cb g_progress_cb = NULL;

void aicupg_fat_set_process_cb(progress_cb cb)
{
    g_progress_cb = cb;
}

static void *upg_fat_malloc_align(struct fwc_info *fwc, u32 *size, size_t align)
{
    void *ptr = NULL;

    switch (*size) {
        case DATA_WRITE_ONCE_MAX_SIZE:
            *size = ALIGN_DOWN(DATA_WRITE_ONCE_MAX_SIZE, fwc->block_size);
            ptr = aicupg_malloc_align(*size, align);
            if (ptr)
                break;
        case DATA_WRITE_ONCE_MID_SIZE:
            *size = ALIGN_DOWN(DATA_WRITE_ONCE_MID_SIZE, fwc->block_size);
            ptr = aicupg_malloc_align(*size, align);
            if (ptr)
                break;
        case DATA_WRITE_ONCE_MIN_SIZE:
            *size = ALIGN_DOWN(DATA_WRITE_ONCE_MIN_SIZE, fwc->block_size);
            ptr = aicupg_malloc_align(*size, align);
            if (ptr)
                break;
        default:
            ptr = aicupg_malloc_align(*size, align);
            if (ptr)
                break;
    }

    return ptr;
}

static void upg_fat_free_align(void *mem)
{
    aicupg_free_align(mem);
}

#define FRAME_LIST_SIZE 4096
static s32 media_device_write(char *image_name, struct fwc_meta *pmeta)
{
    struct fwc_info *fwc;
    u32 offset, write_once_size, len, remaining_size;
    u8 *buf;
    s32 ret;
    ulong actread, total_len = 0;
    u64 start_us;
    u32 percent;

    fwc = NULL;
    buf = NULL;
    fwc = aicupg_malloc_align(sizeof(struct fwc_info), FRAME_LIST_SIZE);
    if (!fwc) {
        pr_err("Error: malloc fwc failed.\n");
        ret = -1;
        goto err;
    }
    memset((void *)fwc, 0, sizeof(struct fwc_info));

    printf("Firmware component: %s\n", pmeta->name);
    printf("    partition: %s programming ...\n", pmeta->partition);
    /*config fwc */
    fwc_meta_config(fwc, pmeta);

    /*start write data*/
    start_us = aic_get_time_us();
    media_data_write_start(fwc);

    /*config write size once*/
    if (config_ram_size <= 0x400000)
        write_once_size = DATA_WRITE_ONCE_MIN_SIZE;
    else
        write_once_size = DATA_WRITE_ONCE_MAX_SIZE;

    /*malloc buf memory*/
    buf = upg_fat_malloc_align(fwc, &write_once_size, FRAME_LIST_SIZE);
    if (!buf) {
        pr_err("Error: malloc buf failed.\n");
        ret = -1;
        goto err;
    }

    memset((void *)buf, 0, write_once_size);

    offset = 0;
    while (offset < pmeta->size) {
        remaining_size = pmeta->size - offset;
        len = min(remaining_size, write_once_size);
        if (len % fwc->block_size)
            len = ((len / fwc->block_size) + 1) * fwc->block_size;

        ret = aic_fat_read_file(image_name, (void *)buf,
                                pmeta->offset + offset, len, &actread);
        if (actread != len && actread != remaining_size) {
            pr_err("Error:read file failed!\n");
            goto err;
        }
        /* write data to media */
        ret = media_data_write(fwc, buf, len);
        if (ret == 0) {
            pr_err("Error: media write failed!..\n");
            goto err;
        }
        total_len += ret;
        offset += len;

        write_size += ret;
        percent = write_size * 100 / image_size;
        if (g_progress_cb)
            g_progress_cb(percent);
    }

    /* write data end */
    media_data_write_end(fwc);
    /* check partition crc */
    if (fwc->calc_partition_crc != pmeta->crc) {
        pr_err("calc partition crc:0x%x, expect partition crc:0x%x\n",
               fwc->calc_partition_crc, pmeta->crc);
        goto err;
    }
    /* show burn time */
    start_us = aic_get_time_us() - start_us;
    printf("    Partition: %s programming done.\n", pmeta->partition);
    pr_info("    Used time: %lld.%lld sec, Speed: %lld.%lld MB/s.\n",
            start_us / 1000000, start_us / 1000 % 1000,
            (total_len * 1000000 / start_us) / 1024 / 1024,
            (total_len * 1000000 / start_us) / 1024 % 1024);

    upg_fat_free_align(buf);
    aicupg_free_align(fwc);

    return total_len;
err:
    printf("    Partition: %s programming failed.\n", pmeta->partition);
    if (buf)
        upg_fat_free_align(buf);
    if (fwc)
        aicupg_free_align(fwc);
    return 0;
}

/*fat upgrade function*/
s32 aicupg_fat_write(char *image_name, char *protection,
                     struct image_header_upgrade *header)
{
    struct fwc_meta *p;
    struct fwc_meta *pmeta;
    int i, cnt, ret;
    u32 start_us;
    ulong actread, write_len = 0;
    u64 total_len = 0;

    pmeta = NULL;
    pmeta = (struct fwc_meta *)aicupg_malloc_align(header->meta_size, FRAME_LIST_SIZE);
    if (!pmeta) {
        pr_err("Error: malloc for meta failed.\n");
        ret = -1;
        goto err;
    }
    memset((void *)pmeta, 0, header->meta_size);

    start_us = aic_get_time_us();
    /*read meta info*/
    ret = aic_fat_read_file(image_name, (void *)pmeta,
                            header->meta_offset, header->meta_size, &actread);
    if (actread != header->meta_size) {
        pr_err("Error:read file failed!\n");
        goto err;
    }

    /*prepare device*/
    ret = media_device_prepare(NULL, header);
    if (ret != 0) {
        pr_err("Error:prepare device failed!\n");
        goto err;
    }

    if (g_progress_cb)
        g_progress_cb(0);
    image_size = header->file_size;

    p = pmeta;
    cnt = header->meta_size / sizeof(struct fwc_meta);
    for (i = 0; i < cnt; i++) {
        if (!strcmp(p->partition, "") || strstr(protection, p->partition)) {
            p++;
            continue;
        }

        ret = media_device_write(image_name, p);
        if (ret == 0) {
            pr_err("media device write data failed!\n");
            goto err;
        }

        p++;
        write_len += ret;
    }
    if (g_progress_cb)
        g_progress_cb(100);

    total_len = write_len;
    start_us = aic_get_time_us() - start_us;
    printf("All firmaware components programming done.\n");
    printf("    Used time: %u.%u sec, Speed: %lu.%lu MB/s.\n",
           start_us / 1000000, start_us / 1000 % 1000,
           (ulong)((total_len * 1000000 / start_us) / 1024 / 1024),
           (ulong)((total_len * 1000000 / start_us) / 1024 % 1024));

    aicupg_free_align(pmeta);
    return write_len;
err:
    if (pmeta)
        aicupg_free_align(pmeta);
    return 0;
}
