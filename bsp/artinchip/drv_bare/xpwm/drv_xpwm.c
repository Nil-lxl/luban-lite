/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include <string.h>

#define LOG_TAG         "XPWM"
#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_xpwm.h"

#ifdef AIC_USING_PM
#include "pm.h"
#endif

#define FIFO_DEF_PRD 1000000
#define FIFO_DEF_CMP 500000

struct aic_xpwm_fifo g_fifo_para[AIC_XPWM_CH_NUM] = {0};
struct aic_xpwm_updt g_updt_para[AIC_XPWM_CH_NUM] = {0};
extern struct aic_xpwm_arg xpwm_pdata[];
extern const int xpwm_pdata_size;

#ifdef AIC_USING_PM
static struct aic_pm_device_node *g_xpwm_pm_node[AIC_XPWM_CH_NUM] = {NULL};
#endif

#ifdef AIC_USING_DMA
struct tx_complete {
    void (*func)(void *para);
    void *para;
};
static struct tx_complete g_xpwm_tx_complete[AIC_XPWM_CH_NUM];

static void xpwm_dma_callback(void *arg)
{
    struct aic_xpwm_arg *p = (struct aic_xpwm_arg *)arg;
    hal_release_dma_chan(p->t_info.dma_chan);
    if (g_xpwm_tx_complete[p->id].func != NULL)
        g_xpwm_tx_complete[p->id].func(g_xpwm_tx_complete[p->id].para);
}
#endif

int drv_xpwm_enable(u32 ch, bool enable)
{
    if (enable)
        return !hal_xpwm_enable(ch) ? EOK : -EINVAL;
    else
        return !hal_xpwm_disable(ch) ? EOK : -EINVAL;
}

int drv_xpwm_set(u32 ch, u32 period_ns, u32 duty_ns, u32 pulse_cnt)
{
    struct aic_xpwm_arg *arg = &g_xpwm_args[ch];

    if (arg->xpwm_int.reg_updt_int_en == 1) {
        g_updt_para[ch].ch = ch;
        g_updt_para[ch].pul_cmp = duty_ns;
        g_updt_para[ch].pul_prd = period_ns;
        g_updt_para[ch].pul_num = pulse_cnt;
    }

    if (hal_xpwm_set(ch, duty_ns, period_ns, pulse_cnt))
        return -EINVAL;

    return EOK;
}

int drv_xpwm_get(u32 ch, u32 *duty_ns, u32 *period_ns)
{
    if (hal_xpwm_get(ch, duty_ns, period_ns))
        return -EINVAL;

    return EOK;
}

int drv_xpwm_set_fifo_num(u32 ch, u32 fifo_num)
{
    struct aic_xpwm_arg *arg = &g_xpwm_args[ch];

    if (!hal_ch_is_xpwm(ch)) {
        pr_err("ch%d is not xpwm channel!\n", ch);
        return -EINVAL;
    }

    if (arg->fifo_en == 0) {
        pr_err("ch%d is not fifo mode!\n", ch);
        return -EINVAL;
    }

    if (arg->xpwm_int.fifo_aval_int_en == 0) {
        pr_err("ch%d is not normal fifo mode!\n", ch);
        return -EINVAL;
    }

    if((fifo_num > XPWM_FIFO_MAX) || (fifo_num < XPWM_FIFO_MIN)) {
        pr_err("Invalid fifo nums(%d-%d): %d!\n", XPWM_FIFO_MIN, XPWM_FIFO_MAX, fifo_num);
        return -EINVAL;
    }

    /* disable the interrupt */
    arg->xpwm_int.fifo_aval_int_en = 0;
    hal_xpwm_irq_en_set(ch, &arg->xpwm_int);

    g_fifo_para[ch].fifo_index = 0;
    g_fifo_para[ch].fifo_num = fifo_num;

    /* enable the interrupt */
    arg->xpwm_int.fifo_aval_int_en = 1;
    hal_xpwm_irq_en_set(ch, &arg->xpwm_int);

    hal_xpwm_resume(ch);

    return EOK;
}

