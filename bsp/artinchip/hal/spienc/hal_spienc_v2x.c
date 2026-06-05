/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Hao Xiong <hao.xiong@artinchip.com>
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <aic_iopoll.h>
#include <hal_spienc.h>

#define SPIE_REG_CTL   0x00
#define SPIE_REG_ICR   0x04
#define SPIE_REG_ISR   0x08
#define SPIE_REG_KCNT  0x0C
#define SPIE_REG_OCNT  0x10
#define SPIE_REG_ADDR  0x14
#define SPIE_REG_TWEAK 0x18
#define SPIE_REG_CPOS  0x1C
#define SPIE_REG_CLEN  0x20

#define SPIE_REG_ALG_CTL      0x100
#define SPIE_REG_ALG_KEY_ADDR 0x104
#define SPIE_REG_ALG_DI_ADDR  0x10C
#define SPIE_REG_ALG_DI_LEN   0x110
#define SPIE_REG_ALG_DO_ADDR  0x114
#define SPIE_REG_ALG_DO_LEN   0x118
#define SPIE_REG_ALG_IV0      0x120
#define SPIE_REG_ALG_IV1      0x124
#define SPIE_REG_ALG_IV2      0x128
#define SPIE_REG_ALG_IV3      0x12C


#define SPIE_START_OFF   0
#define SPIE_SPI_SEL_OFF 12
#define SPIE_SPI_XIP_OFF 16
#define SPIE_WORK_MODE_OFF 17

#define SPIE_START_MSK   (0x1 << SPIE_START_OFF)
#define SPIE_SPI_SEL_MSK (0x3 << SPIE_SPI_SEL_OFF)
#define SPIE_WORK_MODE_MSK (0x1 << SPIE_WORK_MODE_OFF)

#define SPIE_ALG_START_OFF 0
#define SPIE_ALG_SEL_ALG_OFF 1
#define SPIE_ALG_SEL_KEY_OFF 2

#define SPIE_ALG_START_MSK   (0x1 << SPIE_ALG_START_OFF)
#define SPIE_ALG_SEL_ALG_MSK (0x1 << SPIE_ALG_SEL_ALG_OFF)
#define SPIE_ALG_SEL_KEY_MSK (0x1 << SPIE_ALG_SEL_KEY_OFF)

#define SPIE_INTR_KEY_GEN_MSK     (1 << 0)
#define SPIE_INTR_ENC_DEC_FIN_MSK (1 << 1)
#define SPIE_INTR_ALL_EMP_MSK     (1 << 2)
#define SPIE_INTR_HALF_EMP_MSK    (1 << 3)
#define SPIE_INTR_KEY_UDF_MSK     (1 << 4)
#define SPIE_INTR_KEY_OVF_MSK     (1 << 5)
#define SPIE_INTR_ALL_MSK         (0x3F)

#define SPI_CTLR_0     0
#define SPI_CTLR_1     1
#define SPI_CTLR_SE    5
#define SPI_CTLR_INVAL 0xFF


#define PTR2U32(ptr) ((u32)(uintptr_t)(ptr))

static int bypass = 0;
static int tweak_sel = AIC_SPIENC_USER_TWEAK;

int hal_spienc_init(void)
{
    int ret = 0;

    ret = hal_clk_enable(CLK_SPIENC);
    if (ret < 0) {
        hal_log_err("Failed to enable SID clk.\n");
        return -EFAULT;
    }

    ret = hal_clk_enable_deassertrst(CLK_SPIENC);
    if (ret < 0) {
        hal_log_err("Failed to reset SID deassert.\n");
        return -EFAULT;
    }
    /* Enable Interrupt */
    writel(SPIE_INTR_ALL_MSK, (SPI_ENC_BASE + SPIE_REG_ICR));

    return 0;
}

void hal_spienc_set_work_mode(int mode)
{
    u32 val;

    val = readl(SPI_ENC_BASE + SPIE_REG_CTL);
    val &= ~SPIE_WORK_MODE_MSK;
    val |= (mode << SPIE_WORK_MODE_OFF);

    writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
}

void hal_spienc_select_alg(int algo)
{
    u32 val;

    val = readl(SPI_ENC_BASE + SPIE_REG_ALG_CTL);
    val &= ~SPIE_ALG_SEL_ALG_MSK;
    val |= (algo << SPIE_ALG_SEL_ALG_OFF);

    writel(val, (SPI_ENC_BASE + SPIE_REG_ALG_CTL));
}

void hal_spienc_select_alg_key(int key_src)
{
    u32 val;

    val = readl(SPI_ENC_BASE + SPIE_REG_ALG_CTL);
    val &= ~SPIE_ALG_SEL_KEY_MSK;
    val |= (key_src << SPIE_ALG_SEL_KEY_OFF);

    writel(val, (SPI_ENC_BASE + SPIE_REG_ALG_CTL));
}

void hal_spienc_set_alg_key(u8 *key)
{
    writel(PTR2U32(key), (SPI_ENC_BASE + SPIE_REG_ALG_KEY_ADDR));
}

void hal_spienc_set_alg_iv(u8 *iv)
{
    if (iv == NULL) {
        return;
    }

    writel(*(u32 *)(iv + 0x0), (SPI_ENC_BASE + SPIE_REG_ALG_IV0));
    writel(*(u32 *)(iv + 0x4), (SPI_ENC_BASE + SPIE_REG_ALG_IV1));
    writel(*(u32 *)(iv + 0x8), (SPI_ENC_BASE + SPIE_REG_ALG_IV2));
    writel(*(u32 *)(iv + 0xc), (SPI_ENC_BASE + SPIE_REG_ALG_IV3));
}

