/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include <string.h>
#include <drivers/rt_drv_pwm.h>

#define LOG_TAG         "XPWM"
#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_xpwm.h"

#define FIFO_DEF_PRD 1000000
#define FIFO_DEF_CMP 500000

struct aic_xpwm {
    struct rt_device_pwm rtdev;
    struct aic_xpwm_fifo fifo_para[AIC_XPWM_CH_NUM];
    struct aic_xpwm_updt updt_para[AIC_XPWM_CH_NUM];;
    rt_bool_t xpwm_clk_pm_flag[AIC_XPWM_CH_NUM];
};

extern struct aic_xpwm_arg xpwm_pdata[];
extern const int xpwm_pdata_size;

static struct aic_xpwm g_aic_xpwm = {0};

#ifdef AIC_USING_DMA
static void xpwm_dma_callback(void *arg)
{
    struct aic_xpwm_arg *p = (struct aic_xpwm_arg *)arg;
    hal_dma_chan_stop(p->t_info.dma_chan);
    hal_release_dma_chan(p->t_info.dma_chan);

    /* indicate to upper layer application */
    if (g_aic_xpwm.rtdev.parent.tx_complete != RT_NULL)
        g_aic_xpwm.rtdev.parent.tx_complete(&g_aic_xpwm.rtdev.parent, &p->id);
}
#endif

static rt_err_t drv_xpwm_enable(struct rt_device_pwm *device,
                                struct rt_pwm_configuration *cfg,
                                rt_bool_t enable)
{
    if (enable)
        return !hal_xpwm_enable(cfg->channel) ? RT_EOK : -RT_ERROR;
    else
        return !hal_xpwm_disable(cfg->channel) ? RT_EOK : -RT_ERROR;
}

static rt_err_t drv_xpwm_set(struct rt_device_pwm *device,
                             struct rt_pwm_configuration *cfg)
{
    struct aic_xpwm *xpwm_dev = (struct aic_xpwm *)device;
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM(cfg->channel < AIC_XPWM_CH_NUM, -RT_EINVAL);

    arg = &g_xpwm_args[cfg->channel];

    if (arg->xpwm_int.reg_updt_int_en == 1) {
        xpwm_dev->updt_para[cfg->channel].ch = cfg->channel;
        xpwm_dev->updt_para[cfg->channel].pul_cmp = cfg->pulse;
        xpwm_dev->updt_para[cfg->channel].pul_prd = cfg->period;
        xpwm_dev->updt_para[cfg->channel].pul_num = cfg->pulse_cnt;
    }

    if (hal_xpwm_set(cfg->channel, cfg->pulse, cfg->period, cfg->pulse_cnt))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t drv_xpwm_get(struct rt_device_pwm *device,
                             struct rt_pwm_configuration *cfg)
{
    if (hal_xpwm_get(cfg->channel, (u32 *)&cfg->pulse, (u32 *)&cfg->period))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t drv_xpwm_set_fifo_num(struct rt_device_pwm *device,
                    struct rt_pwm_configuration *cfg)
{
    struct aic_xpwm *xpwm_dev = (struct aic_xpwm *)device;
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM(cfg->channel < AIC_XPWM_CH_NUM, -RT_EINVAL);

    arg = &g_xpwm_args[cfg->channel];

    if (!hal_ch_is_xpwm(cfg->channel)) {
        LOG_E("ch%d is not xpwm channel!\n", cfg->channel);
        return -RT_EINVAL;
    }

    if (arg->fifo_en == 0) {
        LOG_E("ch%d is not fifo mode!\n", cfg->channel);
        return -RT_EINVAL;
    }

