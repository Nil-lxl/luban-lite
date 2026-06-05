/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: hao.xiong@artinchip.com
 */

#include <stdio.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_clk_id.h>
#include "aic_hal_gpio.h"

#define CMU_GPIO_REG         ((void *)(CMU_BASE + 0x083c))
#define CMU_PLL_INT1_CFG_REG ((void *)(CMU_BASE + 0x0004))
#define CMU_UART_REG(id)     ((void *)(CMU_BASE + 0x0840 + 4 * (id)))

#define UART_THR_REG(id)  ((void *)(UART_BASE(id) + 0x0000))
#define UART_DLL_REG(id)  ((void *)(UART_BASE(id) + 0x0000))
#define UART_DLH_REG(id)  ((void *)(UART_BASE(id) + 0x0004))
#define UART_IER_REG(id)  ((void *)(UART_BASE(id) + 0x0004))
#define UART_FCR_REG(id)  ((void *)(UART_BASE(id) + 0x0008))
#define UART_LCR_REG(id)  ((void *)(UART_BASE(id) + 0x000c))
#define UART_LSR_REG(id)  ((void *)(UART_BASE(id) + 0x0014))
#define UART_USR_REG(id)  ((void *)(UART_BASE(id) + 0x007c))
#define UART_HALT_REG(id) ((void *)(UART_BASE(id) + 0x00a4))

#define AIC_CLK_UART_MAX_FREQ 60000000 /* max 60M */

// UART Line Control Parameter
#define PARITY 0 //Parity: 0,2 - NONE; 1 - ODD; 3 - EVEN
#define STOP   0 //Number of Stop Bit: 0 - 1bit; 1 - 2(or 1.5)bits
#define DLEN   3 //Data Length: 0 - 5bits; 1 - 6bits; 2 - 7bits; 3 - 8bits

#define LSR_TX_EMP_BIT BIT(6)

static int id = AIC_EARLY_DEBUG_UART;

int get_best_uart_div(int *best_cmu_div, int *best_uart_div)
{
    int cmu_div, uart_div;
    int parent_freq;
    int expect_baudrate, real_baudrate;
    u32 factor_n, factor_m, factor_p;
    u64 err, min_err;

    expect_baudrate = 115200;
    min_err = expect_baudrate / 40;

    /* PLL output mux is CLK_24M */
    if (!((readl(CMU_PLL_INT1_CFG_REG) >> PLL_OUT_MUX) & 0x1)) {
        parent_freq = CLOCK_24M;
    } else {
        factor_n = (readl(CMU_PLL_INT1_CFG_REG) >> PLL_FACTORN_BIT) & PLL_FACTORN_MASK;
        factor_m = (readl(CMU_PLL_INT1_CFG_REG) >> PLL_FACTORM_BIT) & PLL_FACTORM_MASK;
        factor_p = (readl(CMU_PLL_INT1_CFG_REG) >> PLL_FACTORP_BIT) & PLL_FACTORP_MASK;
        parent_freq = CLOCK_24M / (factor_p + 1) * (factor_n + 1) / (factor_m + 1);
    }

#if defined(AIC_CHIP_D12P)
    for (cmu_div = 64; cmu_div > 0; cmu_div--) {
#elif defined(AIC_CHIP_D13X) || defined(AIC_CHIP_G73X) || defined(AIC_CHIP_D21X)
    for (cmu_div = 32; cmu_div > 0; cmu_div--) {
#elif defined(AIC_CHIP_D12X)
    for (cmu_div = 16; cmu_div > 0; cmu_div--) {
#else
    for (cmu_div = 16; cmu_div > 0; cmu_div--) {
#endif
        if ((parent_freq / cmu_div) > AIC_CLK_UART_MAX_FREQ)
            continue;
        for (uart_div = 1; uart_div < 65536; uart_div++) {
            real_baudrate = parent_freq / (cmu_div * 16 * uart_div);
            err = abs(expect_baudrate - real_baudrate);
            if (err < min_err) {
                *best_cmu_div = cmu_div;
                *best_uart_div = uart_div;
                min_err = err;
            }
        }
    }

    if (min_err >= (expect_baudrate / 40)) {
        printf("Error is too large for this baud rate.\n");
        return -1;
    }

    return 0;
}

void early_debug_board_init(void)
{
    u32 val = 0, cfg_val, group, tx_pin, pin;
    volatile void *cfg_reg;
    int cmu_div = 0, uart_div = 0;

    /* Reset and Gating GPIO */
    writel(0x3100, CMU_GPIO_REG);

    tx_pin = hal_gpio_name2pin(AIC_EARLY_DEBUG_UART_TX_GPIO);
    if (tx_pin < 0)
        return;

    /* Config GPIO */
    group = GPIO_GROUP(tx_pin);
    pin = GPIO_GROUP_PIN(tx_pin);
    cfg_reg = (volatile void *)(group * 0x100 + pin * 0x4 + 0x80 + GPIO_BASE);
    cfg_val = 0x320 | AIC_EARLY_DEBUG_UART_TX_GPIO_FUN;
    writel(cfg_val, cfg_reg);

    val = readl(CMU_UART_REG(id));
    if (val & 0x3100) {
        /* Wait for UART Tx FIFO to be empty, if UART already enabled */
        while ((readl(UART_LSR_REG(id)) & LSR_TX_EMP_BIT) == 0)
        {
            continue;
        }
        writel(0, CMU_UART_REG(id));
    };
    /* Reset and Gating UART */
    if (!get_best_uart_div(&cmu_div, &uart_div)) {
        writel(0x3100 | (cmu_div - 1), CMU_UART_REG(id));
    }
}

void early_debug_uart_init(void)
{
    u32 val = 0;
    int cmu_div = 0, uart_div = 0;

    val = readl(UART_HALT_REG(id));
    writel(val | 0x2, UART_HALT_REG(id));

    if (!get_best_uart_div(&cmu_div, &uart_div)) {
        val = readl(UART_LCR_REG(id));
        writel(val | 0x80, UART_LCR_REG(id));
        writel((uart_div) >> 8, UART_DLH_REG(id));
        writel((uart_div)&0xFF, UART_DLL_REG(id));
        writel(val & ~0x80, UART_LCR_REG(id));
    }

    val = readl(UART_HALT_REG(id));
    writel(val | 0x4, UART_HALT_REG(id));
    writel(val & (~0x2), UART_HALT_REG(id));

    writel(0, UART_IER_REG(id));

    val = ((PARITY & 0x03) << 3) | ((STOP & 0x01) << 2) | (DLEN & 0x03);
    writel(val, UART_LCR_REG(id));
    writel(0x7, UART_FCR_REG(id));
}

void early_debug_init(void)
{
    early_debug_board_init();

    early_debug_uart_init();
}

#define UART_BUSY_TIMEOUT 10000
void early_debug_putc(char c)
{
    u32 timecount = 0;

    while (!(readl(UART_LSR_REG(id)) & (0x1 << 5))) {
        timecount++;
        if (timecount >= UART_BUSY_TIMEOUT) {
            return;
        }
    }

    while (!(readl(UART_USR_REG(id)) & (0x1 << 1))) {
        continue;
    }
    writel(c, UART_THR_REG(id));
}

/**
 * This function is used to display a string on console, normally, it's
 * invoked by rt_kprintf
 *
 * @param str the displayed string
 */
void rt_hw_console_output(const char *str)
{
    while (*str) {
        if (*str == '\n') {
            early_debug_putc('\r');
        }

        early_debug_putc(*str++);
    }
}
