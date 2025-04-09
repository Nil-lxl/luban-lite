#********************************************************************************************************
#                                              uC/OS-II
#                                        The Real-Time Kernel
#
#                    Copyright 1992-2020 Silicon Laboratories Inc. www.silabs.com
#
#                                 SPDX-License-Identifier: APACHE-2.0
#
#               This software is subject to an open source license and is distributed by
#                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
#                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
#
#********************************************************************************************************

#********************************************************************************************************
#
#                                              RISC-V Port
#
# Filename  : os_cpu_a.S
# Version   : V2.93.00
#********************************************************************************************************
# For       : RISC-V RV64
# Toolchain : GNU C Compiler
#********************************************************************************************************
# Note(s)   : Hardware FP is not supported.
#********************************************************************************************************
#include "cpuport.h"

#ifdef __ASSEMBLY__
#define __ASM_STR(x)    x
#else
#define __ASM_STR(x)    #x
#endif

#if __riscv_xlen == 64
    #define __REG_SEL(a, b)    __ASM_STR(a)
#elif __riscv_xlen == 32
    #define __REG_SEL(a, b)    __ASM_STR(b)
#else
    #error "Unexpected __riscv_xlen"
#endif

#define REG_L       __REG_SEL(ld, lw)
#define REG_S       __REG_SEL(sd, sw)
#define REG_ASM     __REG_SEL(.dword, .word)
#define SZREG       __REG_SEL(8, 4)
#define LGREG       __REG_SEL(3, 2)

#ifdef __riscv_flen

#if __riscv_flen == 64
    #define __FREG_SEL(a, b)    __ASM_STR(a)
#elif __riscv_flen == 32
    #define __FREG_SEL(a, b)    __ASM_STR(b)
#else
#error "Unexpected __riscv_flen"
#endif

#define FREG_L       __FREG_SEL(fld, flw)
#define FREG_S       __FREG_SEL(fsd, fsw)
#define FREG_ASM     __FREG_SEL(.dword, .word)
#define FSZREG       __FREG_SEL(8, 4)
#define FLGREG       __FREG_SEL(3, 2)

#endif /* __riscv_flen */

#********************************************************************************************************
#                                          PUBLIC FUNCTIONS
#********************************************************************************************************

    .extern  OSRunning                               # External references
    .extern  OSPrioCur
    .extern  OSPrioHighRdy
    .extern  OSTCBCur
    .extern  OSTCBHighRdy
    .extern  OSIntExit
    .extern  OSTaskSwHook


    .global  OSStartHighRdy                          # Functions declared in this file
    .global  OS_CPU_SR_Save
    .global  OS_CPU_SR_Restore
    .global  OSCtxSw
    .global  OSIntCtxSw
    .global  PendSV_Handler


#********************************************************************************************************
#                                               EQUATES
#********************************************************************************************************

    .equ  RISCV_MSTATUS_MIE,         0x08

    #.equ  TSPEND_ADDR,              (0x20000000 + 0x4000000)
#********************************************************************************************************
#                                     CODE GENERATION DIRECTIVES
#********************************************************************************************************

.section .text


#********************************************************************************************************
#                                 CRITICAL SECTION METHOD 3 FUNCTIONS
#
# Description: Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking you
#              would store the state of the interrupt disable flag in the local variable 'cpu_sr' and then
#              disable interrupts.  'cpu_sr' is allocated in all of uC/OS-II's functions that need to
#              disable interrupts.  You would restore the interrupt disable state by copying back 'cpu_sr'
#              into the CPU's status register.
#
# Prototypes :     OS_CPU_SR  OS_CPU_SR_Save(void);
#                  void       OS_CPU_SR_Restore(OS_CPU_SR cpu_sr);
#
#
# Note(s)    : 1) These functions are used in general like this:
#
#                 void Task (void *p_arg)
#                 {
#                 #if OS_CRITICAL_METHOD == 3          /* Allocate storage for CPU status register */
#                     OS_CPU_SR  cpu_sr;
#                 #endif
#
#                          :
#                          :
#                     OS_ENTER_CRITICAL();             /* cpu_sr = OS_CPU_SaveSR();                */
#                          :
#                          :
#                     OS_EXIT_CRITICAL();              /* OS_CPU_RestoreSR(cpu_sr);                */
#                          :
#                          :
#                 }
#********************************************************************************************************

