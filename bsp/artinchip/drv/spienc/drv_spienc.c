/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Hao Xiong <hao.xiong@artinchip.com>
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <hal_spienc.h>
#include <drv_spienc.h>
#include <rtdevice.h>
#include <drivers/pm.h>

static struct rt_device spienc_dev = { 0 };
#ifdef AIC_USING_PM
static rt_bool_t g_spienc_clk_pm_flag = RT_FALSE;

static int drv_spienc_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode) {
        case PM_SLEEP_MODE_IDLE:
            break;
        case PM_SLEEP_MODE_LIGHT:
        case PM_SLEEP_MODE_DEEP:
        case PM_SLEEP_MODE_STANDBY:
            if (hal_clk_is_enabled(CLK_SPIENC)) {
#ifdef AIC_PM_DRV_V15
                hal_clk_disable_assertrst(CLK_SPIENC);
#else
                hal_clk_disable(CLK_SPIENC);
#endif
                g_spienc_clk_pm_flag = RT_TRUE;
            }
            break;
        default:
            break;
    }
    return 0;
}

static void drv_spienc_resume(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode) {
        case PM_SLEEP_MODE_IDLE:
            break;
        case PM_SLEEP_MODE_LIGHT:
        case PM_SLEEP_MODE_DEEP:
        case PM_SLEEP_MODE_STANDBY:
            if (g_spienc_clk_pm_flag && !hal_clk_is_enabled(CLK_SPIENC)) {
#ifdef AIC_PM_DRV_V15
                hal_clk_enable_deassertrst(CLK_SPIENC);
#else
                hal_clk_enable(CLK_SPIENC);
#endif
            }
            if (g_spienc_clk_pm_flag)
                g_spienc_clk_pm_flag = RT_FALSE;
            break;
        default:
            break;
    }
}

static struct rt_device_pm_ops drv_spienc_pm_ops = {
    SET_DEVICE_PM_OPS(drv_spienc_suspend, drv_spienc_resume)
    NULL,
};
#endif

int drv_spienc_init(void)
{
    int ret;

    ret = hal_spienc_init();
    if (ret) {
        LOG_E("Failed to initialize spienc.\n");
        return RT_FALSE;
    }

#ifdef KERNEL_RTTHREAD
    rt_device_register(&spienc_dev, "spienc", RT_DEVICE_FLAG_DEACTIVATE);
#ifdef AIC_USING_PM
    rt_pm_device_register(&spienc_dev, &drv_spienc_pm_ops);
#endif
#endif

    return RT_TRUE;
}

void drv_spienc_set_cfg(u32 spi_bus, u32 addr, u32 cpos, u32 clen)
{
    hal_spienc_set_cfg(spi_bus, addr, cpos, clen);
}

void drv_spienc_xip_enable(void)
{
    hal_spienc_xip_enable();
}

void drv_spienc_xip_disable(void)
{
    hal_spienc_xip_disable();
}

void drv_spienc_start(void)
{
    hal_spienc_start();
}

void drv_spienc_stop(void)
{
    hal_spienc_stop();
}

int drv_spienc_check_empty(void)
{
    return hal_spienc_check_empty();
}

