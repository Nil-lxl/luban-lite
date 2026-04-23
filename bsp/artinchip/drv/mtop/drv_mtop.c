/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aic_core.h"
#include "aic_drv_mtop.h"

struct mtop_dev aic_mtop =
{
    .name = "mtop",
};

rt_err_t mtop_ops_init(rt_device_t dev)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;
    struct aic_mtop_dev *phandle = &p_aic_mtop->mtop_handle;

    hal_mtop_init(&p_aic_mtop->mtop_handle);
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

    hal_mtop_attach_callback(phandle, aic_mtop_callback, NULL);
    hal_mtop_irq_enable(phandle, true);
    return RT_EOK;
}

rt_err_t mtop_ops_close(rt_device_t dev)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;
    struct aic_mtop_dev *phandle = &p_aic_mtop->mtop_handle;

    hal_mtop_irq_enable(phandle, false);
    hal_mtop_detach_callback(phandle);
    return RT_EOK;
}

rt_err_t mtop_ops_control(rt_device_t dev, int cmd, void *args)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;
    struct aic_mtop_dev *phandle = &p_aic_mtop->mtop_handle;
    uint32_t freq, period_cnt;

    switch (cmd) {
    case MTOP_SET_PERIOD_MODE:
        freq = hal_clk_get_freq(CLK_APB0);
        period_cnt = freq / *(unsigned int*)args - 1;
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
    return 0;
}

INIT_DEVICE_EXPORT(drv_mtop_init);