OS_CPU_SR_Save:
# Save the Machine status register
    csrr   a0, mstatus

# Disable global interupt
    li     t0, RISCV_MSTATUS_MIE
    csrrc  zero, mstatus, t0
    ret

OS_CPU_SR_Restore:
# Restore the Machine status register previous state
    csrw   mstatus, a0
    ret


#********************************************************************************************************
#                                         START MULTITASKING
#                                      void OSStartHighRdy(void)
#
# Note(s) : 1) OSStartHighRdy() MUST:
#              a) Call OSTaskSwHook() then,
#              b) Set OSRunning to TRUE,
#              c) Set OSTCBHighRdy->OSTCBStkPtr = SP
#              d) Restore x1-x31; There is no need to restore x0 since it is always zero.
#              e) Enable interrupts (tasks will run with interrupts enabled).
#              f) switch to highest priority task.
#********************************************************************************************************

OSStartHighRdy:
# Disable interrupts
    li     t0, RISCV_MSTATUS_MIE
    csrrc  zero, mstatus, t0

# Execute OS task switch hook.
    jal    OSTaskSwHook

# OSRunning = TRUE;
    li     t0, 0x01
    la     t1, OSRunning
    sb     t0, 0(t1)

# SWITCH TO HIGHEST PRIORITY TASK
    la     t0, OSTCBHighRdy
    REG_L     t1, 0(t0)
    REG_L     sp, 0(t1)

# Retrieve the location where to jump
    REG_L     t0, 31 * SZREG(sp)
    csrw   mepc, t0

# Restore {x1, x4-x31} registers
    REG_L     ra,   0 * SZREG(sp)
    REG_L     t0,   4 * SZREG(sp)
    REG_L     t1,   5 * SZREG(sp)
    REG_L     t2,   6 * SZREG(sp)
    REG_L     s0,   7 * SZREG(sp)
    REG_L     s1,   8 * SZREG(sp)
    REG_L     a0,   9 * SZREG(sp)
    REG_L     a1,  10 * SZREG(sp)
    REG_L     a2,  11 * SZREG(sp)
    REG_L     a3,  12 * SZREG(sp)
    REG_L     a4,  13 * SZREG(sp)
    REG_L     a5,  14 * SZREG(sp)
    REG_L     a6,  15 * SZREG(sp)
    REG_L     a7,  16 * SZREG(sp)
    REG_L     s2,  17 * SZREG(sp)
    REG_L     s3,  18 * SZREG(sp)
    REG_L     s4,  19 * SZREG(sp)
    REG_L     s5,  20 * SZREG(sp)
    REG_L     s6,  21 * SZREG(sp)
    REG_L     s7,  22 * SZREG(sp)
    REG_L     s8,  23 * SZREG(sp)
    REG_L     s9,  24 * SZREG(sp)
    REG_L     s10, 25 * SZREG(sp)
    REG_L     s11, 26 * SZREG(sp)
    REG_L     t3,  27 * SZREG(sp)
    REG_L     t4,  28 * SZREG(sp)
    REG_L     t5,  29 * SZREG(sp)
    REG_L     t6,  30 * SZREG(sp)

# Compensate for the stack pointer
    addi   sp, sp, 32 * SZREG

