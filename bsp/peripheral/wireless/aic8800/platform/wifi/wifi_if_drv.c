/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pin.h>
#include <drivers/sdio.h>
#include <drivers/mmcsd_card.h>
#include <rtdevice.h>
#include <rtthread.h>

#include "sdio_func.h"
#include "sdio_port.h"
#include "aic_plat_log.h"
#include "rtos_port.h"
#include "plat_port.h"

struct rt_mmcsd_card *g_wifi_if_sdio = NULL;
struct sdio_func g_wifi_if_sdio_funcs[SDIOM_MAX_FUNCS];

int aic8800_reset(void)
{
    return 0;
}

int aic8800_power_on(void)
{
    return 0;
}

int aic8800_power_off(void)
{
    return 0;
}

static rt_int32_t wifi_if_sdio_probe(struct rt_mmcsd_card *card)
{
    int idx;
    AIC_LOG_PRINTF("%s: card_type=%d, host_flags=0x%x\n", __func__, card->card_type, card->host->flags);
    g_wifi_if_sdio = card;
    for (idx = 0; idx < SDIOM_MAX_FUNCS; idx++) {
        g_wifi_if_sdio_funcs[idx].num    = card->sdio_function[idx]->num;
        g_wifi_if_sdio_funcs[idx].vendor = card->sdio_function[idx]->manufacturer;
        g_wifi_if_sdio_funcs[idx].device = card->sdio_function[idx]->product;
        g_wifi_if_sdio_funcs[idx].drv_priv = card;
    }

    aicwf_sdio_probe(&g_wifi_if_sdio_funcs[0]);
}

static rt_int32_t wifi_if_sdio_remove(struct rt_mmcsd_card *card)
{
    aicwf_sdio_remove(&g_wifi_if_sdio_funcs[0]);
    return 0;
}

struct rt_sdio_device_id wifi_if_sdio_ids[]= {
    { 0, 0xc8a1, 0x0082},
    { 0, 0xc8a1, 0xc08d},
    { },
};

struct rt_sdio_driver wifi_if_sdio_drv = {
    "aicwfsdio",
    wifi_if_sdio_probe,
    wifi_if_sdio_remove,
    wifi_if_sdio_ids,
};

#if 1
int wifi_if_sdio_init(void)
{
    AIC_LOG_PRINTF("sdio id = 0x%x\n", wifi_if_sdio_drv.id->product);
    sdio_register_driver(&wifi_if_sdio_drv);

    return 0;
}

#else
int wifi_if_sdio_init_my(void)
{
    platform_pwr_wifi_pin_init();
    platform_pwr_wifi_pin_disable();
    rtos_task_suspend(10);
    platform_pwr_wifi_pin_enable();

    AIC_LOG_PRINTF("sdio id = 0x%x\n", wifi_if_sdio_drv.id->product);
    sdio_register_driver(&wifi_if_sdio_drv);

    return 0;
}
INIT_COMPONENT_EXPORT(wifi_if_sdio_init_my);
#endif
