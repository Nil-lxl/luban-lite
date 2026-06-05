/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include <rtconfig.h>
#include <aic_soc.h>

#define RTC_WR_KEY            (RTC_BASE + 0x0fc)
#define RTC_ANALOG3           (RTC_BASE + 0x05c)
#define RTC_SLEEP_CFG         (RTC_BASE + 0x014)
#define CMU_GTC               (CMU_BASE + 0x90c)
#define GTC_CNTVL             (GTC_BASE + 0x008)
#define GTC_CNTVH             (GTC_BASE + 0x00C)

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

.macro pll_write_auth_offset20 val, base
    // record the GTC start time
    li      t3,   GTC_CNTVL
    lw      t5,   (t3)               // start low
    li      t3,   GTC_CNTVH
    lw      t6,   (t3)               // start high

    // write the auth to PLL_FRA0
    li      t3,   0xFE8
    add     t3,   \base,    t3             // t3 = base + 0xFE8
    li      t4,   (0xA1C << 20) | 0x20     // 0xA1C00020
    sw      t4,   0(t3)

    // wait the auth finish (10ms = 40000 GTC count)
1:
    li      t3,   0xFE8
    add     t3,   \base,   t3
    lw      t4,   0(t3)
    srli    t4,   t4,   16
    andi    t4,   t4,   1
    bnez    t4,   2f                       // auth done, jump to write

    // record the GTC current time
    li      t3,    GTC_CNTVL
    lw      t4,    (t3)                    // current low
    li      t3,    GTC_CNTVH
    lw      t3,    (t3)                    // current high

    // high bit different
    bne     t3,   t6,   3f                 // timeout

    // calculate low bit diff
    sub     t4,   t4,   t5                 // diff = current low - start low
    li      t3,   40000                    // 10ms = 40000 counts
    bltu    t4,   t3,   1b                 // < 40000 keep waiting

    // timeout
3:
    //nop
2:
    sw      \val,   0x20(\base)            // write the register
.endm

.macro unlock
    li      t3,   RTC_WR_KEY
    li      t4,   0xac
    sw      t4,   0(t3)
    fence
.endm

.macro lock
    li      t3,   RTC_WR_KEY
    li      t4,   0x0
    sw      t4,   0(t3)
    fence
.endm

    .section .entry, "ax", %progbits
    .align  3
    .option pic
    .global light_suspend_resume
light_suspend_resume:
    sw      s0,    (a0)
    sw      s1,    0x4(a0)
    sw      s2,    0x8(a0)
    sw      s3,    0xc(a0)
    sw      s4,    0x10(a0)
    sw      s5,    0x14(a0)
    sw      s6,    0x18(a0)
    sw      s7,    0x1c(a0)
    sw      s8,    0x20(a0)
    sw      s9,    0x24(a0)
    sw      s10,   0x28(a0)
    sw      s11,   0x2c(a0)

    li      t0,   CMU_BASE
    lw      t1,   0x20(t0)

    /* PLL_FRA0 out mux bypass to 24M */
    li      t2,   0xFFEFFFFF
    and     t1,   t1,    t2
    pll_write_auth_offset20   t1,   t0
    /* Disable PLL_FRA0 out to sys */
    li      t2,   0xFFFBFFFF
    and     t1,   t1,   t2
    pll_write_auth_offset20   t1,   t0
    /* Disable PLL_FRA0 */
    li      t2,   0xFFFEFFFF
    and     t1,   t1,   t2
    pll_write_auth_offset20   t1,   t0

    //li      t3,   SYSCFG_BASE
    //lw      t4,   0x024(t3)
    //
    ///* Disable PSRAM LDO18 */
    //li      t5,   0xFFFFFFEF
    //and     t4,   t4,   t5
    //sw      t4,   0x024(t3)

    wfi

    ///* Enable PSRAM LD018 */
    //li      t5,   0x10
    //or      t4,   t4,   t5
    //sw      t4,   0x024(t3)
    /* Enable PLL_FRA0 */
    li      t2,   0x10000
    or      t1,   t1,   t2
    pll_write_auth_offset20   t1,   t0
    delay_100us
    /* Enable PLL_FRA0 out to sys */
    li      t2,   0x40000
    or      t1,   t1,   t2
    pll_write_auth_offset20   t1,   t0
    /* PLL_FRA0 out mux bypass to PLL_FRA0 */
    li      t2,   0x100000
    or      t1,   t1,   t2
    pll_write_auth_offset20   t1,   t0

    /* After the freq of XSPI is changed, the DLL of XSPI must be reset */
    li      t0,    XSPI_BASE
    lw      t1,    0x14(t0)
    li      t2,    0xFFFFFFFE
    and     t1,    t1, t2
    sw      t1,    0x14(t0)

    lw      t1,    0x1C(t0)
    li      t2,    0xFFFFFFFE
    and     t1,    t1, t2
    sw      t1,    0x1C(t0)

    delay_100us

    lw      t1,    0x14(t0)
    ori     t1,    t1, 1
    sw      t1,    0x14(t0)

    lw      t1,    0x1C(t0)
    ori     t1,    t1, 1
    sw      t1,    0x1C(t0)

    delay_100us

    lw      s0,    (a0)
    lw      s1,    0x4(a0)
    lw      s2,    0x8(a0)
    lw      s3,    0xc(a0)
    lw      s4,    0x10(a0)
    lw      s5,    0x14(a0)
    lw      s6,    0x18(a0)
    lw      s7,    0x1c(a0)
    lw      s8,    0x20(a0)
    lw      s9,    0x24(a0)
    lw      s10,   0x28(a0)
    lw      s11,   0x2c(a0)

    ret