#ifdef __riscv_flen
    FREG_L   f0, 0 * FSZREG(sp)
    FREG_L   f1, 1 * FSZREG(sp)
    FREG_L   f2, 2 * FSZREG(sp)
    FREG_L   f3, 3 * FSZREG(sp)
    FREG_L   f4, 4 * FSZREG(sp)
    FREG_L   f5, 5 * FSZREG(sp)
    FREG_L   f6, 6 * FSZREG(sp)
    FREG_L   f7, 7 * FSZREG(sp)
    FREG_L   f8, 8 * FSZREG(sp)
    FREG_L   f9, 9 * FSZREG(sp)
    FREG_L   f10, 10 * FSZREG(sp)
    FREG_L   f11, 11 * FSZREG(sp)
    FREG_L   f12, 12 * FSZREG(sp)
    FREG_L   f13, 13 * FSZREG(sp)
    FREG_L   f14, 14 * FSZREG(sp)
    FREG_L   f15, 15 * FSZREG(sp)
    FREG_L   f16, 16 * FSZREG(sp)
    FREG_L   f17, 17 * FSZREG(sp)
    FREG_L   f18, 18 * FSZREG(sp)
    FREG_L   f19, 19 * FSZREG(sp)
    FREG_L   f20, 20 * FSZREG(sp)
    FREG_L   f21, 21 * FSZREG(sp)
    FREG_L   f22, 22 * FSZREG(sp)
    FREG_L   f23, 23 * FSZREG(sp)
    FREG_L   f24, 24 * FSZREG(sp)
    FREG_L   f25, 25 * FSZREG(sp)
    FREG_L   f26, 26 * FSZREG(sp)
    FREG_L   f27, 27 * FSZREG(sp)
    FREG_L   f28, 28 * FSZREG(sp)
    FREG_L   f29, 29 * FSZREG(sp)
    FREG_L   f30, 30 * FSZREG(sp)
    FREG_L   f31, 31 * FSZREG(sp)

    addi    sp, sp, 32 * FSZREG
#endif

# Use register t6 to jump to HIGHEST priority
    csrr   t6, mepc

# Enable global interrupts
    li     t0, RISCV_MSTATUS_MIE
    csrrs  zero, mstatus, t0

# Jump to HIGHEST priority task.
    jalr   x0, t6, 0


#********************************************************************************************************
#                       PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
#                   PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
#
# Note(s) : 1) OSCtxSw() is called when OS wants to perform a task context switch.  This function
#              triggers a synchronous software interrupt by writing into the MSIP register
#
#           2) OSIntCtxSw() is called by OSIntExit() when it determines a context switch is needed as
#              the result of an interrupt.  This function triggers a synchronous software interrupt by
#              writing into the MSIP register
#********************************************************************************************************

OSCtxSw:
OSIntCtxSw:
    li      t0, TSPEND_ADDR
    lb      t1, (t0)
    li      t2, 0x01
    or      t1, t1, t2
    sb      t1, (t0)

    /* make sure wite instruction is complete */
    fence
    lb      t1, (t0)
    fence
#ifdef __riscv_xthead
    sync.is
#else
    .long   0x01b0000b
#endif

    ret

#********************************************************************************************************
#                                   void PendSV_Handler (void)
#
# Note(s) : 1) This function is defined with weak linking in 'riscv_hal_stubs.c' so that it can be
#              overridden by the kernel port with same prototype.
#
#           2) Pseudo-code is:
#              a) Save regs {x1, vxsat, x4-x31, mepc} on process stack
#              b) Clear soft interrupt.
#              c) Save the process SP in its TCB, OSTCBCur->OSTCBStkPtr = SP;
#              d) Call OSTaskSwHook();
#              e) Get current high priority, OSPrioCur = OSPrioHighRdy;
#              f) Get current ready thread TCB, OSTCBCur = OSTCBHighRdy;
#              g) Get new process SP from TCB, SP = OSTCBHighRdy->OSTCBStkPtr;
#              h) Retrieve the address at which exception happened
#              i) Restore {x1, vxsat, x4-x31, mepc} from new process stack; x0 is always zero.
#              j) Perform exception return which will restore remaining context.
#********************************************************************************************************

PendSV_Handler:
# Disable interrupts globally and prevent interruption during context switch
#    li     t0, RISCV_MSTATUS_MIE
#    csrrc  zero, mstatus, t0

