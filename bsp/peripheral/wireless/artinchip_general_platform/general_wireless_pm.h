/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#ifndef __GENERAL_WIRELESS_PM_H__
#define __GENERAL_WIRELESS_PM_H__

#include <rtthread.h>
#include <drivers/pm.h>
#include <drivers/mmcsd_card.h>

struct aic_wlan_chip_pm_ops {
    int (*suspend)(rt_uint8_t mode);
    int (*resume)(rt_uint8_t mode);
    struct rt_mmcsd_host *(*get_host)(void);
};

int aic_wlan_pm_register(struct rt_device *device, struct aic_wlan_chip_pm_ops *ops);
void aic_wlan_pm_unregister(struct rt_device *device);
void aic_wlan_pm_sdio_free_card(struct rt_mmcsd_host *host);
struct rt_mmcsd_host *aic_sdmc_get_rthost(int id);

#endif /* __GENERAL_WIRELESS_PM_H__ */
