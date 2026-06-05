/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <string.h>
#include "aic_core.h"
#include "aic_hal_clk.h"
#include "hal_xpwm.h"

#define AIC_XPWM_NAME              "aic-xpwm"

#define PWM_CONF                    0x000
#define PWM_CH_CONF                 0x004
#define XPWM_STS_FLAG               0x008
#define XPWM_INT_EN                 0x00C
#define XPWM_FIFO_FLUSH             0x010
#define XPWM_RESUME                 0x014
#define PUL_THR_RS                  0x018
#define XPWM_FIFO                   0x020
#define XPWM_PUL_STA                0x024
#define XPWM_CNT_STA                0x028
#define XPWM_PRDV                   0x050
#define PUL_CNT                     0x054
#define PUL_THR                     0x058
#define XPWM_CMPV                   0x060
#define XPWM_VER                    0x0FC

/* PWM_CONF */
#define PWM_CONF_CLKDIV_MAX         0x3FF
#define PWM_CONF_CLK_DIV_SHIFT      16
#define PWM_CONF_CLK_DIV_MASK       GENMASK(25,16)
#define PWM_CONF_XPWM_CNT_EN_SHIFT  0
#define PWM_CONF_XPWM_MOD_SHIFT     1
#define XPWM_CNT_EN                 BIT(0)
#define XPWM_MOD                    BIT(1)
#define XPWM_UPDATE_MODE            BIT(2)
#define XPWM_FIFO_EN                BIT(4)
#define PUL_STA_EN                  BIT(5)
#define XPWM_DMA_EN                 BIT(6)
#define PUL_LIMIT_EN                BIT(7)
#define PWM_CONF_XPWM_FIFO_TH_SHIFT 8
#define PWM_CONF_XPWM_FIFO_TH_MASK  GENMASK(15,8)

/* PWM_CH_CONF */
#define XPWM_IMD_UPDT               BIT(0)
#define XPWM_INV_EN                 BIT(1)
#define XPWM_IDLE                   BIT(2)

/* XPWM_INT_EN */
#define XPWM_INT_EN_MASK            0x1FF
#define CNT_ZRO_INT_EN_SHIFT        0
#define CNT_PRD_INT_EN_SHIFT        1
#define REG_UPDT_INT_EN_SHIFT       2
#define FIFO_AVAL_INT_EN_SHIFT      3
#define FIFO_UDFL_INT_EN_SHIFT      4
#define FIFO_OVFL_INT_EN_SHIFT      5
#define XPWM_FRC_PUL_INT_EN_SHIFT   6
#define XPWM_PUL_LIMIT_INT_EN_SHIFT 7
#define CNT_CMP_INT_EN_SHIFT        8

#define XPWM_PRD_MAX                0xFFFFFFFF

#define XPWM_REG_ADDR(arg, offset) ((volatile void *)(uintptr_t)((arg)->base + (offset)))

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC                1000000000
#endif

struct aic_xpwm_arg g_xpwm_args[AIC_XPWM_CH_NUM] = {{0}};

int xpwm_cal_prd_duty(u32 ch, u32 duty_ns, u32 period_ns)
{
    struct aic_xpwm_arg *arg = NULL;
    u32 prd = 0;
    u64 duty = 0;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    arg = &g_xpwm_args[ch];

    arg->freq = (float)NSEC_PER_SEC / period_ns;
    prd = arg->tb_clk_rate / arg->freq;

    if (prd > XPWM_PRD_MAX) {
        hal_log_err("ch%d period %d is too big\n", ch, prd);
        return -ERANGE;
    }
    arg->period = prd;

    duty = (u64)duty_ns * (u64)prd;
    do_div(duty, period_ns);

    arg->duty = (u32)duty;

    return 0;
}

u32 hal_xpwm_int_stat(u32 ch)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_RET_WITH_RET(ch < AIC_XPWM_CH_NUM, 0);

    arg = &g_xpwm_args[ch];

    return readl(XPWM_REG_ADDR(arg, XPWM_STS_FLAG));
}

