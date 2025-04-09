/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include <usbh_core.h>
#include <usbh_msc.h>
#include <disk_part.h>

struct usbh_msc *active_msc_class;

u32 usbh_msc_read(struct blk_desc *blk_dev, u32 start, u32 blkcnt, const void *buffer)
{
    int ret;

    if (active_msc_class == NULL)
        return -1;

    ret = usbh_msc_scsi_read10(active_msc_class, start, buffer, blkcnt);
    if (ret < 0) {
        pr_err("usb mass_storage read failed\n");
        return 0;
    }
    return (u32)blkcnt;
}

void usbh_msc_run(struct usbh_msc *msc_class)
{
    active_msc_class = msc_class;
}

void usbh_msc_stop(struct usbh_msc *msc_class)
{
}
