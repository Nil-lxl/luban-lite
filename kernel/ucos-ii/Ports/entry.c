/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ucos_ii.h>
#include <aic_core.h>
#include <board.h>
#include <aic_tlsf.h>

extern void aic_board_sysclk_init(void);
extern void aic_board_pinmux_init(void);
extern int main(void);

aicos_thread_t ucos_thread_array[OS_LOWEST_PRIO + 1];

void aic_hw_board_init(void)
{
#ifdef TLSF_MEM_HEAP
    aic_tlsf_heap_init();
#endif
    aic_board_sysclk_init();
    aic_board_pinmux_init();

}

static OS_STK main_stack[APP_CFG_STARTUP_TASK_STK_SIZE];

int entry(void)
{
    INT8U ret;
    /* hw&heap init */
    aic_hw_board_init();

    /* kernel init */
    OSInit();
    OSTimeSet(0);

    /* init task */
    ret = OSTaskCreateExt(main,
                          NULL,
                          &main_stack[APP_CFG_STARTUP_TASK_STK_SIZE - 1],
                          APP_CFG_STARTUP_TASK_PRIO,
                          APP_CFG_STARTUP_TASK_PRIO,
                          &main_stack[0],
                          APP_CFG_STARTUP_TASK_STK_SIZE,
                          NULL,
                          OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
    if (ret != OS_ERR_NONE) {
        printf("uC/OS-II create init main task fail.\n");
        return -1;
    }

    /* kernel start */
    OSStart();
    return 0;
}