void hal_xpwm_int_clr(u32 ch, u32 mask)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM_RET(ch < AIC_XPWM_CH_NUM);

    arg = &g_xpwm_args[ch];

    writel(mask, XPWM_REG_ADDR(arg, XPWM_STS_FLAG));
}

void hal_xpwm_resume(u32 ch)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM_RET(ch < AIC_XPWM_CH_NUM);

    arg = &g_xpwm_args[ch];

    writel(0x1, XPWM_REG_ADDR(arg, XPWM_RESUME));
}

void hal_xpwm_fifo_flush(u32 ch)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM_RET(ch < AIC_XPWM_CH_NUM);

    arg = &g_xpwm_args[ch];

    writel(0x1, XPWM_REG_ADDR(arg, XPWM_FIFO_FLUSH));
}

int hal_xpwm_set_fifo(u32 ch, u32 pul_num, u32 pul_prd, u32 pul_cmp)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    arg = &g_xpwm_args[ch];

    if (xpwm_cal_prd_duty(ch, pul_cmp, pul_prd))
        return -ERANGE;

    writel(arg->period, XPWM_REG_ADDR(arg, XPWM_FIFO));
    writel(arg->duty, XPWM_REG_ADDR(arg, XPWM_FIFO));
    writel(pul_num, XPWM_REG_ADDR(arg, XPWM_FIFO));

    return 0;
}

static void xpwm_reg_enable(u32 ch, int offset, int bit, int enable)
{
    struct aic_xpwm_arg *arg = NULL;
    int tmp;

    CHECK_PARAM_RET(ch < AIC_XPWM_CH_NUM);

    arg = &g_xpwm_args[ch];

    tmp = readl(XPWM_REG_ADDR(arg, offset));
    tmp &= ~bit;
    if (enable)
        tmp |= bit;

    writel(tmp, XPWM_REG_ADDR(arg, offset));
}

bool hal_ch_is_xpwm(u32 ch)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_RET_WITH_RET(ch < AIC_XPWM_CH_NUM, 0);

    arg = &g_xpwm_args[ch];

    if (arg->xpwm_mode == 0) {
        return 0;
    }
    return 1;
}

void hal_xpwm_imd_update(u32 ch, u32 enable)
{
    CHECK_PARAM_RET(ch < AIC_XPWM_CH_NUM);

    if (hal_ch_is_xpwm(ch))
        return;

    xpwm_reg_enable(ch, PWM_CH_CONF, XPWM_IMD_UPDT, enable);
}

void hal_xpwm_ch_init(struct aic_xpwm_arg *data)
{
    struct aic_xpwm_arg *arg = NULL;
    u8 ch = 0;

    CHECK_PARAM_RET(data != NULL && data->id < AIC_XPWM_CH_NUM);

    ch = data->id;
    arg = &g_xpwm_args[ch];
    data->available = 1;

    switch (data->xpwm_mode) {
        case XPWM_PWM_MODE:
        case XPWM_NORMAL_MODE:
            hal_log_debug("ch:%d mode %d, no additional configuration required.\n", ch, data->xpwm_mode);
            break;
        case XPWM_REG_UPDT_MODE:
            data->xpwm_int.reg_updt_int_en = 0x1;
            break;
        case XPWM_FIFO_NORMAL_MODE:
            data->xpwm_int.fifo_aval_int_en = 0x1;
            break;
        case XPWM_FIFO_DMA_MODE:
            data->dma_en = 0x1;
            break;
        default:
            hal_log_err("ch:%d unknown mode %d.\n", ch, data->xpwm_mode);
            break;
    }

    if ((data->xpwm_mode == XPWM_FIFO_NORMAL_MODE) || (data->xpwm_mode == XPWM_FIFO_DMA_MODE)) {
        data->fifo_en = 1;
        data->fifo_th = XPWM_FIFO_TH_DEFAULT_VALUE;
    }

    memcpy(arg, data, sizeof(struct aic_xpwm_arg));
}