light_suspend_resume_end:
        .data
        .align 3
        .global light_suspend_resume_size
light_suspend_resume_size:
        .word       light_suspend_resume_end - light_suspend_resume

    .section .entry, "ax", %progbits
    .align 3
    .option pic
    .global save_context_and_suspend
save_context_and_suspend:
    /* save General-purpose reg */
    sw      ra,   0(a0)
    sw      sp,   4(a0)
    sw      gp,   8(a0)
    sw      tp,   12(a0)
    sw      s0,   16(a0)
    sw      s1,   20(a0)
    sw      a1,   24(a0)
    sw      a2,   28(a0)
    sw      a3,   32(a0)
    sw      a4,   36(a0)
    sw      a5,   40(a0)
    sw      a6,   44(a0)
    sw      a7,   48(a0)
    sw      s2,   52(a0)
    sw      s3,   56(a0)
    sw      s4,   60(a0)
    sw      s5,   64(a0)
    sw      s6,   68(a0)
    sw      s7,   72(a0)
    sw      s8,   76(a0)
    sw      s9,   80(a0)
    sw      s10,  84(a0)
    sw      s11,  88(a0)

    /* save CSRs */
    csrr    t0,   mepc
    sw      t0,   92(a0)
    csrr    t0,   mstatus
    sw      t0,   96(a0)
    csrr    t0,   mtval
    sw      t0,   100(a0)
    csrr    t0,   mcause
    sw      t0,   104(a0)
    csrr    t0,   mie
    sw      t0,   108(a0)
    csrr    t0,   mtvec
    sw      t0,   112(a0)
    csrr    t0,   mtvt
    sw      t0,   116(a0)
    csrr    t0,   mscratch
    sw      t0,   120(a0)
    csrr    t0,   mxstatus
    sw      t0,   124(a0)
    csrr    t0,   mhcr
    sw      t0,   128(a0)
    csrr    t0,   mhint
    sw      t0,   132(a0)
    csrr    t0,   mexstatus
    sw      t0,   136(a0)

    /* record the GTC time */
    li      t0,   GTC_BASE
    li      t1,   0x0000
    sw      t1,   0(t0)
    li      t1,   GTC_CNTVL
    lw      t2,   0(t1)
    li      t1,   GTC_CNTVH
    lw      t3,   0(t1)
    sw      t2,   140(a0)
    sw      t3,   144(a0)

    fence.i
    fence

    /* psram pad hold configuration */
    unlock
    li      t0,   RTC_ANALOG3
    lw      t1,   0(t0)
    li      t2,   0x8
    or      t1,   t1, t2
    sw      t1,   0(t0)
    lock

    /* system suspend */
    unlock
    li      t0, RTC_SLEEP_CFG
    lw      t1, 0(t0)
    li      t2, 0x10
    or      t1, t1, t2
    sw      t1, 0(t0)
    lock

save_context_and_suspend_end:
    .data
    .align  3
    .global save_context_and_suspend_size
save_context_and_suspend_size:
    .word   save_context_and_suspend_end - save_context_and_suspend

    .section .entry, "ax", %progbits
    .align 3
    .option pic
    .global restore_context_and_resume
restore_context_and_resume:
    /* get the context addr */
    la      a0,   save_context

    /* rewrite the GTC time */
    li      t0,   CMU_GTC
    li      t1,   0x1100000
    sw      t1,   0(t0)
    lw      t2,   140(a0)
    lw      t3,   144(a0)
    li      t0,   GTC_BASE
    li      t1,   0x0
    sw      t1,   0(t0)
    li      t1,   GTC_CNTVH
    sw      t3,   0(t1)
    li      t1,   GTC_CNTVL
    sw      t2,   0(t1)
    li      t1,   0x1
    sw      t1,   0(t0)

    /* restore General-purpose reg */
    lw      ra,   0(a0)
    lw      sp,   4(a0)
    lw      gp,   8(a0)
    lw      tp,   12(a0)
    lw      s0,   16(a0)
    lw      s1,   20(a0)
    lw      a1,   24(a0)
    lw      a2,   28(a0)
    lw      a3,   32(a0)
    lw      a4,   36(a0)
    lw      a5,   40(a0)
    lw      a6,   44(a0)
    lw      a7,   48(a0)
    lw      s2,   52(a0)
    lw      s3,   56(a0)
    lw      s4,   60(a0)
    lw      s5,   64(a0)
    lw      s6,   68(a0)
    lw      s7,   72(a0)
    lw      s8,   76(a0)
    lw      s9,   80(a0)
    lw      s10,  84(a0)
    lw      s11,  88(a0)

    /* restore CSRs */
    lw      t0,   92(a0)
    csrw    mepc, t0
    lw      t0,   96(a0)
    csrw    mstatus, t0
    lw      t0,   100(a0)
    csrw    mtval, t0
    lw      t0,   104(a0)
    csrw    mcause, t0
    lw      t0,   108(a0)
    csrw    mie, t0
    lw      t0,   112(a0)
    csrw    mtvec, t0
    lw      t0,   116(a0)
    csrw    mtvt, t0
    lw      t0,   120(a0)
    csrw    mscratch, t0
    lw      t0,   124(a0)
    csrw    mxstatus, t0
    lw      t0,   128(a0)
    csrw    mhcr, t0
    lw      t0,   132(a0)
    csrw    mhint, t0
    lw      t0,   136(a0)
    csrw    mexstatus, t0

    ret

restore_context_and_resume_end:
    .data
    .align  3
    .global restore_context_and_resume_size
restore_context_and_resume_size:
    .word   restore_context_and_resume_end - restore_context_and_resume

