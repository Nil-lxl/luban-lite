/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <getopt.h>

#include "hal_adcim.h"
#include "hal_gpai.h"

/* The default voltages are set to D21x->3.0V, D31x、D12x->2.5V */

/* Global macro and variables */
#define AIC_GPAI_NAME               "gpai"
#define LONG_PRESS_TIME             50
#define AIC_KEYADC_DEFAULT_SCALE    50
#define AIC_KEYADC_DEFAULT_CHAN     2

/* Define the ADC corresponding to the key */
static int keyadc_arr[] = {500, 3340, 1350};
//0x1:key1, 0x2:key2, 0x3:key3
static int key_flag_arr[] = {0x1, 0x2, 0x3};
static int g_ch = AIC_KEYADC_DEFAULT_CHAN;
static int g_adc_scale = AIC_KEYADC_DEFAULT_SCALE;

static void cmd_kayadc_usage(void)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: test_keyadc [options]\n");
    printf("\ttest_keyadc read <channel_id>\t\tSelect one channel in [0, %d], default is 2\n",
           AIC_GPAI_CH_NUM - 1);
    printf("\ttest_keyadc set <scale>\t\tSet the adc scale,default is 50\n");
    printf("\ttest_keyadc start\t\tStart the keyadc test\n");
    printf("\ttest_keyadc help \t\tGet this help\n");
    printf("\n");
    printf("Example: test_keyadc read 6\n");
}

static int test_keyadc_init(int ch)
{
    static int inited = 0;
    struct aic_gpai_ch *chan;

    if (!inited) {
        hal_adcim_probe();
        hal_gpai_clk_init();
        inited = 1;
    }

    hal_gpai_set_ch_num(AIC_GPAI_CH_NUM);
    chan = hal_gpai_ch_is_valid(ch);
    if (!chan)
        return -1;

    aich_gpai_enable(1);
    hal_gpai_clk_get(chan);
    aich_gpai_ch_init(chan, chan->pclk_rate);
    return 0;
}

uint8_t test_keyadc_scan(int ch, int scale)
{
    u16 adc_val;
    struct aic_gpai_ch *chan;

    test_keyadc_init(ch);

    chan = hal_gpai_ch_is_valid(ch);
    if (chan == NULL) {
        printf("Ch%d is unavailable!\n", ch);
        return -1;
    }

    chan->complete = aicos_sem_create(0);
    aicos_request_irq(GPAI_IRQn, aich_gpai_isr, 0, NULL, NULL);

    while(1) {
        aich_gpai_read(chan, &adc_val, AIC_GPAI_TIMEOUT);

        for (int i = 0; i < sizeof(keyadc_arr) / sizeof(keyadc_arr[0]); i++)
            if ((keyadc_arr[i] - scale) <= adc_val && adc_val <= (keyadc_arr[i] + scale)) {
                printf("[key%d] ch%d: %d\n", key_flag_arr[i], ch,
                           adc_val);
            }
    }
    return 0;
}

static int cmd_test_keyadc(int argc, char *argv[])
{
    if (argc < 2) {
        cmd_kayadc_usage();
        return 0;
    }

    if (!strcmp(argv[1], "read")) {
        g_ch = atoi(argv[2]);
        if (g_ch < 0 || g_ch > AIC_GPAI_CH_NUM - 1) {
            printf("Please set the channel for keyadc\n");
            return 0;
        }
        return 0;
    }

    if (!strcmp(argv[1], "set")) {
        g_adc_scale = atoi(argv[2]);
        if (g_adc_scale <= 0)
            printf("Please set the valid adc scale\n");
        return 0;
    }

    if (!strcmp(argv[1], "start")) {
        test_keyadc_scan(g_ch, g_adc_scale);
        return 0;
    }

    cmd_kayadc_usage();

    return 0;
}

CONSOLE_CMD(test_keyadc, cmd_test_keyadc,  "KEYADC test example");
