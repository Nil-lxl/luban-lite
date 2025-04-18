/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include <cpuport.h>
#include <aic_soc.h>

/* Enable interrupts when returning from the handler */
#define MSTATUS_PRV1 0x3880

#ifdef QEMU_RUN
#define PLIC_H0_MCLAIM 0x4000200004
#else
#define PLIC_H0_MCLAIM 0x20200004
#endif

.section .bss

    .align  3
    .global g_base_irqstack
    .global g_top_irqstack
g_base_irqstack:
    .space AIC_INTERRUPTSTACK_SIZE
g_top_irqstack:

    .align 2
    .globl g_trap_sp
    .type  g_trap_sp, object
g_trap_sp:
    .long 0
    .size g_trap_sp, .-g_trap_sp

#ifdef KERNEL_BAREMETAL
    .align  3
    .global g_base_normalstack
    .global g_top_normalstack
g_base_normalstack:
    .space AIC_NORMALSTACK_SIZE
g_top_normalstack:
#endif

__g_irq_num:
    .long 0

.text

    .align  2
    .global Default_IRQHandler
    .weak   Default_IRQHandler
    .type   Default_IRQHandler, %function
Default_IRQHandler:
    /* Switch sp to irqstack */
    csrw    mscratch, sp
    la      sp, g_top_irqstack

    /* ipush */
    addi    sp, sp, -(18 * REGBYTES)
    STORE   x1, 1 * REGBYTES(sp)    // ra
    csrr    x1, mepc
    STORE   x1, 0 * REGBYTES(sp)    // mepc
    csrr    x1, mstatus
    STORE   x1, 2 * REGBYTES(sp)	// mstatus
    STORE   x5, 3 * REGBYTES(sp)    // t0
    STORE   x6, 4 * REGBYTES(sp)    // t1
    STORE   x7, 5 * REGBYTES(sp)    // t2
    STORE   x10, 6 * REGBYTES(sp)   // a0
    STORE   x11, 7 * REGBYTES(sp)   // a1
    STORE   x12, 8 * REGBYTES(sp)   // a2
    STORE   x13, 9 * REGBYTES(sp)   // a3
    STORE   x14, 10 * REGBYTES(sp)  // a4
    STORE   x15, 11 * REGBYTES(sp)  // a5
    STORE   x16, 12 * REGBYTES(sp)  // a6
    STORE   x17, 13 * REGBYTES(sp)  // a7
    STORE   x28, 14 * REGBYTES(sp)  // t3
    STORE   x29, 15 * REGBYTES(sp)  // t4
    STORE   x30, 16 * REGBYTES(sp)  // t5
    STORE   x31, 17 * REGBYTES(sp)  // t6

#ifdef ARCH_RISCV_FPU
    addi    sp, sp, -(20 * FREGBYTES)

    FSTORE  ft0,   0 * FREGBYTES(sp)
    FSTORE  ft1,   1 * FREGBYTES(sp)
    FSTORE  ft2,   2 * FREGBYTES(sp)
    FSTORE  ft3,   3 * FREGBYTES(sp)
    FSTORE  ft4,   4 * FREGBYTES(sp)
    FSTORE  ft5,   5 * FREGBYTES(sp)
    FSTORE  ft6,   6 * FREGBYTES(sp)
    FSTORE  ft7,   7 * FREGBYTES(sp)
    FSTORE  fa0,   8 * FREGBYTES(sp)
    FSTORE  fa1,   9 * FREGBYTES(sp)
    FSTORE  fa2,  10 * FREGBYTES(sp)
    FSTORE  fa3,  11 * FREGBYTES(sp)
    FSTORE  fa4,  12 * FREGBYTES(sp)
    FSTORE  fa5,  13 * FREGBYTES(sp)
    FSTORE  fa6,  14 * FREGBYTES(sp)
    FSTORE  fa7,  15 * FREGBYTES(sp)
    FSTORE  ft8,  16 * FREGBYTES(sp)
    FSTORE  ft9,  17 * FREGBYTES(sp)
    FSTORE  ft10, 18 * FREGBYTES(sp)
    FSTORE  ft11, 19 * FREGBYTES(sp)
#endif

#if defined(_ENABLE_BACK_TRACE_STACK_) && defined(_NO_OMIT_FRAME_POINT_)
    addi    sp,sp,-16
    STORE   s0,(sp)
    csrr    a0, mepc
    STORE   a0, 8(sp)
    addi    s0, sp, 16
#endif


#if defined(KERNEL_RTTHREAD)
    la      t0, rt_interrupt_enter
    jalr    t0
#elif defined(KERNEL_FREERTOS)
    la      t0, aicos_irq_enter
    jalr    t0