int hal_xpwm_is_enable(u32 ch)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_RET_WITH_RET(ch < AIC_XPWM_CH_NUM, 0);

    arg = &g_xpwm_args[ch];

    return readl(XPWM_REG_ADDR(arg, PWM_CONF)) & XPWM_CNT_EN;
}

static int xpwm_clk_enable(u32 ch)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    arg = &g_xpwm_args[ch];

    if (!hal_clk_is_enabled(arg->clk)) {
        if (hal_clk_enable_deassertrst(arg->clk) < 0) {
            hal_log_err("Failed to reset XPWM deassert\n");
            return -EINVAL;
        }
    }

    return 0;
}

int hal_xpwm_pul_reset(u32 ch)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    arg = &g_xpwm_args[ch];

    /* check if the clock needs needs to be enabled */
    xpwm_clk_enable(ch);

    if (arg->period != 0)
        writel(arg->period, XPWM_REG_ADDR(arg, XPWM_PRDV));

    if (arg->duty != 0)
        writel(arg->duty, XPWM_REG_ADDR(arg, XPWM_CMPV));

    if (hal_ch_is_xpwm(ch)) {
        if (arg->pulse_cnt != 0)
            writel(arg->pulse_cnt, XPWM_REG_ADDR(arg, PUL_CNT));
    }
    return 0;
}

int hal_xpwm_set(u32 ch, u32 duty_ns, u32 period_ns, u32 pulse_cnt)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    arg = &g_xpwm_args[ch];

    if (period_ns < 1) {
        hal_log_err("ch%d invalid period %d\n", ch, period_ns);
        return -ERANGE;
    }

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    /* check if the clock needs needs to be enabled */
    xpwm_clk_enable(ch);

    if (xpwm_cal_prd_duty(ch, duty_ns, period_ns))
        return -ERANGE;

    writel(arg->period, XPWM_REG_ADDR(arg, XPWM_PRDV));
    writel(arg->duty, XPWM_REG_ADDR(arg, XPWM_CMPV));

    if (hal_ch_is_xpwm(ch)) {
        writel(pulse_cnt, XPWM_REG_ADDR(arg, PUL_CNT));
        arg->pulse_cnt = pulse_cnt;
    }
    if (hal_xpwm_is_enable(ch)) {
        if (hal_ch_is_xpwm(ch)) {
            if (arg->xpwm_int.reg_updt_int_en != 0x1)
                hal_xpwm_resume(ch);
        }
    }
    return 0;
}

int hal_xpwm_set_direct(u32 ch, u32 cmp, u32 prd, u32 pulse_cnt)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    arg = &g_xpwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    /* check if the clock needs needs to be enabled */
    xpwm_clk_enable(ch);

    arg->period = prd;
    arg->duty = cmp;

    writel(arg->period, XPWM_REG_ADDR(arg, XPWM_PRDV));
    writel(arg->duty, XPWM_REG_ADDR(arg, XPWM_CMPV));

    if (hal_ch_is_xpwm(ch))
        writel(pulse_cnt, XPWM_REG_ADDR(arg, PUL_CNT));

    if (hal_xpwm_is_enable(ch)) {
        if (hal_ch_is_xpwm(ch)) {
            if (arg->xpwm_int.reg_updt_int_en != 0x1)
                hal_xpwm_resume(ch);
        }
    }
    return 0;
}


int hal_xpwm_get(u32 ch, u32 *duty_ns, u32 *period_ns)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    arg = &g_xpwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    *duty_ns   = arg->duty;
    *period_ns = arg->period;
    return 0;
}

static void xpwm_ch_config(u32 ch, u32 action, enum xpwm_polarity polarity)
{
    struct aic_xpwm_arg *arg = NULL;
    u32 val;

    CHECK_PARAM_RET(ch < AIC_XPWM_CH_NUM);

    arg = &g_xpwm_args[ch];

    val = readl(XPWM_REG_ADDR(arg, PWM_CH_CONF));
    val &= ~(XPWM_IDLE | XPWM_INV_EN);
    val |= action | polarity;
    writel(val, XPWM_REG_ADDR(arg, PWM_CH_CONF));
}

