/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <string.h>
#include <math.h>

#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_pwm.h"

/* Register definition of PWM Controller */

#define PWM_PWMx        0x300

#define PWM_CTL         0x000
#define PWM_MCTL        0x004
#define PWM_CKCTL       0x008
#define PWM_INTCTL      0x00C
#define PWM_INTSTS      0x010
#define PWM_TBCTL(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x000)
#define PWM_TBSTS(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x004)
#define PWM_TBPHS(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x008)
#define PWM_TBCTR(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x010)
#define PWM_TBPRD(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x014)
#define PWM_CMPCTL(n)   (PWM_PWMx + (((n) & 0x7) << 8) + 0x018)
#define PWM_CMPAHR(n)   (PWM_PWMx + (((n) & 0x7) << 8) + 0x01C)
#define PWM_CMPA(n)     (PWM_PWMx + (((n) & 0x7) << 8) + 0x020)
#define PWM_CMPB(n)     (PWM_PWMx + (((n) & 0x7) << 8) + 0x024)
#define PWM_AQCTLA(n)   (PWM_PWMx + (((n) & 0x7) << 8) + 0x028)
#define PWM_AQCTLB(n)   (PWM_PWMx + (((n) & 0x7) << 8) + 0x02C)
#define PWM_AQSFRC(n)   (PWM_PWMx + (((n) & 0x7) << 8) + 0x030)
#define PWM_AQCSFRC(n)  (PWM_PWMx + (((n) & 0x7) << 8) + 0x034)
#define PWM_DBCTL(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x038)
#define PWM_DBRED(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x03C)
#define PWM_DBFED(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x040)
#define PWM_ETSEL(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x044)
#define PWM_ETPS(n)     (PWM_PWMx + (((n) & 0x7) << 8) + 0x048)
#define PWM_ETFLG(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x04C)
#define PWM_ETCLR(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x050)
#define PWM_ETFRC(n)    (PWM_PWMx + (((n) & 0x7) << 8) + 0x054)
#define PWM_VERSION     0xFFC

#define PWM_DEFAULT_TB_CLK_RATE  24000000
#define PWM_DEFAULT_DB_RED       20
#define PWM_DEFAULT_DB_FED       20

#define PWM_ACTION_CFG_NUM       6

#define PWM_CTL_EN               BIT(0)
#define PWM_MCTL_PWM0_EN         BIT(0)
#define PWM_MCTL_PWM_EN(n)       (PWM_MCTL_PWM0_EN << (n))
#define PWM_CKCTL_PWM0_ON        BIT(0)
#define PWM_CKCTL_PWM_ON(n)      (PWM_CKCTL_PWM0_ON << (n))
#define PWM_INTCTL_PWM0_ON       BIT(0)
#define PWM_INTCTL_PWM_ON(n)     (PWM_INTCTL_PWM0_ON << (n))
#define PWM_TBCTL_CLKDIV_MAX     0xFFF
#define PWM_TBCTL_CLKDIV_SHIFT   16
#define PWM_TBCTL_CLKDIV_MASK    GENMASK(27, 16)
#define PWM_TBCTL_CTR_MODE_SHITF 0
#define PWM_TBCTL_CTR_MODE_MASK  GENMASK(1, 0)
#define PWM_TBPRD_MAX            0xFFFF
#define PWM_AQCTL_DEF_LEVEL      BIT(16)
#define PWM_AQCTL_CBD_SHIFT      10
#define PWM_AQCTL_CBU_SHIFT      8
#define PWM_AQCTL_CAD_SHIFT      6
#define PWM_AQCTL_CAU_SHIFT      4
#define PWM_AQCTL_PRD_SHIFT      2
#define PWM_AQCTL_MASK           0x3
#define PWM_ETSEL_INTEN_SHIFT    3
#define PWM_ETSEL_INTSEL_SHIFT   0
#define PWM_SHADOW_SEL_ZRQ_PRD   0xa

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC             1000000000
#endif

#ifndef AIC_PWM_CH_NUM
#define AIC_PWM_CH_NUM AIC_HRTIMER_CH_NUM
#endif

static struct aic_pwm_arg g_pwm_args[AIC_PWM_CH_NUM] = {{0}};

static inline void pwm_writel(u32 val, int reg)
{
    writel(val, PWM_BASE + reg);
}

static inline u32 pwm_readl(int reg)
{
    return readl(PWM_BASE + reg);
}