#elif defined(KERNEL_UCOS_II)
    la      t0, OSIntEnter
    jalr    t0
#endif

    /* call isr */
    /* t1 = irq_num */
    csrr    t1, mcause
    andi    t1, t1, 0x3FF
    li      t2, 0x7        /* CORET_IRQn */
    beq     t1, t2, .no_ext_irq
    /* read external irq num from PLIC_H0_MCLAIM */
    li      t0, PLIC_H0_MCLAIM
    lw      t1, (t0)
    la      t0, __g_irq_num
    sw      t1, (t0)
    j       .call_isr
.no_ext_irq:
    la      t0, __g_irq_num
    sw      x0, (t0)
.call_isr:
    slli    t1, t1, 3
    /* irq_handler_func(): t2 = g_irqvector[irq_num] */
    la      t0, g_irqvector
    add     t0, t0, t1
    LOAD    t2, (t0)
    /* para1 : a0 = irq_num */
    la      t0, __g_irq_num
    lw      a0, (t0)
    /* para2 : a1 = g_irqdata[irq_num] = irq_data */
    la      t0, g_irqdata
    add     t0, t0, t1
    LOAD    a1, (t0)
    /* g_irqcnt[irq_num]++ */
    la      t0, g_irqcnt
    srli    t1, t1, 1
    add     a3, t0, t1
    lw      t0, (a3)
    addi    t0, t0, 1
    sw      t0, (a3)

    /* call irq_handler_func(irq_num, irq_data) */
    jalr    t2

#if defined(KERNEL_RTTHREAD)
    la      t0, rt_interrupt_leave
    jalr    t0
#elif defined(KERNEL_FREERTOS)
    la      t0, aicos_irq_exit
    jalr    t0
#elif defined(KERNEL_UCOS_II)
    la      t0, OSIntExit
    jalr    t0
#endif

    /* write PLIC_H0_MCLAIM and exit interrupt */
    la      t0, __g_irq_num
    lw      a0, (t0)
    beqz    a0, .no_ext_irq_exit
    li      t0, PLIC_H0_MCLAIM
    sw      a0, (t0)
.no_ext_irq_exit:

#if defined(_ENABLE_BACK_TRACE_STACK_) && defined(_NO_OMIT_FRAME_POINT_)
    lw    s0, (sp)
    addi  sp, sp, 16
#endif

#ifdef ARCH_RISCV_FPU
    FLOAD   ft0,  0 * FREGBYTES(sp)
    FLOAD   ft1,  1 * FREGBYTES(sp)
    FLOAD   ft2,  2 * FREGBYTES(sp)
    FLOAD   ft3,  3 * FREGBYTES(sp)
    FLOAD   ft4,  4 * FREGBYTES(sp)
    FLOAD   ft5,  5 * FREGBYTES(sp)
    FLOAD   ft6,  6 * FREGBYTES(sp)
    FLOAD   ft7,  7 * FREGBYTES(sp)
    FLOAD   fa0,  8 * FREGBYTES(sp)
    FLOAD   fa1,  9 * FREGBYTES(sp)
    FLOAD   fa2, 10 * FREGBYTES(sp)
    FLOAD   fa3, 11 * FREGBYTES(sp)
    FLOAD   fa4, 12 * FREGBYTES(sp)
    FLOAD   fa5, 13 * FREGBYTES(sp)
    FLOAD   fa6, 14 * FREGBYTES(sp)
    FLOAD   fa7, 15 * FREGBYTES(sp)
    FLOAD   ft8, 16 * FREGBYTES(sp)
    FLOAD   ft9, 17 * FREGBYTES(sp)
    FLOAD   ft10,18 * FREGBYTES(sp)
    FLOAD   ft11,19 * FREGBYTES(sp)

    addi    sp, sp, (20 * FREGBYTES)
#endif
    /* ipop */
    LOAD    x1, 0 * REGBYTES(sp)    // mepc
    csrw    mepc, x1
    LOAD    x1, 2 * REGBYTES(sp)    // mstatus
    csrw    mstatus, x1
    LOAD    x1, 1 * REGBYTES(sp)    // ra
    LOAD    x5, 3 * REGBYTES(sp)    // t0
    LOAD    x6, 4 * REGBYTES(sp)    // t1
    LOAD    x7, 5 * REGBYTES(sp)    // t2
    LOAD    x10, 6 * REGBYTES(sp)   // a0
    LOAD    x11, 7 * REGBYTES(sp)   // a1
    LOAD    x12, 8 * REGBYTES(sp)   // a2
    LOAD    x13, 9 * REGBYTES(sp)   // a3
    LOAD    x14, 10 * REGBYTES(sp)  // a4
    LOAD    x15, 11 * REGBYTES(sp)  // a5
    LOAD    x16, 12 * REGBYTES(sp)  // a6
    LOAD    x17, 13 * REGBYTES(sp)  // a7
    LOAD    x28, 14 * REGBYTES(sp)  // t3
    LOAD    x29, 15 * REGBYTES(sp)  // t4
    LOAD    x30, 16 * REGBYTES(sp)  // t5
    LOAD    x31, 17 * REGBYTES(sp)  // t6

    addi    sp, sp, (18 * REGBYTES)

    /* Restore sp to normal stack */
    csrr    sp, mscratch
    mret