#ifdef __riscv_flen
    addi    sp, sp, -32 * FSZREG

    FREG_S  f0, 0 * FSZREG(sp)
    FREG_S  f1, 1 * FSZREG(sp)
    FREG_S  f2, 2 * FSZREG(sp)
    FREG_S  f3, 3 * FSZREG(sp)
    FREG_S  f4, 4 * FSZREG(sp)
    FREG_S  f5, 5 * FSZREG(sp)
    FREG_S  f6, 6 * FSZREG(sp)
    FREG_S  f7, 7 * FSZREG(sp)
    FREG_S  f8, 8 * FSZREG(sp)
    FREG_S  f9, 9 * FSZREG(sp)
    FREG_S  f10, 10 * FSZREG(sp)
    FREG_S  f11, 11 * FSZREG(sp)
    FREG_S  f12, 12 * FSZREG(sp)
    FREG_S  f13, 13 * FSZREG(sp)
    FREG_S  f14, 14 * FSZREG(sp)
    FREG_S  f15, 15 * FSZREG(sp)
    FREG_S  f16, 16 * FSZREG(sp)
    FREG_S  f17, 17 * FSZREG(sp)
    FREG_S  f18, 18 * FSZREG(sp)
    FREG_S  f19, 19 * FSZREG(sp)
    FREG_S  f20, 20 * FSZREG(sp)
    FREG_S  f21, 21 * FSZREG(sp)
    FREG_S  f22, 22 * FSZREG(sp)
    FREG_S  f23, 23 * FSZREG(sp)
    FREG_S  f24, 24 * FSZREG(sp)
    FREG_S  f25, 25 * FSZREG(sp)
    FREG_S  f26, 26 * FSZREG(sp)
    FREG_S  f27, 27 * FSZREG(sp)
    FREG_S  f28, 28 * FSZREG(sp)
    FREG_S  f29, 29 * FSZREG(sp)
    FREG_S  f30, 30 * FSZREG(sp)
    FREG_S  f31, 31 * FSZREG(sp)
#endif

# Save regs {x1, vxsat, x4-x31, mepc} on process stack
    addi      sp,   sp, -SZREG * 32
    REG_S     ra,   0 * SZREG(sp)
    REG_S     t0,   4 * SZREG(sp)
    REG_S     t1,   5 * SZREG(sp)
    REG_S     t2,   6 * SZREG(sp)
    REG_S     s0,   7 * SZREG(sp)
    REG_S     s1,   8 * SZREG(sp)
    REG_S     a0,   9 * SZREG(sp)
    REG_S     a1,  10 * SZREG(sp)
    REG_S     a2,  11 * SZREG(sp)
    REG_S     a3,  12 * SZREG(sp)
    REG_S     a4,  13 * SZREG(sp)
    REG_S     a5,  14 * SZREG(sp)
    REG_S     a6,  15 * SZREG(sp)
    REG_S     a7,  16 * SZREG(sp)
    REG_S     s2,  17 * SZREG(sp)
    REG_S     s3,  18 * SZREG(sp)
    REG_S     s4,  19 * SZREG(sp)
    REG_S     s5,  20 * SZREG(sp)
    REG_S     s6,  21 * SZREG(sp)
    REG_S     s7,  22 * SZREG(sp)
    REG_S     s8,  23 * SZREG(sp)
    REG_S     s9,  24 * SZREG(sp)
    REG_S     s10, 25 * SZREG(sp)
    REG_S     s11, 26 * SZREG(sp)
    REG_S     t3,  27 * SZREG(sp)
    REG_S     t4,  28 * SZREG(sp)
    REG_S     t5,  29 * SZREG(sp)
    REG_S     t6,  30 * SZREG(sp)
    csrr      t0,  mepc
    REG_S     t0,  31 * SZREG(sp)

#ifdef __riscv_dsp
    csrr    t0, vxsat
    REG_S   t0, 3 * REGBYTES(sp)
#endif

    /* clear software interrupt */
    li        t0, TSPEND_ADDR
    li        t1, 0x0
    REG_S     t1, (t0)
    /* make sure wite instruction is complete */
    fence
    lb        t1, (t0)

# Stack pointer was passed by 'entry.s' in register a2.
# OSTCBCur->OSTCBStkPtr = SP;
    la        t0, OSTCBCur
    REG_L     t1, 0(t0)
    REG_S     sp, 0(t1)

# Execute OS task switch hook.
    jal       OSTaskSwHook

# OSPrioCur = OSPrioHighRdy;
    la        t0, OSPrioHighRdy
    lb        t1, 0(t0)
    la        t0, OSPrioCur
    sb        t1, 0(t0)

# OSTCBCur = OSTCBHighRdy;
    la        t0, OSTCBHighRdy
    REG_L     t1, 0(t0)
    la        t0, OSTCBCur
    REG_S     t1, 0(t0)