static inline void pwm_writel_bits(u32 val, u32 mask, u32 shift, int reg)
{
    writel_bits(val, mask, shift, PWM_BASE + reg);
}

static void aic_pwm_ch_info(char *buf, u32 ch, u32 en, struct aic_pwm_arg *arg)
{
    static const char *mode[] = {"Up", "Down", "UpDw"};
    static const char *act[] = {"-", "Low", "Hgh", "Inv"};

    snprintf(buf, 128, "%2d %2d %4s %11d %3d %3s %3s %3s %3s %3s %3s\n"
        "%30s %3s %3s %3s %3s %3s\n",
        ch, en & PWM_MCTL_PWM_EN(ch) ? 1 : 0,
        mode[arg->mode], arg->tb_clk_rate, arg->def_level,
        act[arg->action0.CBD], act[arg->action0.CBU],
        act[arg->action0.CAD], act[arg->action0.CAU],
        act[arg->action0.PRD], act[arg->action0.ZRO],
        act[arg->action1.CBD], act[arg->action1.CBU],
        act[arg->action1.CAD], act[arg->action1.CAU],
        act[arg->action1.PRD], act[arg->action1.ZRO]);
}

static int hal_pwm_ch_check(u32 ch)
{
    if (ch >= AIC_PWM_CH_NUM) {
        hal_log_err("ch%d is out of range, max num:%d\n", ch, AIC_PWM_CH_NUM -1);
        return -EINVAL;
    }
    return 0;
}

int hal_pwm_set_tb(u32 ch, int freq)
{
    struct aic_pwm_arg *arg = NULL;
    u32 div = 0;

    if (hal_pwm_ch_check(ch) < 0)
        return -EINVAL;

    arg = &g_pwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    if ((freq > arg->clk_rate) || (freq < (arg->clk_rate / (PWM_TBCTL_CLKDIV_MAX + 1)))) {
        hal_log_err("ch%d:time-base:%dHz is out of range[%d, %d]Hz\n", ch, freq,
            (arg->clk_rate / (PWM_TBCTL_CLKDIV_MAX + 1)), arg->clk_rate);
        return -EINVAL;
    }

    arg->tb_clk_rate = freq;

    if (arg->clk_rate == arg->tb_clk_rate)
        div = 0;
    else
        div = arg->clk_rate / arg->tb_clk_rate - 1;

    pwm_writel_bits(div, PWM_TBCTL_CLKDIV_MASK, PWM_TBCTL_CLKDIV_SHIFT, PWM_TBCTL(ch));

    return 0;
}

void hal_pwm_status_show(void)
{
    int ver = pwm_readl(PWM_VERSION);
    int enable = pwm_readl(PWM_MCTL);
    char info[AIC_PWM_CH_NUM][128] = {{0}};
    u32 i;

    printf("In PWM V%d.%02d:\n"
               "Module Enable: %d, IRQ Enable: %#x\n"
               "Ch En Mode Tb-clk-rate Def CBD CBU CAD CAU PRD ZRO\n",
               ver >> 8, ver & 0xFF,
               pwm_readl(PWM_CTL), pwm_readl(PWM_INTCTL));

    for (i = 0; i < AIC_PWM_CH_NUM; i++) {
        aic_pwm_ch_info(info[i], i, enable, &g_pwm_args[i]);
        printf("%s", info[i]);
    }
}

static void pwm_reg_enable(int offset, int bit, int enable)
{
    int tmp;

    tmp = pwm_readl(offset);
    tmp &= ~bit;
    if (enable)
        tmp |= bit;

    pwm_writel(tmp, offset);
}

u32 hal_pwm_int_sts(void)
{
    return pwm_readl(PWM_INTSTS);
}

void hal_pwm_clr_int(u32 stat)
{
    pwm_writel(stat, PWM_INTSTS);
}

void hal_pwm_int_config(u32 ch, enum aic_pwm_irq_event irq_mode, u8 enable)
{
    pwm_writel((enable << PWM_ETSEL_INTEN_SHIFT) | (irq_mode << PWM_ETSEL_INTSEL_SHIFT), PWM_ETSEL(ch));
    pwm_reg_enable(PWM_INTCTL, PWM_INTCTL_PWM_ON(ch), enable);
}

void hal_pwm_ch_init(u32 ch, enum aic_pwm_mode mode, u32 default_level,
                     struct aic_pwm_action *a0, struct aic_pwm_action *a1)
{
    struct aic_pwm_arg *arg = NULL;