    /* fifo normal interrupt mode */
    if (arg->xpwm_int.fifo_aval_int_en == 1) {
        if((cfg->fifo_num > XPWM_FIFO_MAX) || (cfg->fifo_num < XPWM_FIFO_MIN)) {
            LOG_E("Invalid fifo nums(%d-%d): %d!\n", XPWM_FIFO_MIN, XPWM_FIFO_MAX, cfg->fifo_num);
            return -RT_ERROR;
        }

        /* disable the interrupt */
        arg->xpwm_int.fifo_aval_int_en = 0;
        hal_xpwm_irq_en_set(cfg->channel, &arg->xpwm_int);

        xpwm_dev->fifo_para[cfg->channel].fifo_index = 0;
        xpwm_dev->fifo_para[cfg->channel].fifo_num = cfg->fifo_num;

        /* enable the interrupt */
        arg->xpwm_int.fifo_aval_int_en = 1;
        hal_xpwm_irq_en_set(cfg->channel, &arg->xpwm_int);

        hal_xpwm_resume(cfg->channel);
    }

    return RT_EOK;
}

static rt_err_t drv_xpwm_set_fifo(struct rt_device_pwm *device,
                    struct rt_pwm_configuration *cfg)
{
    struct aic_xpwm *xpwm_dev = (struct aic_xpwm *)device;
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM(cfg->channel < AIC_XPWM_CH_NUM, -RT_EINVAL);

    arg = &g_xpwm_args[cfg->channel];

    if (!hal_ch_is_xpwm(cfg->channel)) {
        LOG_E("ch%d is not xpwm channel!\n", cfg->channel);
        return -RT_EINVAL;
    }

    if (arg->fifo_en == 0) {
        LOG_E("ch%d is not fifo mode!\n", cfg->channel);
        return -RT_EINVAL;
    }

    if (arg->xpwm_int.fifo_aval_int_en == 0) {
        LOG_E("ch%d is not normal fifo mode!\n", cfg->channel);
        return -RT_EINVAL;
    }

    if((cfg->fifo_index > (XPWM_FIFO_MAX - 1)) || (cfg->fifo_index < (XPWM_FIFO_MIN - 1))) {
        LOG_E("Invalid fifo index(%d-%d): %d!\n", XPWM_FIFO_MIN - 1, XPWM_FIFO_MAX - 1, cfg->fifo_index);
        return -RT_ERROR;
    }

    /* disable the interrupt */
    arg->xpwm_int.fifo_aval_int_en = 0;
    hal_xpwm_irq_en_set(cfg->channel, &arg->xpwm_int);

    xpwm_dev->fifo_para[cfg->channel].fifo_index = 0;
    xpwm_dev->fifo_para[cfg->channel].pul_prd[cfg->fifo_index] = cfg->pul_prd;
    xpwm_dev->fifo_para[cfg->channel].pul_cmp[cfg->fifo_index] = cfg->pul_cmp;
    xpwm_dev->fifo_para[cfg->channel].pul_num[cfg->fifo_index] = cfg->pul_num;

    /* enable the interrupt */
    arg->xpwm_int.fifo_aval_int_en = 1;
    hal_xpwm_irq_en_set(cfg->channel, &arg->xpwm_int);

    hal_xpwm_resume(cfg->channel);

    return RT_EOK;
}

#ifdef AIC_USING_DMA
static rt_err_t drv_xpwm_dma_set_fifo(struct rt_device_pwm *device,
                    struct rt_pwm_configuration *cfg)
{
    struct aic_xpwm_arg *arg = NULL;
    int ret = 0;

    CHECK_PARAM(cfg->channel < AIC_XPWM_CH_NUM, -RT_EINVAL);

    arg = &g_xpwm_args[cfg->channel];

    if (!hal_ch_is_xpwm(cfg->channel)) {
        LOG_E("ch%d is not xpwm channel!\n", cfg->channel);
        return -RT_EINVAL;
    }

    if (arg->fifo_en == 0) {
        LOG_E("ch%d is not fifo mode!\n", cfg->channel);
        return -RT_EINVAL;
    }

    if (arg->dma_en == 0) {
        LOG_E("ch%d is not dma fifo mode!\n", cfg->channel);
        return -RT_EINVAL;
    }

    arg->t_info.buf_info.buf = (u32 *)cfg->buf;
    arg->t_info.buf_info.buf_len = cfg->buf_len;

