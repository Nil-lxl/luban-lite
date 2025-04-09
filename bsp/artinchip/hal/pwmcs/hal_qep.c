/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <string.h>

#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_qep.h"

/* Register definition of PWMCS Controller */

#define QEP_BASE(n)                 (PWMCS_BASE + 0x2000 + 0x100 * (n))
#define GLB_BASE_Q                  (PWMCS_BASE + 0xF000)

#if defined (AIC_QEP_DRV_V10)
#define GLB_CLK_CTL                 (GLB_BASE_Q + 0x00)
#define GLB_CLK_CTL_QEP_EN(n)       (BIT(24) << (n))
#endif
#if defined (AIC_QEP_DRV_V11)
#define GLB_CLK_CTL                 (GLB_BASE_Q + 0x28)
#define GLB_CLK_CTL_QEP_EN(n)       BIT(n)
#endif

#define QEP_POS_CNT_V(n)            (QEP_BASE(n) + 0x00)
#define QEP_POS_CNT_SP(n)           (QEP_BASE(n) + 0x04)
#define QEP_POS_CNT_EP(n)           (QEP_BASE(n) + 0x08)
#define QEP_POS_CNT_CMPV(n)         (QEP_BASE(n) + 0x0c)
#define QEP_DEC_CONF(n)             (QEP_BASE(n) + 0x2c)
#define QEP_POS_CNT_CONF(n)         (QEP_BASE(n) + 0x30)
#define QEP_POS_CNTCMP_CONF(n)      (QEP_BASE(n) + 0x38)
#define QEP_INT_EN(n)               (QEP_BASE(n) + 0x3C)
#define QEP_INT_FLG(n)              (QEP_BASE(n) + 0x40)
#define QEP_INT_CLR(n)              (QEP_BASE(n) + 0x44)
#define QEP_IN_CTL(n)               (QEP_BASE(n) + 0x68)

/* QEP_DEC_CONF */
#define QEP_CNT_MODE_MASK           GENMASK(16, 14)
#define QEP_CNT_MODE_SHIFT          14
#define QEP_A_INV_EN_MASK           GENMASK(8, 8)
#define QEP_A_INV_EN_SHIFT          8

/* QEP_POS_CNT_CONF */
#define QEP_POS_CNT_EN_MASK         GENMASK(3, 3)
#define QEP_POS_CNT_EN_SHIFT        3

/* QEP_POS_CNTCMP_CONF */
#define QEP_POS_CMP_EN_MASK         GENMASK(12, 12)
#define QEP_POS_CMP_EN_SHIFT        12

/* QEP_IN_CTL */
#define QEP_IN_FLT_EN_MASK          GENMASK(0, 0)
#define QEP_IN_FLT_EN_SHIFT         0

/* QEP_INT_EN */
#define QEP_POS_CMP_INT_EN_MASK     GENMASK(8, 8)
#define QEP_POS_CMP_INT_EN_SHIFT    8

struct aic_qep_arg {
    u16 available;
    u16 id;
};

static struct aic_qep_arg g_qep_args[AIC_QEP_CH_NUM] = {{0}};
static int g_qep_inited = 0;

static void qep_reg_enable(int addr, int bit, int enable)
{
    int tmp;

    tmp = readl((ulong)addr);
    if (enable)
        tmp |= bit;
    else
        tmp &= ~bit;

    writel(tmp, (ulong)addr);
}

u32 hal_qep_get_cnt(u32 ch)
{
    return readl(QEP_POS_CNT_V(ch));
}

u32 hal_qep_int_stat(u32 ch)
{
    return readl(QEP_INT_FLG(ch));
}

void hal_qep_clr_int(u32 ch, u32 stat)
{
    writel(stat, QEP_INT_CLR(ch));
}

void hal_qep_set_cnt_ep(u32 i, u32 cnt)
{
    writel(cnt, QEP_POS_CNT_EP(i));
}

void hal_qep_set_cnt_cmp(u32 i, u32 cnt)
{
    writel(cnt, QEP_POS_CNT_CMPV(i));
}

void hal_qep_config(u32 ch)
{
    qep_reg_enable(GLB_CLK_CTL, GLB_CLK_CTL_QEP_EN(ch), 1);

    /* qep configuration */
    writel_bits((u32)INCREMENTAL_COUNT, QEP_CNT_MODE_MASK, QEP_CNT_MODE_SHIFT, QEP_DEC_CONF(ch));
    writel_bits(1, QEP_A_INV_EN_MASK, QEP_A_INV_EN_SHIFT, QEP_DEC_CONF(ch));
    writel_bits(1, QEP_POS_CMP_EN_MASK, QEP_POS_CMP_EN_SHIFT, QEP_POS_CNTCMP_CONF(ch));
    writel_bits(1, QEP_IN_FLT_EN_MASK, QEP_IN_FLT_EN_SHIFT, QEP_IN_CTL(ch));
}

void hal_qep_int_enable(u32 ch, u32 enable)
{
    writel_bits(enable, QEP_POS_CMP_INT_EN_MASK, QEP_POS_CMP_INT_EN_SHIFT, QEP_INT_EN(ch));
}

void hal_qep_enable(u32 ch, u32 enable)
{
    writel_bits(enable, QEP_POS_CNT_EN_MASK, QEP_POS_CNT_EN_SHIFT, QEP_POS_CNT_CONF(ch));
}

int hal_qep_init(void)
{
    int i, ret = 0;
    u32 clk_id = 0;

    if (g_qep_inited) {
        hal_log_debug("PWMCS was already inited\n");
        return 0;
    }

#if defined (AIC_QEP_DRV_V10)
    clk_id = CLK_PWMCS;
#endif
#if defined (AIC_QEP_DRV_V11)
    clk_id = CLK_PWMCS_SDFM;
#endif
    ret = hal_clk_set_freq(clk_id, QEP_CLK_RATE);
    if (ret < 0) {
        hal_log_err("Failed to set QEP clk %d\n", QEP_CLK_RATE);
        return -1;
    }
    ret = hal_clk_enable(CLK_PWMCS);
    if (ret < 0) {
        hal_log_err("Failed to enable QEP clk\n");
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_PWMCS);
    if (ret < 0) {
        hal_log_err("Failed to reset QEP deassert\n");
        return -1;
    }

    /* default configuration */
    for (i = 0; i < AIC_QEP_CH_NUM; i++)
        g_qep_args[i].id = i;

    g_qep_inited = 1;
    return 0;
}

int hal_qep_deinit(void)
{
    u32 i;

    for (i = 0; i < AIC_QEP_CH_NUM; i++) {
        hal_qep_int_enable(i, 0);
        hal_qep_enable(i, 0);
        qep_reg_enable(GLB_CLK_CTL, GLB_CLK_CTL_QEP_EN(i), 0);
    }

    hal_clk_disable_assertrst(CLK_PWMCS);
    hal_clk_disable(CLK_PWMCS);
    g_qep_inited = 0;
    return 0;
}
