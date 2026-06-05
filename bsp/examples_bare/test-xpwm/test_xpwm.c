/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <getopt.h>
#include "aic_common.h"
#include "aic_log.h"
#include "hal_xpwm.h"
#include "xpwm.h"

static bool g_xpwm_init_flag = 0;

static void cmd_xpwm_usage(void)
{
    printf("Usage: \n");
    printf("test_xpwm enable  <channel>                                                 - enable pwm channel\n");
    printf("test_xpwm disable <channel>                                                 - disable pwm channel\n");
    printf("test_xpwm get     <channel>                                                 - get pwm register val\n");
    printf("test_xpwm set          <channel> <period> <pulse> <pulse cnt>               - set pwm channel info\n");
    printf("test_xpwm set_fifo_num <channel> <fifo_num>                                 - set xpwm fifo count\n");
    printf("test_xpwm set_fifo     <channel> <fifo_index> <period> <pulse> <pulse cnt>  - set xpwm fifo info\n");
    printf("test_xpwm get_fifo     <channel>                                            - get xpwm fifo info\n");
#ifdef AIC_USING_DMA
    printf("test_xpwm dma_set_fifo <channel> <period> <pulse> <pulse cnt>               - set xpwm dma fifo info\n");
    printf("test_xpwm dma_test     <channel> <loop_times>                               - xpwm dma test\n");
#endif
    printf("test_xpwm help                                                              - get this help\n");
}

static void cmd_xpwm_en(int argc, char **argv)
{
    int ret = 0;
    char *ret_str;

    if(!strcmp(argv[1], "enable")) {
        if(argc == 3) {
            ret = drv_xpwm_enable(atoi(argv[2]), 1);
            ret_str = (ret == 0) ? "success" : "failure";
            printf("channel %d is enabled %s \n", atoi(argv[2]), ret_str);
        } else {
            printf("test_xpwm enable <channel>                     - enable pwm channel\n");
            printf("    e.g. MSH >pwm enable  1              - PWM_CH1  nomal\n");
            printf("    e.g. MSH >pwm enable -1              - PWM_CH1N complememtary\n");
        }
    } else if(!strcmp(argv[1], "disable")) {
        if(argc == 3) {
            ret = drv_xpwm_enable(atoi(argv[2]), 0);
            ret_str = (ret == 0) ? "success" : "failure";
            printf("channel %d is enabled %s \n", atoi(argv[2]), ret_str);
        } else {
            printf("test_xpwm disable <channel>                    - disable pwm channel\n");
        }
    } else {
        return;
    }
}

