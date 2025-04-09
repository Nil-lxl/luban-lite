/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao.Li <siyao.li@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>

#include "rtdevice.h"
#include "aic_core.h"
#include "aic_log.h"
#include "board.h"

/* Global macro and variables */
#define AIC_GPAI_NAME               "gpai"
#define LONG_PRESS_TIME             50
#define AIC_KEYADC_DEFAULT_SCALE    50
#define AIC_KEYADC_DEFAULT_CHAN     2

static const char sopts[] = "c:s:gh";
static const struct option lopts[] = {
    {"channel",   required_argument, NULL, 'c'},
    {"set_scale", required_argument, NULL, 's'},
    {"get_adc", no_argument, NULL, 'g'},
    {"help",          no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

static rt_adc_device_t gpai_dev;
/* Define the ADC corresponding to the key */
static int keyadc_arr[] = {497, 3340, 1351};
static int key_flag_arr[] = {0x1, 0x2, 0x3};

static void cmd_keyadc_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -c, --channel\t\tSelect one channel in [0, %d], default is 2\n",
           AIC_GPAI_CH_NUM - 1);
    printf("\t -s, --scale\t\tSet the adc scale,default is 50\n");
    printf("\t -g, --get\t\tGet the adc value\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: %s -c 2 -g\n", program);
}

int aic_key_init(int chan)
{
    gpai_dev = (rt_adc_device_t)rt_device_find(AIC_GPAI_NAME);
    if (!gpai_dev) {
        rt_kprintf("Failed to open %s device\n", AIC_GPAI_NAME);
    }
    rt_adc_enable(gpai_dev, chan);

    return 0;
}

uint8_t aic_key_scan(int chan, int scale)
{
    int adc_val;

    while(1) {
        adc_val = rt_adc_read(gpai_dev, chan);

        for (int i = 0; i < sizeof(keyadc_arr) / sizeof(keyadc_arr[0]); i++)
            if ((keyadc_arr[i] - scale) <= adc_val && adc_val <= (keyadc_arr[i] + scale)) {
                rt_kprintf("[key%d] ch%d: %d\n", key_flag_arr[i], chan,
                           adc_val);
                return key_flag_arr[i];
            rt_thread_mdelay(100);
            }
    }

  return 0;
}

void keyadc_get_adc(int chan, int scale)
{
    uint8_t key_value;
    int flag = 0, last_flag = 0;

    while(1) {
        key_value = aic_key_scan(chan, scale);
        if (key_value == key_flag_arr[1]) {
            flag++;
            if (flag > LONG_PRESS_TIME && last_flag > 0)
                printf("LONG Press\n");
        } else {
            flag = 0;
        }
        last_flag = flag;
    }

    return;
}

static void cmd_test_keyadc(int argc, char **argv)
{
    int c = 0;
    int ch = AIC_KEYADC_DEFAULT_CHAN;
    int scale = AIC_KEYADC_DEFAULT_SCALE;
    int keyadc_getadc_status = false;

    if (argc < 2) {
        cmd_keyadc_usage(argv[0]);
        return;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'c':
            ch = atoi(optarg);
            if ((ch < 0) || (ch >= AIC_GPAI_CH_NUM)) {
                pr_err("Invalid channel No.%s\n", optarg);
            }
            break;
        case 's':
            scale = atoi(optarg);
            break;
        case 'g':
            keyadc_getadc_status = true;
            break;
        case 'h':
        default:
            cmd_keyadc_usage(argv[0]);
            return;
        }
    }

    if (keyadc_getadc_status) {
        if (ch < 0 || ch > AIC_GPAI_CH_NUM - 1) {
            printf("Please set the channel for keyadc\n");
            return;
        }
        aic_key_init(ch);
        keyadc_get_adc(ch, scale);
        keyadc_getadc_status = false;
    }

    return;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_keyadc, test_keyadc, keyadc device sample);