    if (hal_pwm_ch_check(ch) < 0)
        return;

    arg = &g_pwm_args[ch];

    arg->mode = mode;
    arg->available = 1;
    arg->def_level = default_level;
    memcpy(&arg->action0, a0, sizeof(struct aic_pwm_action));
    memcpy(&arg->action1, a1, sizeof(struct aic_pwm_action));

    pwm_reg_enable(PWM_CKCTL, PWM_CKCTL_PWM_ON(ch), 1);
    pwm_writel(PWM_SHADOW_SEL_ZRQ_PRD, PWM_CMPCTL(ch));
}

static int hal_pwm_calculate_div(u32 ch, float tar_freq)
{
    struct aic_pwm_arg *arg = &g_pwm_args[ch];
    int div = -1, i = 0;
    float min_error = 5e6;

    for (i = 0; i < PWM_TBCTL_CLKDIV_MAX; ++i) {
        int count_value = arg->clk_rate / ((i + 1) * tar_freq);
        if (count_value < PWM_TBPRD_MAX) {
            float current_freq = arg->clk_rate / ((i + 1) * count_value);
            float error = fabs(current_freq - tar_freq);

            if (error < min_error) {
                min_error = error;
                div = i;
            }
         }
    }

    hal_log_debug("div:%d\n", div);

    if (div < 0) {
        hal_log_err("calculate div error,should adjust the PWM_CLK_RATE.\n");
        return -1;
    }

    pwm_writel_bits(div, PWM_TBCTL_CLKDIV_MASK, PWM_TBCTL_CLKDIV_SHIFT, PWM_TBCTL(ch));

    arg->tb_clk_rate = arg->clk_rate / (div + 1);

    return 0;
}


void hal_pwm_ch_deinit(u32 ch)
{
    struct aic_pwm_arg *arg = NULL;

    if (hal_pwm_ch_check(ch) < 0)
        return;

    arg = &g_pwm_args[ch];

    arg->available = 0;
    pwm_reg_enable(PWM_CKCTL, PWM_CKCTL_PWM_ON(ch), 0);
}

int hal_pwm_set_prd(u32 ch, u32 cnt)
{
    struct aic_pwm_arg *arg = NULL;

    if (hal_pwm_ch_check(ch) < 0)
        return -EINVAL;

    arg = &g_pwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    if (cnt > PWM_TBPRD_MAX) {
        hal_log_err("ch%d period %d is too big\n", ch, cnt);
        return -ERANGE;
    }

    arg->period = cnt;

    pwm_writel(arg->period, PWM_TBPRD(ch));

    return 0;
}

int hal_pwm_set(u32 ch, u32 duty_ns, u32 period_ns, u32 output)
{
    struct aic_pwm_arg *arg = NULL;
    u32 prd = 0;
    u64 duty = 0;

    if (hal_pwm_ch_check(ch) < 0)
        return -EINVAL;

    arg = &g_pwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    arg->freq = (float)NSEC_PER_SEC / period_ns;

    if (hal_pwm_calculate_div(ch, arg->freq) < 0)
        return -ERANGE;

    prd = arg->tb_clk_rate / arg->freq;

    hal_log_debug("output:Freq %d.%03d Hz, prd value %d, Time-base %d Hz\n",
        (int)arg->freq, (int)((arg->freq - (int)arg->freq) * 1000), prd, arg->tb_clk_rate);

    if (arg->mode == PWM_MODE_UP_DOWN_COUNT)
        prd >>= 1;
    else
        prd--;

    if (prd > PWM_TBPRD_MAX) {
        hal_log_err("ch%d period %d is too big\n", ch, prd);
        return -ERANGE;
    }
    arg->period = prd;
    pwm_writel(prd, PWM_TBPRD(ch));

    duty = (u64)duty_ns * (u64)prd;
    do_div(duty, period_ns);
    if (duty == prd)
        duty++;

    arg->duty = (u32)duty;
    switch (output) {
    case PWM_SET_CMPA:
        pwm_writel((u32)duty, PWM_CMPA(ch));
        hal_log_debug("ch%d Set CMPA %u/%u\n", ch, (u32)duty, prd);
        break;
    case PWM_SET_CMPB:
        pwm_writel((u32)duty, PWM_CMPB(ch));
        hal_log_debug("ch%d Set CMPB %u/%u\n", ch, (u32)duty, prd);
        break;
    case PWM_SET_CMPA_CMPB:
        pwm_writel((u32)duty, PWM_CMPA(ch));
        pwm_writel((u32)duty, PWM_CMPB(ch));
        hal_log_debug("ch%d Set CMPA&B %u/%u\n", ch, (u32)duty, prd);
        break;
    default:
        break;
    }

    return 0;
}

