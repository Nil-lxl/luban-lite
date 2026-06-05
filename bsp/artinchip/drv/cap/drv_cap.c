/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include <string.h>
#include <math.h>
#include <drivers/rt_inputcapture.h>
#include <drivers/pm.h>

#define LOG_TAG         "CAP"
#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_cap.h"

struct aic_cap {
    struct rt_inputcapture_device rtdev;
    struct aic_cap_data *data;
    rt_bool_t cap_clk_pm_flag;
};

static struct aic_cap *g_cap[AIC_CAP_CH_NUM];
static struct aic_cap_data g_cap_info[] = {
#ifdef AIC_USING_CAP0
    {.id = 0,
     .flt_sel = AIC_CAP0_FLT_SEL,
    },
#endif
#ifdef AIC_USING_CAP1
    {.id = 1,
     .flt_sel = AIC_CAP1_FLT_SEL,
    },
#endif
#ifdef AIC_USING_CAP2
    {.id = 2,
     .flt_sel = AIC_CAP2_FLT_SEL,
    },
#endif
#ifdef AIC_USING_CAP3
    {.id = 3,
     .flt_sel = AIC_CAP3_FLT_SEL,
    },
#endif
#ifdef AIC_USING_CAP4
    {.id = 4,
     .flt_sel = AIC_CAP4_FLT_SEL,
    },
#endif
#ifdef AIC_USING_CAP5
    {.id = 5,
     .flt_sel = AIC_CAP5_FLT_SEL,
    },
#endif
};

static rt_err_t aic_cap_init(struct rt_inputcapture_device *capture)
{
    struct aic_cap *aic_capture;

    RT_ASSERT(capture != RT_NULL);

    aic_capture = (struct aic_cap *)capture;

    hal_cap_ch_init(aic_capture->data->id);

    return RT_EOK;
}

static rt_err_t aic_cap_open(struct rt_inputcapture_device *capture)
{
    struct aic_cap *aic_capture;

    RT_ASSERT(capture != RT_NULL);

    aic_capture = (struct aic_cap *)capture;

    hal_cap_in_config(aic_capture->data->id);
    hal_cap_in_flt_sel(aic_capture->data->id, aic_capture->data->flt_sel);
    hal_cap_cnt_start(aic_capture->data->id);

    return RT_EOK;
}

static rt_err_t aic_cap_close(struct rt_inputcapture_device *capture)
{
    struct aic_cap *aic_capture;

    RT_ASSERT(capture != RT_NULL);

    aic_capture = (struct aic_cap *)capture;

    hal_cap_int_enable(aic_capture->data->id, 0);
    hal_cap_cnt_stop(aic_capture->data->id);
    hal_cap_disable(aic_capture->data->id);

    return RT_EOK;
}

static rt_err_t aic_cap_get_pulsewidth(struct rt_inputcapture_device *capture, rt_uint32_t *pulsewidth_us)
{
    struct aic_cap *aic_capture;
    int cap_clk_rate;

    RT_ASSERT(capture != RT_NULL);

    cap_clk_rate = hal_cap_get_clk_rate();
    if (cap_clk_rate < 0)
        return -RT_ERROR;

    aic_capture = (struct aic_cap *)capture;

    aic_capture->data->freq = round((float)cap_clk_rate / (float)hal_cap_reg2(aic_capture->data->id));
    aic_capture->data->duty = (float)hal_cap_reg1(aic_capture->data->id) * 100 / (float)hal_cap_reg2(aic_capture->data->id);

    aic_capture->rtdev.parent.user_data = (void *)aic_capture->data;

    *pulsewidth_us = hal_cap_reg2(aic_capture->data->id) / (cap_clk_rate / 1000000);

    return RT_EOK;
}

static struct rt_inputcapture_ops aic_input_ops =
{
    .init   =   aic_cap_init,
    .open   =   aic_cap_open,
    .close  =   aic_cap_close,
    .get_pulsewidth =   aic_cap_get_pulsewidth,
};