    for (int i = 0; i < arg->t_info.buf_info.buf_len / sizeof(u32); i += 3) {
        xpwm_cal_prd_duty(cfg->channel, arg->t_info.buf_info.buf[i + 1], arg->t_info.buf_info.buf[i]);
        arg->t_info.buf_info.buf[i] = arg->period;
        arg->t_info.buf_info.buf[i + 1] = arg->duty;
    }

    ret = hal_xpwm_dma_config(cfg->channel, xpwm_dma_callback, arg);
    if (ret < 0) {
        LOG_E("ch%d dma config err!", cfg->channel);
        return -RT_EINVAL;
    }

    return RT_EOK;
}
#endif

static rt_err_t drv_xpwm_get_fifo(struct rt_device_pwm *device,
                    struct rt_pwm_configuration *cfg)
{
    struct aic_xpwm *xpwm_dev = (struct aic_xpwm *)device;
    struct aic_xpwm_arg *arg = NULL;
    int i;

    CHECK_PARAM(cfg->channel < AIC_XPWM_CH_NUM, -RT_EINVAL);

    arg = &g_xpwm_args[cfg->channel];

    if (!hal_ch_is_xpwm(cfg->channel)) {
        LOG_E("ch%d is not xpwm channel!\n", cfg->channel);
        return -RT_EINVAL;
    }

    if (arg->fifo_en == 0) {
        LOG_E("ch%d is not fifo mode!\n", cfg->channel);
        return -RT_EINVAL;
    }

    if (arg->xpwm_int.fifo_aval_int_en == 0) {
        LOG_E("ch%d is not normal fifo mode!\n", cfg->channel);
        return -RT_EINVAL;
    }

    LOG_I("ch%d fifo valid count: %d\n", cfg->channel, xpwm_dev->fifo_para[cfg->channel].fifo_num);

    for (i = 0; i < xpwm_dev->fifo_para[cfg->channel].fifo_num; i++)
        LOG_I("fifo_idx %d: prd:%d pulse:%d pulse cnt:%d\n", i, xpwm_dev->fifo_para[cfg->channel].pul_prd[i],
            xpwm_dev->fifo_para[cfg->channel].pul_cmp[i], xpwm_dev->fifo_para[cfg->channel].pul_num[i]);

    return RT_EOK;
}

static rt_err_t drv_xpwm_voltage_set(struct rt_device_pwm *device,
                    struct rt_pwm_configuration *cfg)
{
    CHECK_PARAM(cfg->channel < AIC_XPWM_CH_NUM, -RT_EINVAL);

    if (hal_xpwm_voltage_set(cfg->channel, cfg->vol) < 0) {
        LOG_E("voltage:%d mV set failed on ch:%d!\n", cfg->vol, cfg->channel);
        return -RT_EINVAL;
    }