static void cmd_xpwm_set(int argc, char **argv)
{
    if(argc == 6) {
        drv_xpwm_set(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
        printf("pwm info set at channel %d\n", atoi(argv[2]));
    } else {
        printf("Set info of channel: [%d] error\n", atoi(argv[2]));
        printf("Usage: test_xpwm set <channel> <period> <pulse> <pulse cnt>\n");
    }
}

static void cmd_xpwm_get(int argc, char **argv)
{
    int ret = 0;
    u32 duty_ns = 0;
    u32 period_ns = 0;

    ret = drv_xpwm_get(atoi(argv[2]), &duty_ns, &period_ns);
    if(ret == 0) {
        printf("Info of channel [%d]:\n", atoi(argv[2]));
        printf("period value     : %d\n", period_ns);
        printf("duty value      : %d\n", duty_ns);
        printf("Duty cycle  : %d%%\n",(int)(((double)(duty_ns)/(period_ns)) * 100));
    } else {
        printf("Get info of channel: [%d] error.\n", atoi(argv[2]));
    }

}

static void cmd_xpwm_set_fifo_num(int argc, char **argv)
{
    if (argc == 4) {
        drv_xpwm_set_fifo_num(atoi(argv[2]), atoi(argv[3]));
        printf("pwm set fifo num at channel %d\n", atoi(argv[2]));
    } else {
        printf("Set fifo num of channel: [%d] error\n", atoi(argv[2]));
        printf("Usage: test_xpwm set_fifo_num <channel> <fifo_num>\n");
    }
}

static void cmd_xpwm_set_fifo(int argc, char **argv)
{
    if (argc == 7) {
        struct aic_xpwm_fifo fifo_info;
        fifo_info.fifo_index = atoi(argv[3]);
        fifo_info.pul_prd[fifo_info.fifo_index] = atoi(argv[4]);
        fifo_info.pul_cmp[fifo_info.fifo_index] = atoi(argv[5]);
        fifo_info.pul_num[fifo_info.fifo_index] = atoi(argv[6]);
        drv_xpwm_set_fifo(atoi(argv[2]), fifo_info);
        printf("pwm set fifo at channel %d\n", atoi(argv[2]));
    } else {
        printf("Set fifo of channel: [%d] error\n", atoi(argv[2]));
        printf("Usage: test_xpwm set_fifo <channel> <fifo_index> <period> <pulse> <pulse cnt>\n");
    }
}

static void cmd_xpwm_get_fifo(int argc, char **argv)
{
    if (argc == 3) {
       drv_xpwm_get_fifo(atoi(argv[2]));
    } else {
      printf("get fifo info of ch: [%d] error\n", atoi(argv[2]));
      printf("Usage: test_xpwm get_fifo <channel>\n");
    }
}

#ifdef AIC_USING_DMA
u32 loop_times = 0;
static void cmd_xpwm_dma_set_fifo(int argc, char **argv)
{
    if (argc == 6) {
        u32 buf[3] __attribute__((aligned(CACHE_LINE_SIZE))) = {0};

        loop_times = 0;
        buf[0] = atoi(argv[3]);
        buf[1] = atoi(argv[4]);
        buf[2] = atoi(argv[5]);

        struct aic_xpwm_buf_info dma_info;
        dma_info.buf = buf;
        dma_info.buf_len = sizeof(buf);
        drv_xpwm_dma_set_fifo(atoi(argv[2]), dma_info, NULL, NULL);
    } else {
        printf("DMA set fifo of ch: [%d] error\n", atoi(argv[2]));
        printf("Usage: test_xpwm dma_set_fifo <channel> <period> <pulse> <pulse cnt>\n");
    }
}

static u32 g_buf[8][30] __attribute__((aligned(CACHE_LINE_SIZE))) = {0};
static u32 g_pul_num[10] = {2, 5, 2, 1, 2, 1, 2, 1, 2, 2};
static u32 g_pul_prd[10] = {1000000, 500000, 800000, 1000000, 300000,
                        500000, 800000, 200000, 900000, 600000};
static u32 g_pul_cmp[10] = {200000, 400000, 100000, 500000, 100000,
                        400000, 600000, 100000, 450000, 300000};

/* callback function */
static void xpwm_cb(void *p)
{
    int i, j;
    struct aic_xpwm_buf_info buf_info = {0};
    u32 ch = *(u32 *)p;

    static int loop = 1;

    if (loop < loop_times) {
        for (j = 0, i = 0; j < 10; j++, i += 3) {
            g_buf[ch][i] = g_pul_prd[j];
            g_buf[ch][i + 1] = g_pul_cmp[j];
            g_buf[ch][i + 2] = g_pul_num[j];
        }
        buf_info.buf = g_buf[ch];
        buf_info.buf_len = sizeof(g_buf[ch]);
        drv_xpwm_dma_set_fifo(ch, buf_info, xpwm_cb, p);
        loop++;
    } else {
        loop = 1;
    }
}

static void cmd_xpwm_dma_test(int argc, char **argv)
{
    if (argc == 4) {
        int i, j;
        struct aic_xpwm_buf_info buf_info = {0};
        u32 ch = atoi(argv[2]);
        u32 *p = &ch;

        /* loop triggering will be reflected in the callback */
        loop_times = atoi(argv[3]);

        if (loop_times <= 0) {
            printf("err input loop_times:%d\n", loop_times);
            return;
        }

        printf("xpwm ch%d will loop %d times\n", ch, loop_times);

        for (j = 0, i = 0; j < 10; j++, i += 3) {
            g_buf[ch][i] = g_pul_prd[j];
            g_buf[ch][i + 1] = g_pul_cmp[j];
            g_buf[ch][i + 2] = g_pul_num[j];
        }

        buf_info.buf = g_buf[ch];
        buf_info.buf_len = sizeof(g_buf[ch]);

        /* trigger the first DMA transport */
        drv_xpwm_dma_set_fifo(ch, buf_info, xpwm_cb, (void *)p);
    } else {
        printf("xpwm dma test: [%d] error\n", atoi(argv[2]));
        printf("Usage: test_xpwm dma_test <channel> <loop_times>\n");
    }

}
#endif

static int cmd_test_xpwm(int argc, char *argv[])
{
    if (argc < 3) {
        cmd_xpwm_usage();
        return 0;
    }
    if (!g_xpwm_init_flag) {
        drv_xpwm_init();
        g_xpwm_init_flag = 1;
    }

    if (!strcmp(argv[1], "enable") || !strcmp(argv[1], "disable"))
        cmd_xpwm_en(argc, &argv[0]);
    else if (!strcmp(argv[1], "set"))
        cmd_xpwm_set(argc, &argv[0]);
    else if (!strcmp(argv[1], "get"))
        cmd_xpwm_get(argc, &argv[0]);
    else if (!strcmp(argv[1], "set_fifo_num"))
        cmd_xpwm_set_fifo_num(argc, &argv[0]);
    else if (!strcmp(argv[1], "set_fifo"))
        cmd_xpwm_set_fifo(argc, &argv[0]);
    else if (!strcmp(argv[1], "get_fifo"))
        cmd_xpwm_get_fifo(argc, &argv[0]);
#ifdef AIC_USING_DMA
    else if (!strcmp(argv[1], "dma_set_fifo"))
        cmd_xpwm_dma_set_fifo(argc, &argv[0]);
    else if (!strcmp(argv[1], "dma_test"))
        cmd_xpwm_dma_test(argc, &argv[0]);
#endif
    else
        cmd_xpwm_usage();

    return 0;
}

CONSOLE_CMD(test_xpwm, cmd_test_xpwm,  "XPWM/PWM test example");

