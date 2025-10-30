/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <drivers/hwtimer.h>

#include "drv_hrtimer.h"

#define LOG_TAG                 "HRTimer"
#include "aic_core.h"
#include "hal_pwm.h"

#define HRTIMER_DEV_NAME        "hrtimer"
#define HRTIMER_DEFAULT_CYCLE   1000000

static const struct rt_hwtimer_info drv_hrtimer_info =
{
    .maxfreq = HRTIMER_DEFAULT_CYCLE,
    .minfreq = 1,
    .maxcnt  = 0xFFFF,
    .cntmode = HWTIMER_CNTMODE_UP,
};

static struct hrtimer_info g_hrtimer_info[] = {
#ifdef AIC_USING_HRTIMER0
    {HRTIMER_DEV_NAME"0", 0},
#endif
#ifdef AIC_USING_HRTIMER1
    {HRTIMER_DEV_NAME"1", 1},
#endif
};

struct hrtimer_info *_get_hrtimer_priv(rt_hwtimer_t *timer)
{
    return (struct hrtimer_info *)timer->parent.user_data;
}

irqreturn_t drv_hrtimer_irq(int irq, void *arg)
{
    u32 i;
    u32 stat;

    stat = hal_pwm_int_sts();
    struct hrtimer_info *info = g_hrtimer_info;

    for (i = 0; i < ARRAY_SIZE(g_hrtimer_info); i++, info++) {
        if (stat & (1 << info->id))
            rt_device_hwtimer_isr(&info->hrtimer);
    }

    hal_pwm_clr_int(stat);

    return IRQ_HANDLED;
}

static void drv_hrtimer_init(rt_hwtimer_t *timer, rt_uint32_t state)
{
    struct hrtimer_info *info = _get_hrtimer_priv(timer);
    struct aic_pwm_action action0 = {0};
    struct aic_pwm_action action1 = {0};

    if (state)
        hal_pwm_ch_init(info->id, PWM_MODE_UP_COUNT, 0, &action0, &action1);
    else
        hal_pwm_ch_deinit(info->id);
}

static rt_err_t drv_hrtimer_start(rt_hwtimer_t *timer, rt_uint32_t cnt,
                                  rt_hwtimer_mode_t mode)
{
    struct hrtimer_info *info = _get_hrtimer_priv(timer);
    rt_err_t ret = RT_EOK;

    hal_pwm_int_config(info->id, PWM_PRD_EVENT, 1);

    ret = hal_pwm_enable(info->id);
    if (ret < 0) {
        LOG_E("hrtimer%d enable failed!\n", info->id);
        return ret;
    }

    ret = hal_pwm_set_prd(info->id, cnt);
    if (ret < 0) {
        LOG_E("hrtimer%d set cnt failed!\n", info->id);
        return ret;
    }

    return ret;
}

static void drv_hrtimer_stop(rt_hwtimer_t *timer)
{
    struct hrtimer_info *info = _get_hrtimer_priv(timer);

    hal_pwm_int_config(info->id, 0, 0);

    hal_pwm_disable(info->id);
}

static rt_err_t drv_hrtimer_ctrl(rt_hwtimer_t *timer,
                                 rt_uint32_t cmd, void *args)
{
    struct hrtimer_info *info = _get_hrtimer_priv(timer);

    switch (cmd) {
    case HWTIMER_CTRL_FREQ_SET:
        /* set timer frequence */
        return hal_pwm_set_tb(info->id, *((rt_uint32_t *)args));

    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

static const struct rt_hwtimer_ops drv_hrtimer_ops =
{
    .init  = drv_hrtimer_init,
    .start = drv_hrtimer_start,
    .stop  = drv_hrtimer_stop,
    .control = drv_hrtimer_ctrl,
};

static int drv_hwtimer_init(void)
{
    u32 i;
    rt_err_t ret = RT_EOK;
    struct hrtimer_info *info = g_hrtimer_info;

    for (i = 0; i < ARRAY_SIZE(g_hrtimer_info); i++, info++) {
        info->hrtimer.info = &drv_hrtimer_info;
        info->hrtimer.ops  = &drv_hrtimer_ops;
        ret = rt_device_hwtimer_register(&info->hrtimer, info->name, info);
        if (ret == RT_EOK)
            LOG_D("%s register success", info->name);
        else
            LOG_E("%s register failed", info->name);
    }

    hal_pwm_init();
    aicos_request_irq(PWM_IRQn, drv_hrtimer_irq, 0, NULL, NULL);

    return ret;
}
INIT_DEVICE_EXPORT(drv_hwtimer_init);