    return RT_EOK;
}

static rt_err_t drv_xpwm_control(struct rt_device_pwm *device,
                                int cmd, void *arg)
{
    struct rt_pwm_configuration *cfg = (struct rt_pwm_configuration *)arg;

    switch (cmd) {
    case PWM_CMD_ENABLE:
        return drv_xpwm_enable(device, cfg, RT_TRUE);
    case PWM_CMD_DISABLE:
        return drv_xpwm_enable(device, cfg, RT_FALSE);
    case PWM_CMD_SET:
        return drv_xpwm_set(device, cfg);
    case PWM_CMD_GET:
        return drv_xpwm_get(device, cfg);
    case PWM_CMD_SET_FIFO_NUM:
        return drv_xpwm_set_fifo_num(device, cfg);
    case PWM_CMD_SET_FIFO:
        return drv_xpwm_set_fifo(device, cfg);
#ifdef AIC_USING_DMA
    case PWM_CMD_DMA_SET_FIFO:
        return drv_xpwm_dma_set_fifo(device, cfg);
#endif
    case PWM_CMD_GET_FIFO:
        return drv_xpwm_get_fifo(device, cfg);
    case PWM_CMD_VDD_REGU:
        return drv_xpwm_voltage_set(device, cfg);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

static struct rt_pwm_ops aic_xpwm_ops = {
    .control = drv_xpwm_control
};

irqreturn_t aic_xpwm_irq(int irq, void *arg)
{
    struct aic_xpwm_arg *parg;
    struct aic_xpwm_fifo *fifo_para = (struct aic_xpwm_fifo *)arg;
    struct aic_xpwm_updt *updt_para = (struct aic_xpwm_updt *)arg;
    u32 *pch = (u32 *)arg;

    u32 stat = 0;
    u32 ch = (u32)*pch;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, IRQ_NONE);

    parg = &g_xpwm_args[ch];

    stat = hal_xpwm_int_stat(ch);

    if (parg->xpwm_int.fifo_aval_int_en == 1) {
        if((stat & (1 << FIFO_AVAL_FLG_SHIFT))) {
            hal_xpwm_int_clr(fifo_para->ch, (1 << FIFO_AVAL_FLG_SHIFT));
            hal_xpwm_set_fifo(fifo_para->ch, fifo_para->pul_num[fifo_para->fifo_index],
                fifo_para->pul_prd[fifo_para->fifo_index], fifo_para->pul_cmp[fifo_para->fifo_index]);

            fifo_para->fifo_index ++;
            if (fifo_para->fifo_index == fifo_para->fifo_num)
                fifo_para->fifo_index = 0;
        }
    } else if (parg->xpwm_int.reg_updt_int_en == 1) {
        if ((stat & (1 << REG_UPDT_FLG_SHIFT))) {
            hal_xpwm_int_clr(updt_para->ch, (1 << REG_UPDT_FLG_SHIFT));
            hal_xpwm_set(updt_para->ch, updt_para->pul_cmp, updt_para->pul_prd, updt_para->pul_num);
        }
    } else {
        return IRQ_NONE;
    }

    return IRQ_HANDLED;
}

static int drv_xpwm_clk_freq_init(void)
{
    int i = 0;
#ifdef AIC_XPWM_DRV_V10
    int pwm_clk_rate = 0, xpwm_clk_rate = 0;

    if (hal_clk_set_freq(CLK_PWM_SDFM, XPWM_CLK_RATE) < 0) {
        pr_err("Failed to set PWM clk %d\n", XPWM_CLK_RATE);
        return -RT_ERROR;
    }
    if (hal_clk_set_freq(CLK_XPWM_SDFM, XPWM_CLK_RATE) < 0) {
        pr_err("Failed to set XPWM clk %d\n", XPWM_CLK_RATE);
        return -RT_ERROR;
    }
    pwm_clk_rate = hal_clk_get_freq(CLK_PWM_SDFM);
    if (pwm_clk_rate < 0) {
        pr_err("Failed to get PWM clk.\n");
        return -RT_ERROR;
    }
    xpwm_clk_rate = hal_clk_get_freq(CLK_XPWM_SDFM);
    if (xpwm_clk_rate < 0) {
        pr_err("Failed to get XPWM clk.\n");
        return -RT_ERROR;
    }
    if (pwm_clk_rate != xpwm_clk_rate) {
        pr_err("please check the clk.\n");
        return -RT_ERROR;
    }
#endif
    for (i = 0; i < xpwm_pdata_size; i++) {
#ifdef AIC_XPWM_DRV_V10
        xpwm_pdata[i].act_clk_rate = xpwm_clk_rate;
#endif
#ifdef AIC_XPWM_DRV_V11
        xpwm_pdata[i].act_clk_rate = hal_clk_get_freq(xpwm_pdata[i].clk);
        if (xpwm_pdata[i].act_clk_rate < 0) {
            pr_err("Failed to get XPWM clk.\n");
            return -RT_ERROR;
        }
#endif
    }
    return 0;
}
static void drv_xpwm_channels_init(void)
{
    int i, j;
    u32 xpwm_ch = 0;

    for (i = 0; i < xpwm_pdata_size; i++) {
        hal_xpwm_ch_init(&xpwm_pdata[i]);
        xpwm_ch = xpwm_pdata[i].id;

        if (hal_ch_is_xpwm(xpwm_ch) == 1) {
            if (xpwm_pdata[i].xpwm_int.fifo_aval_int_en == 1) {
                g_aic_xpwm.fifo_para[xpwm_ch].ch = xpwm_ch;
                g_aic_xpwm.fifo_para[xpwm_ch].fifo_num = XPWM_FIFO_MAX;
                for (j = 0; j < XPWM_FIFO_MAX; j++) {
                    g_aic_xpwm.fifo_para[xpwm_ch].pul_num[j] = 1;
                    g_aic_xpwm.fifo_para[xpwm_ch].pul_prd[j] = FIFO_DEF_PRD;
                    g_aic_xpwm.fifo_para[xpwm_ch].pul_cmp[j] = FIFO_DEF_CMP;
                }
                aicos_request_irq(xpwm_pdata[i].irq, aic_xpwm_irq, 0, NULL, &g_aic_xpwm.fifo_para[xpwm_ch]);
            } else if (xpwm_pdata[i].xpwm_int.reg_updt_int_en == 1) {
                aicos_request_irq(xpwm_pdata[i].irq, aic_xpwm_irq, 0, NULL, &g_aic_xpwm.updt_para[xpwm_ch]);
            } else {
                LOG_D("ch:%d Normal mode does not require registration interruption.\n", xpwm_ch);
            }
        }
    }

}

#ifdef RT_USING_PM
static int aic_xpwm_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_xpwm *xpwm_dev = (struct aic_xpwm *)device;

    int i;
    switch (mode) {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_STANDBY:
    case PM_SLEEP_MODE_DEEP:
        for (i = 0; i < xpwm_pdata_size; i++) {
            if (hal_clk_is_enabled(xpwm_pdata[i].clk)) {
                hal_clk_disable_assertrst(xpwm_pdata[i].clk);
                xpwm_dev->xpwm_clk_pm_flag[xpwm_pdata[i].id] = RT_TRUE;
            }
        }
        break;
    default:
        break;
    }
    return 0;
}

static void aic_xpwm_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_xpwm *xpwm_dev = (struct aic_xpwm *)device;
    int i;
    switch (mode) {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_STANDBY:
    case PM_SLEEP_MODE_DEEP:
        drv_xpwm_clk_freq_init();
        for (i = 0; i < xpwm_pdata_size; i++) {
            if (xpwm_dev->xpwm_clk_pm_flag[xpwm_pdata[i].id] && !hal_clk_is_enabled(xpwm_pdata[i].clk)) {
                hal_clk_enable_deassertrst(xpwm_pdata[i].clk);
                if ((xpwm_pdata[i].xpwm_mode == XPWM_PWM_MODE) || (xpwm_pdata[i].xpwm_mode == XPWM_REG_UPDT_MODE))
                    hal_xpwm_pul_reset(xpwm_pdata[i].id);
                if (xpwm_pdata[i].xpwm_mode != XPWM_REG_UPDT_MODE)
                    hal_xpwm_resume(xpwm_pdata[i].id);
            }
            if (xpwm_dev->xpwm_clk_pm_flag[xpwm_pdata[i].id])
                xpwm_dev->xpwm_clk_pm_flag[xpwm_pdata[i].id] = RT_FALSE;
        }
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_xpwm_pm_ops = {
    SET_DEVICE_PM_OPS(aic_xpwm_suspend, aic_xpwm_resume)
    NULL,
};
#endif

int drv_xpwm_init(void)
{
    if(drv_xpwm_clk_freq_init())
        return -RT_ERROR;

    drv_xpwm_channels_init();

    if (rt_device_pwm_register(&g_aic_xpwm.rtdev, "xpwm", &aic_xpwm_ops, NULL))
        return -RT_ERROR;

#ifdef RT_USING_PM
    rt_pm_device_register(&g_aic_xpwm.rtdev.parent, &aic_xpwm_pm_ops);
#endif

    LOG_I("ArtInChip XPWM loaded");
    return RT_EOK;
}
INIT_DEVICE_EXPORT(drv_xpwm_init);
