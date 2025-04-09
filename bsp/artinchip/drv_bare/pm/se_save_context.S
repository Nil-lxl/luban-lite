/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * Authors:
 *   dwj <weijie.ding@artinchip.com>
 */

#define PRCM_SESS_CPU_STATUS        0x88000114
#define PRCM_SE_CONTEXT_ADDR        0x88000120

    .section .entry, "ax", %progbits
    .align  3
    .global se_save_context_and_suspend
se_save_context_and_suspend:
    stm     r1-r15, (r0)    //save r1-r15
    mfcr    r1, psr         //save psr
    stw     r1, (r0, 60)
    mfcr    r1, vbr         //save vbr
    stw     r1, (r0, 64)
    mfcr    r1, epsr        //save epsr
    stw     r1, (r0, 68)
    mfcr    r1, epc        //save epc
    stw     r1, (r0, 72)
    mfcr    r1, gcr        //save gcr
    stw     r1, (r0, 76)
    mfcr    r1, gsr        //save gsr
    stw     r1, (r0, 80)
    mfcr    r1, ccr        //save ccr
    stw     r1, (r0, 84)
    mfcr    r1, capr        //save capr
    stw     r1, (r0, 88)
    mfcr    r1, pacr        //save pacr
    stw     r1, (r0, 92)
    mfcr    r1, prsr        //save prsr
    stw     r1, (r0, 96)
    mfcr    r1, chr        //save chr
    stw     r1, (r0, 100)
    mfcr    r1, cr<15, 1>  //save INT-sp
    stw     r1, (r0, 104)

    /* save se context addr to PRCM_SE_CONTEXT_ADDR */
    lrw     r1, PRCM_SE_CONTEXT_ADDR
    stw     r0, (r1, 0)

    /* Update CPU status to PRCM bank register */
    lrw     r1, PRCM_SESS_CPU_STATUS
    lrw     r2, 1
    stw     r2, (r1, 0)

    sync

    doze

    rts
se_save_context_and_suspend_end:

.section .entry, "ax", %progbits
    .align  3
    .global se_restore_context_and_resume
se_restore_context_and_resume:
    lrw     r1, PRCM_SE_CONTEXT_ADDR
    ldw     r0, (r1, 0)

    ldw     r1, (r0, 60)
    mtcr    r1, psr
    ldw     r1, (r0, 64)
    mtcr    r1, vbr
    ldw     r1, (r0, 68)
    mtcr    r1, epsr
    ldw     r1, (r0, 72)
    mtcr    r1, epc
    ldw     r1, (r0, 76)
    mtcr    r1, gcr
    ldw     r1, (r0, 80)
    mtcr    r1, gsr
    ldw     r1, (r0, 84)
    mtcr    r1, ccr
    ldw     r1, (r0, 88)
    mtcr    r1, capr
    ldw     r1, (r0, 92)
    mtcr    r1, pacr
    ldw     r1, (r0, 96)
    mtcr    r1, prsr
    ldw     r1, (r0, 100)
    mtcr    r1, chr
    ldw     r1, (r0, 104)
    mtcr    r1, cr<15, 1>

    /* clear se context addr to 0 in PRCM_SE_CONTEXT_ADDR */
    lrw     r1, PRCM_SE_CONTEXT_ADDR
    lrw     r2, 0
    stw     r2, (r1, 0)

    /* clear CPU status to PRCM bank register */
    lrw     r1, PRCM_SESS_CPU_STATUS
    lrw     r2, 0
    stw     r2, (r1, 0)

    /* restore r1~r15 */
    ldm     r1-r15, (r0)

    rts
se_restore_context_and_resume_end:

    .data
    .align  3
    .global se_restore_context_and_resume_size
se_restore_context_and_resume_size:
    .word   se_restore_context_and_resume_end - se_restore_context_and_resume