irqreturn_t aic_cap_irq(int irq, void *arg)
{
    u32 stat;

#if defined (AIC_CAP_DRV_V10) || defined (AIC_CAP_DRV_V11)
    for (int i = 0; i < AIC_CAP_CH_NUM; i++) {
        stat = hal_cap_int_flg(i);
        if (stat & CAP_EVENT3_FLG) {
            rt_hw_inputcapture_isr(&g_cap[i]->rtdev, 0);
            hal_cap_clr_flg(i, CAP_EVENT3_FLG);
        }
    }
#endif

#ifdef AIC_CAP_DRV_V12
    u32 ch = irq - CAP0_IRQn;
    stat = hal_cap_int_flg(ch);
    if (stat & CAP_EVENT3_FLG) {
        rt_hw_inputcapture_isr(&g_cap[ch]->rtdev, 0);
        hal_cap_clr_flg(ch, CAP_EVENT3_FLG);
    }
#endif
    return IRQ_HANDLED;
}

#ifdef RT_USING_PM
static int drv_cap_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_cap *cap_dev = (struct aic_cap *)device;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        if (hal_clk_is_enabled(CLK_PWMCS)) {
#ifdef AIC_PM_DRV_V15
            hal_clk_disable_assertrst(CLK_PWMCS);
#else
            hal_clk_disable(CLK_PWMCS);
#endif
            cap_dev->cap_clk_pm_flag = RT_TRUE;
        }
        break;
    default:
        break;
    }
    return 0;
}

static void drv_cap_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_cap *cap_dev = (struct aic_cap *)device;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
#if defined (AIC_CAP_DRV_V10)
        hal_clk_set_freq(CLK_PWMCS, PWMCS_CLK_RATE);
#elif defined (AIC_CAP_DRV_V11)
        hal_clk_set_freq(CLK_PWMCS_SDFM, PWMCS_CLK_RATE);
#endif
        if (cap_dev->cap_clk_pm_flag && !hal_clk_is_enabled(CLK_PWMCS)) {
#ifdef AIC_PM_DRV_V15
            hal_clk_enable_deassertrst(CLK_PWMCS);
#else
            hal_clk_enable(CLK_PWMCS);
#endif
        }
        if (cap_dev->cap_clk_pm_flag)
            cap_dev->cap_clk_pm_flag = RT_FALSE;

        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops drv_cap_pm_ops =
{
    SET_DEVICE_PM_OPS(drv_cap_suspend, drv_cap_resume)
    NULL,
};
#endif

static rt_err_t aic_cap_probe(struct aic_cap_data *pdata)
{
    struct aic_cap *cap;
    rt_err_t ret = RT_EOK;
    char aic_cap_device_name[10] = "";

    cap = (struct aic_cap *)malloc(sizeof(struct aic_cap));
    if (!cap) {
        LOG_E("Failed to malloc(%d)\n", (u32)sizeof(struct aic_cap));
        return -RT_ENOMEM;
    }

    cap->data = pdata;
    cap->rtdev.ops = &aic_input_ops;

    /* store the pointer */
    g_cap[cap->data->id] = cap;

    snprintf(aic_cap_device_name, 10, "cap%d", cap->data->id);

    ret = rt_device_inputcapture_register(&cap->rtdev, aic_cap_device_name, NULL);
    if (ret == RT_EOK) {
        LOG_I("ArtInChip %s loaded", aic_cap_device_name);
    } else {
        LOG_E("%s register failed", aic_cap_device_name);
        goto err;
    }

#ifdef RT_USING_PM
    rt_pm_device_register(&cap->rtdev.parent, &drv_cap_pm_ops);
#endif

    return ret;

err:
    if (cap)
        free(cap);

    return ret;
}

static int drv_cap_init(void)
{
    rt_err_t ret = RT_EOK;
    u32 i;

    for (i = 0; i < ARRAY_SIZE(g_cap_info); i++) {
        ret = aic_cap_probe(&g_cap_info[i]);
        if (ret)
            return ret;
#ifdef AIC_CAP_DRV_V12
        aicos_request_irq(CAP0_IRQn + g_cap_info[i].id, aic_cap_irq, 0, NULL, NULL);
#endif
    }

    hal_cap_init();

#if defined (AIC_CAP_DRV_V10) || defined (AIC_CAP_DRV_V11)
    aicos_request_irq(PWMCS_CAP_IRQn, aic_cap_irq, 0, NULL, NULL);
#endif
    return ret;
}
INIT_DEVICE_EXPORT(drv_cap_init);
