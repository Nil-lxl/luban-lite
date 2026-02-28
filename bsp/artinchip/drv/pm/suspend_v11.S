/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */
#include <rtconfig.h>
#include <aic_soc.h>

#define GTC_CNTVL       0x19050008
#define GTC_CNTVH       0x1905000C

.macro delay_100us
    li      s0, GTC_CNTVL
    li      s1, GTC_CNTVH
    lw      s2, (s0)     //start value 32bit low
    lw      s3, (s1)     //start value 32bit high
1:
    lw      s4, (s0)     //current value 32bit low
    lw      s5, (s1)     //current value 32bit high
    sub     s6, s4, s2   //s6 save 32bit low of sub
    sltu    s7, s4, s6   //s7 save carry
    sub     s8, s5, s3   //s8 save 32bit high of sub
    sub     s8, s8, s7   //s8 sub carry
    bnez    s8, 2f
    li      s4, 400      //GTC frequency is 4000000Hz, 100us counter 400
    bltu    s6, s4, 1b
2:
.endm

    .section .entry, "ax", %progbits
    .align  3
    .option pic
    .global aic_suspend_resume
aic_suspend_resume:
    sw      s0, (a0)
    sw      s1, 0x4(a0)
    sw      s2, 0x8(a0)
    sw      s3, 0xc(a0)
    sw      s4, 0x10(a0)
    sw      s5, 0x14(a0)
    sw      s6, 0x18(a0)
    sw      s7, 0x1c(a0)
    sw      s8, 0x20(a0)
    sw      s9, 0x24(a0)
    sw      s10, 0x28(a0)
    sw      s11, 0x2c(a0)

    /* PLL_FRA0 out mux bypass to 24M */
    li      t0, CMU_BASE
    lw      t1, 0x20(t0)
    li      t2, 0xFFEFFFFF
    and     t1, t1, t2
    sw      t1, 0x20(t0)
    /* Disable PLL_FRA0 out to sys */
    li      t2, 0xFFFBFFFF
    and     t1, t1, t2
    sw      t1, 0x20(t0)
    /* Disable PLL_FRA0 */
    li      t2, 0xFFFEFFFF
    and     t1, t1, t2
    sw      t1, 0x20(t0)

    wfi

    /* Enable PLL_FRA0 */
    li      t2, 0x10000
    or      t1, t1, t2
    sw      t1, 0x20(t0)
    delay_100us
    /* Enable PLL_FRA0 out to sys  */
    li      t2, 0x40000
    or      t1, t1, t2
    sw      t1, 0x20(t0)
    /* PLL_FRA0 out mux bypass to PLL_FRA0 */
    li      t2, 0x100000
    or      t1, t1, t2
    sw      t1, 0x20(t0)

    /* After the freq of XSPI is changed, the DLL of XSPI must be reset */
    li      t0, XSPI_BASE
    lw      t1, 0x14(t0)
    li      t2, 0xFFFFFFFE
    and     t1, t1, t2
    sw      t1, 0x14(t0)

    lw      t1, 0x1C(t0)
    li      t2, 0xFFFFFFFE
    and     t1, t1, t2
    sw      t1, 0x1C(t0)

    delay_100us

    lw      t1, 0x14(t0)
    ori     t1, t1, 1
    sw      t1, 0x14(t0)

    lw      t1, 0x1C(t0)
    ori     t1, t1, 1
    sw      t1, 0x1C(t0)

    delay_100us

    lw      s0, (a0)
    lw      s1, 0x4(a0)
    lw      s2, 0x8(a0)
    lw      s3, 0xc(a0)
    lw      s4, 0x10(a0)
    lw      s5, 0x14(a0)
    lw      s6, 0x18(a0)
    lw      s7, 0x1c(a0)
    lw      s8, 0x20(a0)
    lw      s9, 0x24(a0)
    lw      s10, 0x28(a0)
    lw      s11, 0x2c(a0)

    ret
aic_suspend_resume_end:

        .data
        .align 3
        .global aic_suspend_resume_size
aic_suspend_resume_size:
        .word       aic_suspend_resume_end - aic_suspend_resume
