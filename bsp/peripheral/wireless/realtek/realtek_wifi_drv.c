/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <drivers/pin.h>
#include <drivers/sdio.h>
#include <drivers/mmcsd_card.h>
#include <aic_core.h>
#include <aic_drv.h>
#include "card.h"
#include "wifi_io.h"
#include "rtconfig.h"

static rt_int32_t realtek_probe(struct rt_mmcsd_card *card)
{
#ifdef REALTEK_WLAN_INTF_SDIO
    return (wifi_sdio_probe(card));
#else
    return 0;
#endif
}

static rt_int32_t realtek_remove(struct rt_mmcsd_card *card)
{
#ifdef REALTEK_WLAN_INTF_SDIO
    wifi_sdio_remove(card);
#endif

    return 0;
}

struct rt_sdio_device_id realtex_id[]= {
#if defined(AIC_USING_RTL8733_WLAN0)
    { 1, 0x024c, 0xB733 },
#elif defined(AIC_USING_RTL8189_WLAN0)
    { 1, 0x024c, 0xf179 },
#endif
};

struct rt_sdio_driver realtek_drv = {
    "realtek-wifi",
    realtek_probe,
    realtek_remove,
    realtex_id,
};



int realtek_init(void)
{
    printf("wifi device id == 0x%x\n", realtek_drv.id->product);
    sdio_register_driver(&realtek_drv);

    return 0;
}