void hal_xpwm_irq_en_set(u32 ch, struct xpwm_int_s *xpwm_int)
{
    struct aic_xpwm_arg *arg = NULL;
    u32 val;

    CHECK_PARAM_RET(ch < AIC_XPWM_CH_NUM);

    arg = &g_xpwm_args[ch];

    val = readl(XPWM_REG_ADDR(arg, XPWM_INT_EN));

    val &= ~XPWM_INT_EN_MASK;

    val |= (xpwm_int->cnt_zro_int_en << CNT_ZRO_INT_EN_SHIFT)
        | (xpwm_int->cnt_prd_int_en << CNT_PRD_INT_EN_SHIFT)
        | (xpwm_int->reg_updt_int_en << REG_UPDT_INT_EN_SHIFT)
        | (xpwm_int->fifo_aval_int_en << FIFO_AVAL_INT_EN_SHIFT)
        | (xpwm_int->fifo_udfl_int_en << FIFO_UDFL_INT_EN_SHIFT)
        | (xpwm_int->fifo_ovfl_int_en << FIFO_OVFL_INT_EN_SHIFT)
        | (xpwm_int->frc_pul_int_en << XPWM_FRC_PUL_INT_EN_SHIFT)
        | (xpwm_int->pul_limit_int_en << XPWM_PUL_LIMIT_INT_EN_SHIFT)
        | (xpwm_int->cnt_cmp_int_en << CNT_CMP_INT_EN_SHIFT);

    writel(val, XPWM_REG_ADDR(arg, XPWM_INT_EN));
}

void hal_xpwm_cnt_en(u32 ch, u32 val)
{
    CHECK_PARAM_RET(ch < AIC_XPWM_CH_NUM);

    xpwm_reg_enable(ch, PWM_CONF, XPWM_CNT_EN, val);
}

int hal_xpwm_enable(u32 ch)
{
    struct aic_xpwm_arg *arg = NULL;
    enum xpwm_polarity polarity;
    u32 div = 0, action = 0;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    arg = &g_xpwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    /* check if the clock needs to be enabled */
    xpwm_clk_enable(ch);

    hal_log_debug("ch%d enable\n", ch);
    if (arg->act_clk_rate == arg->tb_clk_rate)
        div = 0;
    else
        div = arg->act_clk_rate / arg->tb_clk_rate - 1;
    hal_log_debug("XPWM_CLK_RATE:%d arg->tb_clk_rate:%d, div:%d\n", arg->act_clk_rate, arg->tb_clk_rate, div);
    if (div > PWM_CONF_CLKDIV_MAX) {
        hal_log_err("ch%d clkdiv %d is too big\n", ch, div);
        return -ERANGE;
    }
    hal_log_debug("ch base addr 0x%08X\n", arg->base);
    writel_bits(div, PWM_CONF_CLK_DIV_MASK, PWM_CONF_CLK_DIV_SHIFT, XPWM_REG_ADDR(arg, PWM_CONF));

    action = arg->def_level ? XPWM_IDLE : 0;
    polarity = arg->polarity ? XPWM_INV_EN : 0;
    xpwm_ch_config(ch, action, polarity);

    if (hal_ch_is_xpwm(ch)) {
        if (((arg->fifo_en == 1) && (arg->dma_en == 0))
            || ((arg->fifo_en == 0) && (arg->xpwm_int.reg_updt_int_en == 0)))
            xpwm_reg_enable(ch, PWM_CONF, XPWM_UPDATE_MODE, 1);

        xpwm_reg_enable(ch, PWM_CONF, XPWM_MOD, 1);//XPWM
        xpwm_reg_enable(ch, PWM_CONF, XPWM_FIFO_EN, arg->fifo_en);
        xpwm_reg_enable(ch, PWM_CONF, XPWM_DMA_EN, arg->dma_en);
        xpwm_reg_enable(ch, PWM_CONF, PUL_LIMIT_EN, arg->pul_limit_en);
        writel_bits(arg->fifo_th, PWM_CONF_XPWM_FIFO_TH_MASK, PWM_CONF_XPWM_FIFO_TH_SHIFT, XPWM_REG_ADDR(arg, PWM_CONF));
        hal_xpwm_irq_en_set(ch, &arg->xpwm_int);
    } else {
        xpwm_reg_enable(ch, PWM_CONF, XPWM_MOD, 0);//PWM
    }
    hal_xpwm_cnt_en(ch, 1);

    return 0;
}