int drv_xpwm_set_fifo(u32 ch, struct aic_xpwm_fifo fifo_info)
{
    struct aic_xpwm_arg *arg = &g_xpwm_args[ch];

    if (!hal_ch_is_xpwm(ch)) {
        pr_err("ch%d is not xpwm channel!\n", ch);
        return -EINVAL;
    }

    if (arg->fifo_en == 0) {
        pr_err("ch%d is not fifo mode!\n", ch);
        return -EINVAL;
    }

    if (arg->xpwm_int.fifo_aval_int_en == 0) {
        pr_err("ch%d is not normal fifo mode!\n", ch);
        return -EINVAL;
    }

    if((fifo_info.fifo_index > (XPWM_FIFO_MAX - 1)) || (fifo_info.fifo_index < (XPWM_FIFO_MIN - 1))) {
        pr_err("Invalid fifo index(%d-%d): %d!\n", XPWM_FIFO_MIN - 1, XPWM_FIFO_MAX - 1, fifo_info.fifo_index);
        return -EINVAL;
    }

    /* disable the interrupt */
    arg->xpwm_int.fifo_aval_int_en = 0;
    hal_xpwm_irq_en_set(ch, &arg->xpwm_int);

    g_fifo_para[ch].fifo_index = 0;
    g_fifo_para[ch].pul_prd[fifo_info.fifo_index] = fifo_info.pul_prd[fifo_info.fifo_index];
    g_fifo_para[ch].pul_cmp[fifo_info.fifo_index] = fifo_info.pul_cmp[fifo_info.fifo_index];
    g_fifo_para[ch].pul_num[fifo_info.fifo_index] = fifo_info.pul_num[fifo_info.fifo_index];

    /* enable the interrupt */
    arg->xpwm_int.fifo_aval_int_en = 1;
    hal_xpwm_irq_en_set(ch, &arg->xpwm_int);

    hal_xpwm_resume(ch);

    return EOK;
}

#ifdef AIC_USING_DMA
int drv_xpwm_dma_set_fifo(u32 ch, struct aic_xpwm_buf_info dma_info, void *tx_complete, void *cb_para)
{
    struct aic_xpwm_arg *arg = &g_xpwm_args[ch];

    if (!hal_ch_is_xpwm(ch)) {
        pr_err("ch%d is not xpwm channel!\n", ch);
        return -EINVAL;
    }

    if (arg->fifo_en == 0) {
        pr_err("ch%d is not fifo mode!\n", ch);
        return -EINVAL;
    }

    if (arg->dma_en == 0) {
        pr_err("ch%d is not dma fifo mode!\n", ch);
        return -EINVAL;
    }

    arg->t_info.buf_info.buf = dma_info.buf;
    arg->t_info.buf_info.buf_len = dma_info.buf_len;

    for (int i = 0; i < arg->t_info.buf_info.buf_len / sizeof(u32); i += 3) {
        xpwm_cal_prd_duty(ch, arg->t_info.buf_info.buf[i + 1], arg->t_info.buf_info.buf[i]);
        arg->t_info.buf_info.buf[i] = arg->period;
        arg->t_info.buf_info.buf[i + 1] = arg->duty;
    }

    if (tx_complete != NULL) {
        g_xpwm_tx_complete[ch].func = tx_complete;
        g_xpwm_tx_complete[ch].para = cb_para;
    }
    hal_xpwm_dma_config(ch, xpwm_dma_callback, arg);

    return EOK;
}
#endif

