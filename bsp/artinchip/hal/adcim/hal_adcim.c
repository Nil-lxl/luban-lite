/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_adcim.h"
#ifdef AIC_SYSCFG_DRV
#include "hal_syscfg.h"
#endif

/* Register of ADCIM */
#define ADCIM_MCSR       0x000
#define ADCIM_CALCSR     0x004
#define ADCIM_FIFOSTS    0x008
#define ADCIM_VERSION    0xFFC

#define ADCIM_MCSR_BUSY                 BIT(16)
#define ADCIM_MCSR_SEMFLAG_SHIFT        8
#define ADCIM_MCSR_SEMFLAG_MASK         GENMASK(15, 8)
#define ADCIM_MCSR_CHN_MASK             GENMASK(3, 0)
#define ADCIM_CALCSR_CALVAL_UPD         BIT(31)
#define ADCIM_CALCSR_CALVAL_SHIFT       16
#define ADCIM_CALCSR_CALVAL_MASK        GENMASK(27, 16)
#define ADCIM_CALCSR_ADC_ACQ_SHIFT      8
#define ADCIM_CALCSR_ADC_ACQ_MASK       GENMASK(15, 8)
#define ADCIM_CALCSR_DCAL_MASK          BIT(1)
#define ADCIM_CALCSR_CAL_ENABLE         BIT(0)
#define ADCIM_FIFOSTS_ADC_ARBITER_IDLE  BIT(6)
#define ADCIM_FIFOSTS_FIFO_ERR          BIT(5)
#define ADCIM_FIFOSTS_CTR_MASK          GENMASK(4, 0)
#define ADCIM_CAL_ADC_STANDARD_VAL      0x800
#define AIC_ADC_MAX_VAL                 0xFFF
#define AIC_VOLTAGE_ACCURACY            10000
#define ADCIM_CALCSR_NUM                6
#define ADCIM_ADC_ACC_BIT               12
#define ADCIM_ADC_ACC_RANGE             ((1 << ADCIM_ADC_ACC_BIT) - 1)

#ifdef AIC_CHIP_D12X
#define ADCIM_CAL_ADC_OFFSET_MISMATCH   0x8
#elif defined(AIC_CHIP_D13X)
#define ADCIM_CAL_ADC_OFFSET_MISMATCH   0x28
#elif defined(AIC_CHIP_D21X)
#define ADCIM_CAL_ADC_OFFSET_MISMATCH   0x32
#else
#define ADCIM_CAL_ADC_OFFSET_MISMATCH   0x0
#endif
#ifdef AIC_ADCIM_DM_DRV
#define ADCDM_RTP_CFG   0x03F0
#define ADCDM_RTP_STS   0x03F4
#define ADCDM_SRAM_CTL  0x03F8
#define ADCDM_SRAM_BASE 0x0400

#define ADCDM_RTP_CAL_VAL_SHIFT     16
#define ADCDM_RTP_CAL_VAL_MASK      GENMASK(27, 16)
#define ADCDM_RTP_PDET              BIT(0)

#define ADCDM_RTP_DRV_SHIFT         4
#define ADCDM_RTP_DRV_MASK          GENMASK(7, 4)
#define ADCDM_RTP_VPSEL_SHIFT       2
#define ADCDM_RTP_VPSEL_MASK        GENMASK(3, 2)
#define ADCDM_RTP_VNSEL_MASK        GENMASK(1, 0)

#define ADCDM_SRAM_CLR_SHIFT        16
#define ADCDM_SRAM_CLR(n)           (1 << ((n) + ADCDM_SRAM_CLR_SHIFT))
#define ADCDM_SRAM_MODE_SHIFT       8
#define ADCDM_SRAM_MODE             BIT(8)
#define ADCDM_SRAM_SEL_MASK         GENMASK(3, 0)
#define ADCDM_SRAM_SEL(n)           (n)

#define ADCDM_SRAM_SIZE             (512 * 4)
#define ADCDM_CHAN_NUM              16