int hal_pwm_get(u32 ch, u32 *duty_ns, u32 *period_ns)
{
    struct aic_pwm_arg *arg = NULL;

    if (hal_pwm_ch_check(ch) < 0)
        return -EINVAL;

    arg = &g_pwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    *duty_ns   = arg->duty;
    *period_ns = arg->period;
    return 0;
}

int hal_pwm_set_polarity(u32 ch, enum pwm_polarity polarity)
{
    struct aic_pwm_arg *arg = NULL;

    if (hal_pwm_ch_check(ch) < 0)
        return -EINVAL;

    arg = &g_pwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    hal_log_debug("ch%d polarity %d\n", ch, polarity);
    /* Configuration of polarity in hardware delayed, do at enable */
    arg->polarity = polarity;
    return 0;
}

static void pwm_action_set(u32 ch, struct aic_pwm_action *act, char *name)
{
    u32 offset;
    u32 action = g_pwm_args[ch].def_level ? PWM_AQCTL_DEF_LEVEL : 0;

    if (strcmp(name, "action0") == 0)
        offset = PWM_AQCTLA(ch);
    else
        offset = PWM_AQCTLB(ch);

    action |= (act->CBD << PWM_AQCTL_CBD_SHIFT) |
          (act->CBU << PWM_AQCTL_CBU_SHIFT) |
          (act->CAD << PWM_AQCTL_CAD_SHIFT) |
          (act->CAU << PWM_AQCTL_CAU_SHIFT) |
          (act->PRD << PWM_AQCTL_PRD_SHIFT) | act->ZRO;
    pwm_writel(action, offset);
}

int hal_pwm_enable(u32 ch)
{
    struct aic_pwm_arg *arg = NULL;

    if (hal_pwm_ch_check(ch) < 0)
        return -EINVAL;

    arg = &g_pwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    hal_log_debug("ch%d enable\n", ch);

    pwm_writel_bits(arg->mode, PWM_TBCTL_CTR_MODE_MASK, PWM_TBCTL_CTR_MODE_SHITF, PWM_TBCTL(ch));

    pwm_action_set(ch, &arg->action0, "action0");
    pwm_action_set(ch, &arg->action1, "action1");

    pwm_reg_enable(PWM_MCTL, PWM_MCTL_PWM_EN(ch), 1);

    return 0;
}

int hal_pwm_disable(u32 ch)
{
    struct aic_pwm_arg *arg = NULL;

    if (hal_pwm_ch_check(ch) < 0)
        return -EINVAL;

    arg = &g_pwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    hal_log_debug("ch%d disable\n", ch);
    pwm_reg_enable(PWM_MCTL, PWM_MCTL_PWM_EN(ch), 0);
    return 0;
}

int hal_pwm_init(void)
{
    int i, ret = 0;
    int clock_rate;

    ret = hal_clk_set_freq(CLK_PWM, PWM_CLK_RATE);
    if (ret < 0) {
        hal_log_err("Failed to set PWM clk %d\n", PWM_CLK_RATE);
        return -1;
    }

    clock_rate = hal_clk_get_freq(CLK_PWM);
    if (clock_rate < 0) {
        hal_log_err("Failed to get PWM clk\n");
        return -1;
    }

    ret = hal_clk_enable(CLK_PWM);
    if (ret < 0) {
        hal_log_err("Failed to enable PWM clk\n");
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_PWM);
    if (ret < 0) {
        hal_log_err("Failed to reset PWM deassert\n");
        return -1;
    }

    pwm_reg_enable(PWM_CTL, PWM_CTL_EN, 1);

    /* default configuration */
    for (i = 0; i < AIC_PWM_CH_NUM; i++) {
        g_pwm_args[i].id = i;
        g_pwm_args[i].mode = PWM_MODE_UP_COUNT;
        g_pwm_args[i].tb_clk_rate = PWM_DEFAULT_TB_CLK_RATE;
        g_pwm_args[i].clk_rate = clock_rate;
    }

    return 0;
}

int hal_pwm_deinit(void)
{
    hal_clk_disable_assertrst(CLK_PWM);
    hal_clk_disable(CLK_PWM);
    return 0;
}
