/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <console.h>
#include "aic_common.h"
#include "aic_core.h"
#include "boot_param.h"
#include "hal_inputcap.h"
#include "inputcap.h"

static bool g_inputcap_init_flag = 0;

#ifdef AIC_DMA_DRV
static uint32_t g_data_buf0[AIC_INPUTCAP_CH_NUM][INPUTCAP_WATER_MARK] __attribute__((aligned(CACHE_LINE_SIZE))) = {0};
static uint32_t g_data_buf1[AIC_INPUTCAP_CH_NUM][INPUTCAP_WATER_MARK] __attribute__((aligned(CACHE_LINE_SIZE))) = {0};
#endif

/* callback function */
void inputcap_event0_cb(void *arg)
{
    int i;
    u8 ch = *(u8 *)arg;
#ifdef AIC_DMA_DRV
    /* DMA mode returns the original data of the register. */
    u32 temp_cnt;
    for (i = 0; i < INPUTCAP_WATER_MARK - 1; i++) {
        if (g_data_buf0[ch][i + 1] > g_data_buf0[ch][i])
            temp_cnt = g_data_buf0[ch][i + 1] - g_data_buf0[ch][i];
        else
            temp_cnt = g_data_buf0[ch][i + 1] + ((0xFFFFFFFF - g_data_buf0[ch][i]) + 1);

        printf("incap%d event0: pulsewidth:%d us\n", ch, temp_cnt / (INPUTCAP_CLK_RATE / 1000000));
    }
#else
    /* Interrupt mode returns calculated us data. */
    //disable the interrupt to prevent data from being overwritten
    aic_inputcap_int_dis(ch, 0);
    u32 *data = aic_get_inputcap_data(ch, 0);
    for (i = 0; i < INPUTCAP_WATER_MARK; i++)
        printf("incap%d event0: pulsewidth:%d us\n", ch, data[i]);

#endif
}

void inputcap_event1_cb(void *arg)
{
    int i;
    u8 ch = *(u8 *)arg;

#ifdef AIC_DMA_DRV
    /* DMA mode returns the original data of the register. */
    u32 temp_cnt;
    for (i = 0; i < INPUTCAP_WATER_MARK - 1; i++) {
        if (g_data_buf1[ch][i + 1] > g_data_buf1[ch][i])
            temp_cnt = g_data_buf1[ch][i + 1] - g_data_buf1[ch][i];
        else
            temp_cnt = g_data_buf1[ch][i + 1] + ((0xFFFFFFFF - g_data_buf1[ch][i]) + 1);

        printf("incap%d event1: pulsewidth:%d us\n", ch, temp_cnt / (INPUTCAP_CLK_RATE / 1000000));
    }
#else
    /* Interrupt mode returns calculated us data. */
    //disable the interrupt to prevent data from being overwritten
    aic_inputcap_int_dis(ch, 1);
    u32 *data = aic_get_inputcap_data(ch, 1);
    for (i = 0; i < INPUTCAP_WATER_MARK; i++)
        printf("incap%d event1: pulsewidth:%d us\n", ch, data[i]);

#endif
}

int cmd_test_inputcap(int argc, char **argv)
{
    int ret = EOK;
    u32 ch = 0;

    if (argc != 2) {
        printf("Usage: test_inputcap <channel>\n");
        return -EINVAL;
    }

    ch = atoi(argv[1]);

    if (!g_inputcap_init_flag) {
        drv_inputcap_init();
        g_inputcap_init_flag = 1;
    }

    aic_inputcap_close(ch);

#ifdef AIC_DMA_DRV
    struct inputcap_fifo_buf buf_para;
    buf_para.event0_buf = g_data_buf0[ch];
    buf_para.event0_buflen = sizeof(g_data_buf0[ch]);
    buf_para.event1_buf = g_data_buf1[ch];
    buf_para.event1_buflen = sizeof(g_data_buf1[ch]);
    ret = aic_inputcap_setbuf(ch, &buf_para);
    if (ret != EOK) {
        printf("Failed to set cap%d device dma buff!\n", ch);
        return ret;
    }
#endif

    struct inputcap_cb cb;
    cb.func_event0 = inputcap_event0_cb;
    cb.func_event1 = inputcap_event1_cb;

    if (aic_inputcap_open(ch, cb) < 0) {
        printf("open inputcap%d failed!\n", ch);
        return -EINVAL;
    }

    return ret;
}

CONSOLE_CMD(test_inputcap, cmd_test_inputcap,  "inputcap test example");