enum adcdm_sram_mode {
    ADCDM_NORMAL_MODE = 0,
    ADCDM_DEBUG_MODE = 1
};
#endif

struct adcim_dev {
#ifdef AIC_ADCIM_DM_DRV
    u32 dm_cur_chan;
#endif
    int usr_cnt;
};

#ifdef AIC_ADCIM_DM_DRV
static struct adcim_dev g_adcim_dev = {0};
#endif

static int g_adcim_cal_param = 0;

static inline void adcim_writel(u32 val, int reg)
{
    writel(val, ADCIM_BASE + reg);
}

static inline u32 adcim_readl(int reg)
{
    return readl(ADCIM_BASE + reg);
}

int hal_adcim_calibration_set(unsigned int val)
{
    int cal;

    if (val > 4095) {
        pr_err("The calibration value %d is too big\n", val);
        return -EINVAL;
    }

    cal = adcim_readl(ADCIM_CALCSR);
    cal = (cal & ~ADCIM_CALCSR_CALVAL_MASK)
        | (val << ADCIM_CALCSR_CALVAL_SHIFT);
    cal = cal | ADCIM_CALCSR_CALVAL_UPD;
    adcim_writel(cal, ADCIM_CALCSR);

    return 0;
}

/*
 * The calibration value is taken six times and the average value is obtained
 * after removing the maximum and minimum values, in order to ensure the
 * stability of calibration
 */
u32 hal_adcim_auto_calibration(void)
{
    u32 flag = 1;
    u32 data = 0;
    int max = 0;
    int min = 0;
    u32 cal_array[ADCIM_CALCSR_NUM];

    for (int i = 0; i < ADCIM_CALCSR_NUM; i++) {
        adcim_writel(0x08002f03, ADCIM_CALCSR);//auto cal
        do {
            flag = adcim_readl(ADCIM_CALCSR) & 0x00000001;
        } while (flag);

        cal_array[i] = (adcim_readl(ADCIM_CALCSR) >> 16) & 0xfff;

        if (cal_array[i] > max)
            max = cal_array[i];

        if (i == 0) {
            min = cal_array[0];
        } else if (cal_array[i] < min) {
            min = cal_array[i];
        }

        data += cal_array[i];
        pr_debug("[%d]cal_data %d\n", i, cal_array[i]);
    }

    data = (data - min - max) / (ADCIM_CALCSR_NUM - 2);

    pr_debug("max %d min %d, latest_data %d\n", max, min, data);
    return data;
}

int hal_adcim_adc2voltage(u16 *val, u32 cal_data, int scale, float def_voltage)
{
    int cal_voltage;
    int st_voltage = 0;
    int cal_val = 0;

#ifdef AIC_SYSCFG_DRV
    st_voltage = hal_syscfg_read_ldo_cfg();
#endif
    if (!st_voltage) {
        pr_debug("Failed to get standard voltage\n");
        st_voltage = (int)(def_voltage * AIC_VOLTAGE_ACCURACY);
    }

    cal_val = *val + ADCIM_CAL_ADC_STANDARD_VAL - cal_data + ADCIM_CAL_ADC_OFFSET_MISMATCH;
    if (cal_val >= 0) {
        *val = cal_val;
        cal_voltage = *val * st_voltage / AIC_ADC_MAX_VAL;
    } else {
        pr_err("Out of the input voltage range - %d \n", cal_val);
        *val = 0;
        cal_voltage = 0;
    }

    return cal_voltage;
}

u32 hal_adcim_adc2caled(u32 adc_data)
{
    int caled_adc_val = 0;

    caled_adc_val = adc_data + ADCIM_CAL_ADC_STANDARD_VAL - g_adcim_cal_param + ADCIM_CAL_ADC_OFFSET_MISMATCH;

    if (caled_adc_val > ADCIM_ADC_ACC_RANGE)
        caled_adc_val = ADCIM_ADC_ACC_RANGE;
    if (caled_adc_val < 0)
        caled_adc_val = 0;
    pr_debug("Calibration param: %d\n", g_adcim_cal_param);
    return caled_adc_val;
}

