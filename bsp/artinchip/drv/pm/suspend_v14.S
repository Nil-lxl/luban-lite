/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */
#include <rtconfig.h>

#define DDRC_BASE               0x18400000
#define DDR_PHY_BASE            0x18500000
#define GTC_CNTVL               0x09050008
#define GTC_CNTVH               0x0905000C
#define RTC_GPIO_CFG            0x090300EC
#define PRCM_AP_CORE_CFG        0x08020D80

.macro delay_200us
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
    li      s4, 800      //GTC frequency is 4000000Hz, 200us counter 800
    bltu    s6, s4, 1b
2:
.endm

    .section .entry, "ax", %progbits
    .align  3
    .option pic
    .global aic_suspend_resume
aic_suspend_resume:
    //step1: DDR enter self refresh
    //ddr self-refresh flow
    li      t0, DDRC_BASE
    li      t1, DDR_PHY_BASE

    //disable DDR port
    li      t2, 0
    li      t3, 0
    li      t5, 5
    addi    t0, t0, 0x490
port_close_loop:
    add     t0, t0, t2
    sw      zero, (t0)
    addi    t2, t2, 0xb0
    addi    t3, t3, 1
    bltu    t3, t5, port_close_loop

    //enter self refresh
    li      t0, DDRC_BASE
    lw      t1, 0x30(t0)
    ori     t1, t1, 0x21
    sw      t1, 0x30(t0)

#ifndef FPGA_BOARD_ARTINCHIP
    //Setting AP_GPIO_PAD_HOLD
    li      t0, RTC_GPIO_CFG
    sw      0xFF, (t0)
    //Setting AP_CORE_ISO
    li      t0, PRCM_AP_CORE_CFG
    lw      t1, (t0)
    li      t2, 0x10000
    or      t1, t1, t2
    sw      t1, (t0)
    //Reset AP_CORE
    li      t2, 0xFEFFFFFF
    and     t1, t1, t2
    sw      t1, (t0)
    //TODO: verified on IC

#endif

    wfi

#ifndef FPGA_BOARD_ARTINCHIP
    //Release AP_CORE
    li      t0, PRCM_AP_CORE_CFG
    lw      t1, (t0)
    li      t2, 0x1000000
    or      t1, t1, t2
    sw      t1, (t0)
    //Clear AP_CORE_ISO
    li      t2, 0xFFFEFFFF
    and     t1, t1, t2
    sw      t1, (t0)
    //disable AP_GPIO_PAD_HOLD
    li      t0, RTC_GPIO_CFG
    sw      0,  (t0)
    //TODO: verified on IC

#endif
    //DDR exit self-refresh
    li      t0, DDRC_BASE
    lw      t2, 0x30(t0)
    li      t3, 0xDE
    and     t2, t2, t3
    sw      t2, 0x30(t0) //exit self-refresh
check_ddr_exit_sr_status:
    lw      t2, 0x4(t0)
    andi    t2, t2, 0x3
    li      t3, 0x1
    bne     t2, t3, check_ddr_exit_sr_status

    delay_200us

    li      t2, 0
    li      t3, 0
    li      t4, 1
    li      t5, 5
    addi    t0, t0, 0x490
port_open_loop:
    add     t0, t0, t2
    sw      t4, (t0)
    addi    t2, t2, 0xb0
    addi    t3, t3, 1
    bltu    t3, t5, port_open_loop
aic_suspend_resume_end:

    .data
    .align  3
    .global aic_suspend_resume_size
aic_suspend_resume_size:
    .word   aic_suspend_resume_end - aic_suspend_resume
