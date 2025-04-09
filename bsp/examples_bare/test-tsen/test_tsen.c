/*
 * Copyright (c) 2022-2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include "hal_tsen.h"
#include "hal_adcim.h"
#include "test_tsen.h"

static void cmd_tsen_usage(void)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: test_tsen [options]\n");
    printf("test_tsen read <sensor_id>         : Select one channel in [0, %d], default is 0\n", AIC_TSEN_CH_NUM - 1);
    printf("test_tsen help                     : Get this help\n");
    printf("\n");
    printf("Example: test_tsen read 1\n");
    printf("Sensor ID:\n");
    printf("\t[0] sensor_cpu\n");
    printf("\t[1] sensor_gpai\n");
}

static int test_tsen_init(int argc, char **argv)
{
    u32 ch;
    static int inited = 0;
    struct aic_tsen_ch *chan;

    ch = strtod(argv[1], NULL);
    hal_tsen_set_ch_num(ARRAY_SIZE(aic_tsen_chs));

    chan = hal_tsen_get_valid_ch(ch);
    if (chan == NULL) {
        printf("Please enter the correct sensor id\n");
        return -1;
    }

    if (!inited) {
        hal_adcim_probe();
        inited = 1;
    }
    if (hal_tsen_clk_init())
        return -1;

    hal_tsen_enable(1);
    hal_tsen_ch_enable(0, 1);

    hal_tsen_pclk_get(chan);
    hal_tsen_ch_init(chan, chan->pclk_rate);
    return 0;
}

static void test_tsen_read(int argc, char **argv)
{
    u32 ch;
    int num;
    s32 value;
    struct aic_tsen_ch *chan;

    ch = strtod(argv[1], NULL);
    chan = hal_tsen_get_valid_ch(ch);

#ifdef AIC_SID_DRV
    hal_tsen_curve_fitting(chan);
#endif

    chan->complete = aicos_sem_create(0);
    aicos_request_irq(TSEN_IRQn, hal_tsen_irq_handle, 0, NULL, NULL);
    printf("Starting the %s sensor temperature reading\n", chan->name);
    for (num = 0; num < 10; num++) {
        if (hal_tsen_get_temp(chan, &value) >= 0)
            printf("num:%3d, temp:%3d.%d C (%d)\n", num, value / 10,value % 10,
                   chan->latest_data);
    }
    aicos_sem_delete(chan->complete);
}

static int cmd_test_tsen(int argc, char *argv[])
{
   if (argc < 3) {
        cmd_tsen_usage();
        return 0;
    }

    if (!strcmp(argv[1], "read")) {
        if (!test_tsen_init(argc - 1, &argv[1])) {
            test_tsen_read(argc - 1, &argv[1]);
            return 0;
        }
    }

    cmd_tsen_usage();

    return 0;
}

CONSOLE_CMD(test_tsen, cmd_test_tsen,  "TSensor test example");