void adcim_status_show(void)
{
    int mcsr;
    int fifo;
    int version;
#ifdef AIC_ADCIM_DM_DRV
    int rtp_cfg, rtp_sts, sram_ctl;
#endif

    mcsr = adcim_readl(ADCIM_MCSR);
    fifo = adcim_readl(ADCIM_FIFOSTS);
    version = adcim_readl(ADCIM_VERSION);

#ifdef AIC_ADCIM_DM_DRV
    rtp_cfg = adcim_readl(ADCDM_RTP_CFG);
    rtp_sts = adcim_readl(ADCDM_RTP_STS);
    sram_ctl = adcim_readl(ADCDM_SRAM_CTL);
#endif

    hal_log_info("In ADCIM V%d.%02d:\n"
               "Busy state: %d\n"
               "Semflag: %d\n"
               "Current Channel: %d\n"
               "ADC Arbiter Idel: %d\n"
               "FIFO Error: %d\n"
               "FIFO Counter: %d\n"
#ifdef AIC_ADCIM_DM_DRV
               "\nIn DM:\nMode: %s, Current channel: %d/%ld\n"
               "Calibration val: %ld\n"
               "RTP PDET: %ld, DRV: %ld, VPSEL: %ld, VNSEL: %ld\n"
#endif
               , version >> 8, version & 0xff,
               (mcsr & ADCIM_MCSR_BUSY) ? 1 : 0,
               (mcsr & ADCIM_MCSR_SEMFLAG_MASK)
                >> ADCIM_MCSR_SEMFLAG_SHIFT,
               mcsr & ADCIM_MCSR_CHN_MASK,
               fifo & ADCIM_FIFOSTS_ADC_ARBITER_IDLE ? 1 : 0,
               fifo & ADCIM_FIFOSTS_FIFO_ERR ? 1 : 0,
               fifo & ADCIM_FIFOSTS_CTR_MASK
#ifdef AIC_ADCIM_DM_DRV
               , sram_ctl & ADCDM_SRAM_MODE ? "Debug" : "Normal",
               g_adcim_dev.dm_cur_chan, sram_ctl & ADCDM_SRAM_SEL_MASK,
               (rtp_cfg & ADCDM_RTP_CAL_VAL_MASK)
                >> ADCDM_RTP_CAL_VAL_SHIFT,
               rtp_cfg & ADCDM_RTP_PDET,
               (rtp_sts & ADCDM_RTP_DRV_MASK) >> ADCDM_RTP_DRV_SHIFT,
               (rtp_sts & ADCDM_RTP_VPSEL_MASK)
                >> ADCDM_RTP_VPSEL_SHIFT,
               rtp_sts & ADCDM_RTP_VNSEL_MASK
#endif
               );
}

void adcim_calibration_show(void)
{
    int cal;

    cal = adcim_readl(ADCIM_CALCSR);

    pr_info("Calibration Enable: %d\n Current value: %d\nADC ACQ: %d\n",
           (cal & ADCIM_CALCSR_CAL_ENABLE) ? 0 : 1,
           (cal & ADCIM_CALCSR_CALVAL_MASK) >> ADCIM_CALCSR_CALVAL_SHIFT,
           (cal & ADCIM_CALCSR_ADC_ACQ_MASK) >> ADCIM_CALCSR_ADC_ACQ_SHIFT);
}

#ifdef AIC_ADCIM_DM_DRV
void hal_dm_chan_show(void)
{
    pr_info("Current chan: %d/%ld\n", g_adcim_dev.dm_cur_chan,
               adcim_readl(ADCDM_SRAM_CTL) & ADCDM_SRAM_SEL_MASK);
}