/******************************************************************************
 * Functions:
 *     void trap(void);
 * default exception handler
 ******************************************************************************/
    .align  2
    .global trap
    .type   trap, %function
trap:
#ifndef AIC_BACKTRACE_DEBUG
    csrw    mscratch, sp
    la      t0, g_trap_sp
#else
    mv       t0, sp
#endif
    addi    t0, t0, -(264)
    STORE   x1, 0(t0)
    STORE   x2, 8(t0)
    STORE   x3, 16(t0)
    STORE   x4, 24(t0)
    STORE   x5, 32(t0)
    STORE   x6, 40(t0)
    STORE   x7, 48(t0)
    STORE   x8, 56(t0)
    STORE   x9, 64(t0)
    STORE   x10,72(t0)
    STORE   x11,80(t0)
    STORE   x12,88(t0)
    STORE   x13,96(t0)
    STORE   x14,104(t0)
    STORE   x15,112(t0)
    STORE   x16,120(t0)
    STORE   x17,128(t0)
    STORE   x18,136(t0)
    STORE   x19,144(t0)
    STORE   x20,152(t0)
    STORE   x21,160(t0)
    STORE   x22,168(t0)
    STORE   x23,176(t0)
    STORE   x24,184(t0)
    STORE   x25,192(t0)
    STORE   x26,200(t0)
    STORE   x27,208(t0)
    STORE   x28,216(t0)
    STORE   x29,224(t0)
    STORE   x30,232(t0)
    STORE   x31,240(t0)
    csrr    a0, mepc
    STORE   a0, (248)(t0)
    csrr    a0, mstatus
    STORE   a0, (256)(t0)

    mv      a0, t0
    mv      sp, a0

    la      a5, trap_c
    jalr    a5

#if defined(_ENABLE_BACK_TRACE_STACK_) && defined(KERNEL_RTTHREAD)
    mv      a0, sp
    addi    a0, a0, 264
    csrr    a1, mscratch
    csrr    a2, mepc
    la      a5, print_stack
    jalr    a5
#define CALL_STACK_MAX_DEPTH 32
    la      a0, rt_current_thread
    beqz    a0, .
#ifdef _NO_OMIT_FRAME_POINT_
    mv      t0, sp
    la      t1, CALL_STACK_MAX_DEPTH
    slli    t1, t1, 3
    sub     t0, t0, t1
    csrr    a1, mepc
    STORE   a1, (t0)
    mv      a1, s0
    li      t1, 1
1:
    // check addr
    li      t2, 0x40000000
    la      t3, AIC_DRAM_TOTAL_SIZE
    add     t3, t2, t3
    blt     a1, t2, 3f
    bge     a1, t3, 3f

    addi    a2, a1, -(8)
    blt     a2, t2, 3f
    bge     a2, t3, 3f

    addi    a2, a1, -(16)
    blt     a2, t2, 3f
    bge     a2, t3, 3f
2:
    // get ra
    LOAD    a2, -8(a1)
    slli    t2, t1, 3
    add     t2, t2, t0
    STORE   a2, (t2)
    // get next fp
    LOAD    a1, -16(a1)
    //inc call depth
    addi    t1, t1, 1
    // check end
    li  a2, 0xdeadbeef
    beq a1, a2, 3f
    la  a2, CALL_STACK_MAX_DEPTH
    blt t1, a2, 1b
3:
    mv      a0, t1
    mv      a1, t0
    mv      sp, t0
    la      a5, print_back_trace
    jalr    a5
#else
    mv      a0, sp
    addi    a0, a0, 264
    csrr    a1, mscratch
    csrr    a2, mepc
    la      a5, backtrace_call_stack
    jalr    a5
#endif
#endif

    j      .

    .align  6
    .weak   Default_Handler
    .global Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    j       trap

    .size   Default_Handler, . - Default_Handler

#ifdef KERNEL_BAREMETAL
    .align  6
    .weak   PendSV_Handler
    .global PendSV_Handler
    .type   PendSV_Handler, %function
PendSV_Handler:
    j       trap

    .size   PendSV_Handler, . - PendSV_Handler
#endif
