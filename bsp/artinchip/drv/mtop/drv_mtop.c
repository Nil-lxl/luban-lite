/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aic_core.h"
#include "aic_drv_mtop.h"
#include <rtdevice.h>

#ifdef RT_USING_PM
#include <drivers/pm.h>
#include <rtdef.h>
#endif

struct mtop_dev aic_mtop =
{
    .name = "mtop",
};

rt_err_t mtop_ops_init(rt_device_t dev)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;
    struct aic_mtop_dev *phandle = &p_aic_mtop->mtop_handle;
    int ret;

    ret = hal_mtop_init(&p_aic_mtop->mtop_handle);
    if (ret)
        return -RT_ERROR;
    aicos_request_irq(phandle->irq_num, hal_mtop_irq_handler, 0, NULL, (void *)phandle);
    return RT_EOK;
}

void aic_mtop_callback(struct aic_mtop_dev *phandle, void *arg)
{
    struct mtop_dev *p_aic_mtop;
    rt_device_t dev;

    p_aic_mtop = rt_container_of(phandle, struct mtop_dev, mtop_handle);
    dev = (rt_device_t)p_aic_mtop;

    if (dev->rx_indicate)
        dev->rx_indicate(dev, 0);
}

rt_err_t mtop_ops_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;
    struct aic_mtop_dev *phandle = &p_aic_mtop->mtop_handle;

#ifdef RT_USING_PM
    /*
     * Stall PM at NONE while the device is opened. Only the first open counts;
     * nested rt_device_open() invokes open again — skip duplicate request.
     */
    if (!(dev->open_flag & RT_DEVICE_OFLAG_OPEN))
        rt_pm_request(PM_SLEEP_MODE_NONE);
#endif
    hal_mtop_attach_callback(phandle, aic_mtop_callback, NULL);
    hal_mtop_irq_enable(phandle, true);
    return RT_EOK;
}

rt_err_t mtop_ops_close(rt_device_t dev)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;
    struct aic_mtop_dev *phandle = &p_aic_mtop->mtop_handle;

#ifdef RT_USING_PM
    rt_pm_release(PM_SLEEP_MODE_NONE);
#endif
    hal_mtop_irq_enable(phandle, false);
    hal_mtop_detach_callback(phandle);
    hal_mtop_disable(phandle);
    return RT_EOK;
}

rt_err_t mtop_ops_control(rt_device_t dev, int cmd, void *args)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;
    struct aic_mtop_dev *phandle = &p_aic_mtop->mtop_handle;
    uint32_t freq, period_cnt;

    switch (cmd) {
    case MTOP_SET_PERIOD_MODE:
        if (!args)
            return -RT_EINVAL;
        freq = hal_clk_get_freq(CLK_APB0);
        period_cnt = freq / *(unsigned int *)args - 1;
        hal_mtop_set_period_cnt(phandle, period_cnt);
        break;
    case MTOP_ENABLE:
        hal_mtop_enable(phandle);
        break;
    default:
        break;
    }

    return RT_EOK;
}

#ifdef RT_USING_PM
static int aic_mtop_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct mtop_dev *priv = rt_container_of(device, struct mtop_dev, dev);

    switch (mode) {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        priv->mtop_clk_pm_flag  = 0;
        if (hal_clk_is_enabled(CLK_MTOP) > 0) {
            hal_clk_disable(CLK_MTOP);
            priv->mtop_clk_pm_flag  = 1;
        }
        break;
    default:
        break;
    }

    return 0;
}

static void aic_mtop_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct mtop_dev *priv = rt_container_of(device, struct mtop_dev, dev);

    if (!priv->mtop_clk_pm_flag)
        return;

    switch (mode) {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        if (hal_clk_is_enabled(CLK_MTOP) == 0)
            hal_clk_enable(CLK_MTOP);
        priv->mtop_clk_pm_flag  = 0;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_mtop_pm_ops = {
    SET_DEVICE_PM_OPS(aic_mtop_suspend, aic_mtop_resume)
    NULL,
};
#endif

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops aic_mtop_ops =
{
    mtop_ops_init,
    mtop_ops_open,
    mtop_ops_close,
    NULL,
    NULL,
    mtop_ops_control
};
#endif

int drv_mtop_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    aic_mtop.dev.ops = &aic_mtop_ops;
#else
    aic_mtop.dev.init = mtop_ops_init;
    aic_mtop.dev.open = mtop_ops_open;
    aic_mtop.dev.close = mtop_ops_close;
    aic_mtop.dev.control = mtop_ops_control;
    aic_mtop.dev.type = RT_Device_Class_Miscellaneous;
#endif
    rt_device_register(&aic_mtop.dev, "mtop", 0);
#ifdef RT_USING_PM
    rt_pm_device_register(&aic_mtop.dev, &aic_mtop_pm_ops);
#endif
    return 0;
}

INIT_DEVICE_EXPORT(drv_mtop_init);