s32 hal_dm_chan_store(u32 val)
{
    int ret;

    if (val >= ADCDM_CHAN_NUM) {
        hal_log_err("Invalid channel number %u\n", val);
        return 0;
    }

    g_adcim_dev.dm_cur_chan = val;
    ret = adcim_readl(ADCDM_SRAM_CTL);
    ret &= ~ADCDM_SRAM_SEL_MASK;
    ret |= val;
    adcim_writel(ret, ADCDM_SRAM_CTL);

    return 0;
}

void hal_adcdm_rtp_down_store(u32 val)
{
    int ret;

    ret = adcim_readl(ADCDM_RTP_CFG);
    if (val)
        ret &= ~ADCDM_RTP_PDET;
    else
        ret |= ADCDM_RTP_PDET;
    adcim_writel(ret, ADCDM_RTP_CFG);
}

static void adcdm_sram_clear(u32 chan)
{
    u32 val = 0;

    val = adcim_readl(ADCDM_SRAM_CTL);
    val |= ADCDM_SRAM_CLR(chan);
    adcim_writel(val, ADCDM_SRAM_CTL);

    val &= ~ADCDM_SRAM_CLR(chan);
    adcim_writel(val, ADCDM_SRAM_CTL);
}

static void adcdm_sram_mode(enum adcdm_sram_mode mode)
{
    u32 val = 0;

    val = adcim_readl(ADCDM_SRAM_CTL);
    if (mode)
        val |= mode << ADCDM_SRAM_MODE_SHIFT;
    else
        val &= ~ADCDM_SRAM_MODE;
    adcim_writel(val, ADCDM_SRAM_CTL);
}

static void adcdm_sram_select(u32 chan)
{
    u32 val = 0;

    val = adcim_readl(ADCDM_SRAM_CTL);
    val &= ~ADCDM_SRAM_SEL_MASK;
    val |= chan;
    adcim_writel(val, ADCDM_SRAM_CTL);
}

ssize_t hal_adcdm_sram_write(int *buf, u32 offset, size_t count)
{
    int i;

    if (count + offset > ADCDM_SRAM_SIZE)
        return 0;
    hal_adcim_probe();
    adcdm_sram_mode(ADCDM_DEBUG_MODE);
    adcdm_sram_select(g_adcim_dev.dm_cur_chan);

    for (i = 0; i < count; i++) {
        adcim_writel(buf[i], ADCDM_SRAM_BASE + i * 4);
    }

    adcdm_sram_mode(ADCDM_NORMAL_MODE);
    for (i = 0; i < ADCDM_CHAN_NUM; i++)
        adcdm_sram_clear(i);

    return count;
}

#endif

void hal_adcim_set_dcalmask(void)
{
    int val;

    val = adcim_readl(ADCIM_CALCSR);
    val = val | ADCIM_CALCSR_DCAL_MASK | ADCIM_CALCSR_ADC_ACQ_MASK;
    adcim_writel(val, ADCIM_CALCSR);
}

s32 hal_adcim_probe(void)
{
    s32 ret = 0;
    static s32 inited = 0;

    if (inited) {
        hal_log_info("ADCIM is already inited\n");
        return 0;
    }

#ifdef AIC_ADCIM_DRV_V11
    ret = hal_clk_set_freq(CLK_ADCIM, 48000000);
    if (ret < 0) {
        hal_log_err("Failed to set ADCIM clk\n");
        return -1;
    }
#endif

    ret = hal_clk_enable(CLK_ADCIM);
    if (ret < 0) {
        hal_log_err("ADCIM clk enable failed!\n");
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_ADCIM);
    if (ret < 0) {
        hal_log_err("ADCIM reset deassert failed!\n");
        return -1;
    }

#ifdef AIC_ADCIM_DELTA_ADC
    //Init delta ADC
    adcim_writel(0xf002a600, ADCIM_MCSR);
#endif

    hal_adcim_set_dcalmask();
    g_adcim_cal_param = hal_adcim_auto_calibration();

    inited = 1;
    return 0;
}
