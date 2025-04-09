/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
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
#include <time.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "aic_log.h"
#include "hal_rtp.h"
#include "touch.h"
#include "hal_adcim.h"

/* The voltage range of the RTP ADC is based on VCCIO_3V3.
 * The voltage range of the GPAI ADC is based on LDO.
 */
#define AIC_RTP_ADC_DEFAULT_VOLTAGE    3.3
#define AIC_RTP_ADC_VOLTAGE_ACCURACY   10000
#define AIC_RTP_ADC_CHAN_MAX        4

static const char sopts[] = "r:t:n:h";
static const struct option lopts[] = {
    {"read",         required_argument, NULL, 'r'},
    {"def_voltage",  required_argument, NULL, 't'},
    {"number",       required_argument, NULL, 'n'},
    {"help",         no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

static rt_device_t g_rtp_dev = RT_NULL;
static int g_sample_num = 10;

static void cmd_rtp_usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -r, --read <chan>\t\n");
    printf("\t -t, --voltage\t\tModify default voltage\n");
    printf("\t -n, --number\t\tSet the number of samples\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: %s -r 1 -n 10 -t 3.3\n", program);
    printf("CHAN-ID:\n");
    printf("\t[0] Y-\n");
    printf("\t[1] X-\n");
    printf("\t[2] Y+\n");
    printf("\t[3] X+\n");
}

static void rtp_get_data(int ch, float def_voltage)
{
    struct aic_rtp_adc_info adc_info = {0};
    int scale = AIC_RTP_ADC_VOLTAGE_ACCURACY;
    int voltage = 0, cnt = 0, cal_param = 0;

    g_rtp_dev = rt_device_find(AIC_RTP_NAME);
    if (g_rtp_dev == RT_NULL) {
        rt_kprintf("Failed to find %s device\n", AIC_RTP_NAME);
        return;
    }

    adc_info.ch = ch;
    cal_param = hal_adcim_auto_calibration();
    rt_device_control(g_rtp_dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);

    while (cnt < g_sample_num) {
        rt_device_control(g_rtp_dev, RT_TOUCH_CTRL_GET_ADC, (void *)&adc_info);
        aicos_msleep(10);
        if (adc_info.data) {
            voltage = hal_adcim_adc2voltage(&adc_info.data, cal_param, AIC_RTP_ADC_VOLTAGE_ACCURACY, def_voltage);
            rt_kprintf("[%d] ch%d: %d\n", cnt, ch, adc_info.data);
            rt_kprintf("voltage:%d.%04d v\n", voltage / scale, voltage % scale);
        }
        cnt++;
    }

    return;
}

static void cmd_test_rtp_adc(int argc, char **argv)
{
    int c, ch = 0;
    float def_voltage = AIC_RTP_ADC_DEFAULT_VOLTAGE;

    if (argc < 2) {
        cmd_rtp_usage(argv[0]);
        return;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'r':
            ch = atoi(optarg);
            break;
        case 't':
            def_voltage = atof(optarg);
            break;
        case 'n':
            g_sample_num = atoi(optarg);
            break;
        case 'h':
        default:
            cmd_rtp_usage(argv[0]);
            return;
        }
    }

    if ((ch < 0) || (ch >= AIC_RTP_ADC_CHAN_MAX)) {
        pr_err("Invalid channel No.%s\n", optarg);
    }
    rtp_get_data(ch, def_voltage);

    return;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_rtp_adc, test_rtp_adc, rtp device sample);