void hal_spienc_start_alg(u8 *data, u32 dlen, u8 *out, u32 olen)
{
    int ret;
    u32 val;

    writel(0x70000, (SPI_ENC_BASE + SPIE_REG_ICR));
    writel(0x70000, (SPI_ENC_BASE + SPIE_REG_ISR));

    writel(PTR2U32(data), (SPI_ENC_BASE + SPIE_REG_ALG_DI_ADDR));
    writel(dlen, (SPI_ENC_BASE + SPIE_REG_ALG_DI_LEN));
    writel(PTR2U32(out), (SPI_ENC_BASE + SPIE_REG_ALG_DO_ADDR));

    val = readl((SPI_ENC_BASE + SPIE_REG_ALG_CTL));
    val &= ~SPIE_ALG_START_MSK;
    val |= SPIE_ALG_START_MSK;
    writel(val, (SPI_ENC_BASE + SPIE_REG_ALG_CTL));

    ret = readl_poll_timeout((SPI_ENC_BASE + SPIE_REG_ISR), val,
                             (val & 0x70000), 3000000);
    if (ret) {
        hal_log_err("spienc alg timeout\n");
        writel(val, (SPI_ENC_BASE + SPIE_REG_ISR));
        return;
    }

    if (val &0x40000) {
        hal_log_warn("alg calc error\n");
        writel(val, (SPI_ENC_BASE + SPIE_REG_ISR));
        return;
    } else if (val & 0x20000) {
        hal_log_warn("alg cfg error\n");
        writel(val, (SPI_ENC_BASE + SPIE_REG_ISR));
        return;
    } else if (val & 0x10000) {
        writel(val, (SPI_ENC_BASE + SPIE_REG_ISR));
    }
}

static int hal_spienc_attach_bus(u8 bus)
{
    u32 val;

    val = readl(SPI_ENC_BASE + SPIE_REG_CTL);
    val &= ~SPIE_SPI_SEL_MSK;

    switch (bus) {
#if defined(AIC_SPIENC_QSPI0)
        case SPI_CTLR_0:
            val |= (1 << SPIE_SPI_SEL_OFF);
            writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
            break;
#endif
#if defined(AIC_SPIENC_QSPI1)
        case SPI_CTLR_1:
            val |= (2 << SPIE_SPI_SEL_OFF);
            writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
            break;
#endif
#if defined(AIC_USING_SE_SPI)
        case SPI_CTLR_SE:
            val |= (1 << SPIE_SPI_SEL_OFF);
            writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
            break;
#endif
        case SPI_CTLR_INVAL:
            val |= (0 << SPIE_SPI_SEL_OFF);
            writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
            break;
        default:
            val |= (0 << SPIE_SPI_SEL_OFF);
            writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
            hal_log_warn("SPI controller %d not enable spienc\n", bus);
            return -EINVAL;
    }

    return 0;
}

void hal_spienc_set_cfg(u32 spi_bus, u32 addr, u32 cpos, u32 clen)
{
    u32 tweak = 0;

    if (spi_bus == 0) {
#if defined(AIC_SPIENC_QSPI0)
        tweak = AIC_SPIENC_QSPI0_TWEAK;
#endif
    } else if (spi_bus == 1) {
#if defined(AIC_SPIENC_QSPI1)
        tweak = AIC_SPIENC_QSPI1_TWEAK;
#endif
    } else if (spi_bus == 5) {
#if defined(AIC_SPIENC_SE_SPI)
        tweak = AIC_SPIENC_SE_SPI_TWEAK;
#endif
    } else {
        tweak = 0;
        hal_log_warn("not define spi %d tweak, default(%d).\n", spi_bus, tweak);
    }

    if (bypass)
        hal_spienc_attach_bus(0xff);
    else
        hal_spienc_attach_bus(spi_bus);

    if (tweak_sel == AIC_SPIENC_HW_TWEAK)
        tweak = 0;

    writel(addr, (SPI_ENC_BASE + SPIE_REG_ADDR));
    writel(cpos, (SPI_ENC_BASE + SPIE_REG_CPOS));
    writel(clen, (SPI_ENC_BASE + SPIE_REG_CLEN));
    writel(tweak, (SPI_ENC_BASE + SPIE_REG_TWEAK));
}

void hal_spienc_set_bypass(int status)
{
    bypass = status;
}

void hal_spienc_select_tweak(int select)
{
    tweak_sel = select;
}

void hal_spienc_xip_enable(void)
{
    u32 val;

    val = readl(SPI_ENC_BASE + SPIE_REG_CTL);
    val |= (1 << SPIE_SPI_XIP_OFF);
    writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
}

void hal_spienc_xip_disable(void)
{
    u32 val;

    val = readl(SPI_ENC_BASE + SPIE_REG_CTL);
    val &= ~(1 << SPIE_SPI_XIP_OFF);
    writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
}

void hal_spienc_start(void)
{
    u32 val;

    writel(SPIE_INTR_ALL_MSK, (SPI_ENC_BASE + SPIE_REG_ISR));
    val = readl((SPI_ENC_BASE + SPIE_REG_CTL));
    val |= SPIE_START_MSK;
    writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
}

void hal_spienc_stop(void)
{
    u32 val;

    val = readl((SPI_ENC_BASE + SPIE_REG_CTL));
    val &= ~SPIE_START_MSK;
    writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
}

int hal_spienc_check_empty(void)
{
    u32 val;

    val = readl((SPI_ENC_BASE + SPIE_REG_ISR));
    writel(val, (SPI_ENC_BASE + SPIE_REG_ISR));
    if (val & SPIE_INTR_ALL_EMP_MSK)
        return 1;

    return 0;
}