# SP = OSTCBHighRdy->OSTCBStkPtr;
    REG_L     sp, 0(t1)

# Retrieve the address at which exception happened
    REG_L     t0, 31 * SZREG(sp)
    csrw      mepc, t0

#ifdef __riscv_dsp
    REG_L    t0, 3 * SZREG(sp)
    csrw    vxsat, t0
#endif

# Restore x1 to x31 registers
    REG_L     ra,   0 * SZREG(sp)
    REG_L     t0,   4 * SZREG(sp)
    REG_L     t1,   5 * SZREG(sp)
    REG_L     t2,   6 * SZREG(sp)
    REG_L     s0,   7 * SZREG(sp)
    REG_L     s1,   8 * SZREG(sp)
    REG_L     a0,   9 * SZREG(sp)
    REG_L     a1,  10 * SZREG(sp)
    REG_L     a2,  11 * SZREG(sp)
    REG_L     a3,  12 * SZREG(sp)
    REG_L     a4,  13 * SZREG(sp)
    REG_L     a5,  14 * SZREG(sp)
    REG_L     a6,  15 * SZREG(sp)
    REG_L     a7,  16 * SZREG(sp)
    REG_L     s2,  17 * SZREG(sp)
    REG_L     s3,  18 * SZREG(sp)
    REG_L     s4,  19 * SZREG(sp)
    REG_L     s5,  20 * SZREG(sp)
    REG_L     s6,  21 * SZREG(sp)
    REG_L     s7,  22 * SZREG(sp)
    REG_L     s8,  23 * SZREG(sp)
    REG_L     s9,  24 * SZREG(sp)
    REG_L     s10, 25 * SZREG(sp)
    REG_L     s11, 26 * SZREG(sp)
    REG_L     t3,  27 * SZREG(sp)
    REG_L     t4,  28 * SZREG(sp)
    REG_L     t5,  29 * SZREG(sp)
    REG_L     t6,  30 * SZREG(sp)

    addi   sp, sp, SZREG * 32

#ifdef __riscv_flen
    FREG_L   f0, 0 * FSZREG(sp)
    FREG_L   f1, 1 * FSZREG(sp)
    FREG_L   f2, 2 * FSZREG(sp)
    FREG_L   f3, 3 * FSZREG(sp)
    FREG_L   f4, 4 * FSZREG(sp)
    FREG_L   f5, 5 * FSZREG(sp)
    FREG_L   f6, 6 * FSZREG(sp)
    FREG_L   f7, 7 * FSZREG(sp)
    FREG_L   f8, 8 * FSZREG(sp)
    FREG_L   f9, 9 * FSZREG(sp)
    FREG_L   f10, 10 * FSZREG(sp)
    FREG_L   f11, 11 * FSZREG(sp)
    FREG_L   f12, 12 * FSZREG(sp)
    FREG_L   f13, 13 * FSZREG(sp)
    FREG_L   f14, 14 * FSZREG(sp)
    FREG_L   f15, 15 * FSZREG(sp)
    FREG_L   f16, 16 * FSZREG(sp)
    FREG_L   f17, 17 * FSZREG(sp)
    FREG_L   f18, 18 * FSZREG(sp)
    FREG_L   f19, 19 * FSZREG(sp)
    FREG_L   f20, 20 * FSZREG(sp)
    FREG_L   f21, 21 * FSZREG(sp)
    FREG_L   f22, 22 * FSZREG(sp)
    FREG_L   f23, 23 * FSZREG(sp)
    FREG_L   f24, 24 * FSZREG(sp)
    FREG_L   f25, 25 * FSZREG(sp)
    FREG_L   f26, 26 * FSZREG(sp)
    FREG_L   f27, 27 * FSZREG(sp)
    FREG_L   f28, 28 * FSZREG(sp)
    FREG_L   f29, 29 * FSZREG(sp)
    FREG_L   f30, 30 * FSZREG(sp)
    FREG_L   f31, 31 * FSZREG(sp)

    addi    sp, sp, 32 * FSZREG
#endif

# Exception return will restore remaining context
    mret


#********************************************************************************************************
#                                             MODULE END
#*********************************************************************************************************