int hal_xpwm_disable(u32 ch)
{
    struct aic_xpwm_arg *arg = NULL;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    arg = &g_xpwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    hal_log_debug("ch%d disable\n", ch);
    hal_xpwm_cnt_en(ch, 0);
    return 0;
}

int hal_xpwm_voltage_set(u32 ch, u32 vol)
{
#define VOL_MAX     (1285)
#define VOL_MIN     (816)
#define VOL_ERR     (VOL_MAX - VOL_MIN)

    struct aic_xpwm_arg *arg = NULL;
    u32 freq_src = 0, freq_pwm = 0;
    u32 cmp = 0, prd = 0;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    CHECK_PARAM(vol > VOL_MIN && vol < VOL_MAX, -EINVAL);

    arg = &g_xpwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable.\n", ch);
        return -EINVAL;
    }

    if (hal_ch_is_xpwm(ch)) {
        hal_log_err("ch%d is not pwm mode, please check the configuration.\n", ch);
        return -EINVAL;
    }

    if (hal_xpwm_enable(ch) < 0) {
        hal_log_err("enable xpwm%d failed!\n", ch);
        return -EINVAL;
    }

    freq_src = arg->tb_clk_rate;
    freq_pwm = freq_src / VOL_ERR;
    prd = freq_src / freq_pwm;

    cmp = prd * (VOL_MAX - vol) / VOL_ERR;

    hal_log_debug("ch:%d input vol:%d mV,tb_clk:%d Hz,prd:%d, cmp:%d\n", ch, vol, arg->tb_clk_rate, prd, cmp);

    hal_xpwm_set_direct(ch, cmp, prd, 0);

    return 0;
}

#ifdef AIC_USING_DMA
int hal_xpwm_dma_config(u32 ch, dma_async_callback callback, void *callback_param)
{
    struct dma_slave_config config;
    struct aic_xpwm_transfer_info *info;
    struct aic_xpwm_arg *arg = NULL;
    int ret = 0;

    CHECK_PARAM(ch < AIC_XPWM_CH_NUM, -EINVAL);

    arg = &g_xpwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -1;
    }

    info = &arg->t_info;
    config.direction = DMA_MEM_TO_DEV;
    config.src_maxburst = 16;
    config.dst_maxburst = 1;
    config.src_addr_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
    config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    config.dst_addr = (ulong)XPWM_REG_ADDR(arg, XPWM_FIFO);
    config.src_addr = (ulong)info->buf_info.buf;
    config.slave_id = arg->dma_id;

    info->dma_chan = hal_request_dma_chan();
    if (!info->dma_chan) {
        hal_log_err("XPWM%d request dma channel error\n", ch);
        return -1;
    }

    ret = hal_dma_chan_config(info->dma_chan, &config);
    if (ret < 0) {
        hal_log_err("XPWM%d dma channel config error\n", ch);
        goto err_dma_config;
    }
    ret = hal_dma_chan_register_cb(info->dma_chan, callback, callback_param);
    if (ret < 0) {
        hal_log_err("XPWM%d dma channel register callback error\n", ch);
        goto err_dma_config;
    }
    ret = hal_dma_chan_prep_device(info->dma_chan, config.dst_addr, config.src_addr,
                    info->buf_info.buf_len, config.direction);
    if (ret < 0) {
        hal_log_err("XPWM%d dma channel prep device error\n", ch);
        goto err_dma_config;
    }
    ret = hal_dma_chan_start(info->dma_chan);
    if (ret < 0) {
        hal_log_err("XPWM%d dma channel start error\n", ch);
        goto err_dma_config;
    }

    return 0;

err_dma_config:
    hal_release_dma_chan(info->dma_chan);

    return ret;
}
#endif