int drv_xpwm_get_fifo(u32 ch)
{
    struct aic_xpwm_arg *arg = &g_xpwm_args[ch];
    int i;

    if (!hal_ch_is_xpwm(ch)) {
        pr_err("ch%d is not xpwm channel!\n", ch);
        return -EINVAL;
    }

    if (arg->fifo_en == 0) {
        pr_err("ch%d is not fifo mode!\n", ch);
        return -EINVAL;
    }

    if (arg->xpwm_int.fifo_aval_int_en == 0) {
        pr_err("ch%d is not normal fifo mode!\n", ch);
        return -EINVAL;
    }

    pr_info("ch%d fifo valid count: %d\n", ch, g_fifo_para[ch].fifo_num);

    for (i = 0; i < g_fifo_para[ch].fifo_num; i++)
        pr_info("fifo_idx %d: prd:%d pulse:%d pulse cnt:%d\n", i, g_fifo_para[ch].pul_prd[i],
            g_fifo_para[ch].pul_cmp[i], g_fifo_para[ch].pul_num[i]);

    return EOK;
}

irqreturn_t aic_xpwm_irq(int irq, void *arg)
{
    struct aic_xpwm_arg *parg;
    struct aic_xpwm_fifo *fifo_para = (struct aic_xpwm_fifo *)arg;
    struct aic_xpwm_updt *updt_para = (struct aic_xpwm_updt *)arg;
    u32 *pch = (u32 *)arg;

    u32 stat = 0;
    u32 ch = (u32)*pch;

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

#ifdef AIC_USING_PM
void aic_xpwm_suspend(void *arg, int mode)
{
    u8 ch = *(u8 *)arg;
    struct aic_xpwm_arg *xpwm_arg = NULL;

    if (ch >= AIC_XPWM_CH_NUM) {
        pr_err("xpwm ch:%d out of range, suspend failed.\n", ch);
        return;
    }
    xpwm_arg = &g_xpwm_args[ch];

    UNUSED(mode);

    hal_clk_disable_assertrst(xpwm_arg->clk);
}

void aic_xpwm_resume(void *arg, int mode)
{
    u8 ch = *(u8 *)arg;

    if (ch >= AIC_XPWM_CH_NUM) {
        pr_err("xpwm ch:%d out of range, resume failed.\n", ch);
        return;
    }

    UNUSED(mode);

#ifdef AIC_XPWM_DRV_V10
    if ((ch >= 0) && (ch <= 7)) {
        if (hal_clk_set_freq(CLK_XPWM_SDFM, XPWM_CLK_RATE) < 0)
            pr_err("Failed to set XPWM clk %d\n", XPWM_CLK_RATE);
    } else {
        if (hal_clk_set_freq(CLK_PWM_SDFM, XPWM_CLK_RATE) < 0)
            pr_err("Failed to set PWM clk %d\n", XPWM_CLK_RATE);
    }
#endif
#ifdef AIC_XPWM_DRV_V11
    struct aic_xpwm_arg *xpwm_arg = &g_xpwm_args[ch];

    if (hal_clk_set_freq(xpwm_arg->clk, xpwm_arg->set_clk_rate) < 0)
        pr_err("Failed to set XPWM clk %d\n", xpwm_arg->set_clk_rate);
#endif
}
#endif

int drv_xpwm_init(void)
{
    int i, j;
    u32 xpwm_ch = 0;

#ifdef AIC_XPWM_DRV_V10
    int pwm_clk_rate = 0, xpwm_clk_rate = 0;

    if (hal_clk_set_freq(CLK_PWM_SDFM, XPWM_CLK_RATE) < 0) {
        pr_err("Failed to set PWM clk %d\n", XPWM_CLK_RATE);
        return -EINVAL;
    }
    if (hal_clk_set_freq(CLK_XPWM_SDFM, XPWM_CLK_RATE) < 0) {
        pr_err("Failed to set XPWM clk %d\n", XPWM_CLK_RATE);
        return -EINVAL;
    }
    pwm_clk_rate = hal_clk_get_freq(CLK_PWM_SDFM);
    if (pwm_clk_rate < 0) {
        pr_err("Failed to get PWM clk.\n");
        return -EINVAL;
    }
    xpwm_clk_rate = hal_clk_get_freq(CLK_XPWM_SDFM);
    if (pwm_clk_rate < 0) {
        pr_err("Failed to get XPWM clk.\n");
        return -EINVAL;
    }
    if (pwm_clk_rate != xpwm_clk_rate) {
        pr_err("please check the clk.\n");
        return -EINVAL;
    }
#endif

    for (i = 0; i < xpwm_pdata_size; i++) {
#ifdef AIC_XPWM_DRV_V10
        xpwm_pdata[i].act_clk_rate = xpwm_clk_rate;
#endif
#ifdef AIC_XPWM_DRV_V11
    if (hal_clk_set_freq(xpwm_pdata[i].clk, xpwm_pdata[i].set_clk_rate) < 0) {
        pr_err("Failed to set XPWM clk %d\n", xpwm_pdata[i].set_clk_rate);
        return -EINVAL;
    }
    xpwm_pdata[i].act_clk_rate = hal_clk_get_freq(xpwm_pdata[i].clk);
    if (xpwm_pdata[i].act_clk_rate < 0) {
        pr_err("Failed to get XPWM clk.\n");
        return -EINVAL;
    }
#endif

        hal_xpwm_ch_init(&xpwm_pdata[i]);
        xpwm_ch = xpwm_pdata[i].id;

        if (hal_ch_is_xpwm(xpwm_ch) == 1) {
            if (xpwm_pdata[i].xpwm_int.fifo_aval_int_en == 1) {
                g_fifo_para[xpwm_ch].ch = xpwm_ch;
                g_fifo_para[xpwm_ch].fifo_num = XPWM_FIFO_MAX;
                for (j = 0; j < XPWM_FIFO_MAX; j++) {
                    g_fifo_para[xpwm_ch].pul_num[j] = 1;
                    g_fifo_para[xpwm_ch].pul_prd[j] = FIFO_DEF_PRD;
                    g_fifo_para[xpwm_ch].pul_cmp[j] = FIFO_DEF_CMP;
                }
                aicos_request_irq(xpwm_pdata[i].irq, aic_xpwm_irq, 0, NULL, &g_fifo_para[xpwm_ch]);
            } else if (xpwm_pdata[i].xpwm_int.reg_updt_int_en == 1) {
                aicos_request_irq(xpwm_pdata[i].irq, aic_xpwm_irq, 0, NULL, &g_updt_para[xpwm_ch]);
            } else {
                pr_debug("ch:%d Normal mode does not require registration interruption.\n", xpwm_ch);
            }
        }

#ifdef AIC_USING_PM
        g_xpwm_pm_node[xpwm_ch] = (struct aic_pm_device_node *)aicos_malloc(MEM_DEFAULT,\
        sizeof(struct aic_pm_device_node));
        if (g_xpwm_pm_node[xpwm_ch] == NULL) {
            pr_err("ch%d:Failed to malloc pm node.\n", xpwm_ch);
            goto err;
        }
        g_xpwm_pm_node[xpwm_ch]->suspend = aic_xpwm_suspend;
        g_xpwm_pm_node[xpwm_ch]->resume = aic_xpwm_resume;
        g_xpwm_pm_node[xpwm_ch]->arg = (void *)&xpwm_pdata[i].id;
        snprintf(g_xpwm_pm_node[xpwm_ch]->name, sizeof(g_xpwm_pm_node[xpwm_ch]->name), "xpwm%d", xpwm_ch);
        aic_pm_device_register(g_xpwm_pm_node[xpwm_ch]);
#endif
    }

    pr_info("ArtInChip XPWM loaded.\n");
    return EOK;

#ifdef AIC_USING_PM
err:
    for (i = 0; i < AIC_XPWM_CH_NUM; i++) {
        if (g_xpwm_pm_node[i])
            aicos_free(MEM_DEFAULT, (void *)g_xpwm_pm_node[i]);
    }
    return -EINVAL;
#endif
}

