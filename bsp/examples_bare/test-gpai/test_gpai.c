/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao Li <siyao.li@artinchip.com>
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <getopt.h>
#include "hal_adcim.h"
#include "hal_gpai.h"
#include "gpai.h"

#define AIC_GPAI_DEFAULT_VOLTAGE        3
#define AIC_GPAI_ADC_MAX_VAL            0xFFF
#define AIC_GPAI_VOLTAGE_ACCURACY       10000
#define AIC_GPAI_DEFAULT_SAMPLES_NUM    100

static float g_def_voltage = AIC_GPAI_DEFAULT_VOLTAGE;
static int g_sample_num = AIC_GPAI_DEFAULT_SAMPLES_NUM;
static int g_inited = 1;
static aicos_sem_t g_gpai_sem = NULL;

#ifdef AIC_GPAI_DRV_DMA
static struct aic_dma_transfer_info g_dma_info;
static u32 g_adc_buf[CACHE_LINE_SIZE / 4] __attribute__((aligned(CACHE_LINE_SIZE)));
#endif

static void cmd_gpai_usage(void)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: test_gpai [options]\n");
    printf("test_gpai read <channel_id>         : Select one channel in [0, %d], default is 0\n",
           AIC_GPAI_CH_NUM - 1);
    printf("test_gpai modify <default_voltage>  : Modify default voltage\n");
    printf("test_gpai set <samples_number>      : Set the number of samples,default is 100\n");
    printf("test_gpai help                      : Get this help\n");
    printf("\n");
    printf("Example: test_gpai read 4\n");
}

static void cmd_gpai_set(int argc, char **argv)
{
    g_def_voltage = strtod(argv[1], NULL);
    if (g_def_voltage < 0) {
        printf("Please input valid default voltage\n");
        return;
    }
    printf("Successfully set the default voltage\n");
}

static int gpai_get_adc_single(int ch, u32 cal_param)
{
    u16 value = 0;
    int voltage = 0, cnt = 0;
    int sample_cnt = g_sample_num;
    struct aic_gpai_ch *chan = {0};
    int scale = AIC_GPAI_VOLTAGE_ACCURACY;

    chan = hal_gpai_ch_is_valid(ch);
    drv_gpai_enabled(ch, true);
    while(sample_cnt) {
        hal_gpai_get_data(chan, &value, AIC_GPAI_TIMEOUT);
        cnt++;
        if (value) {
            voltage = hal_adcim_adc2voltage(&value, cal_param,
                                            scale, g_def_voltage);
            printf("[%d] ch %d: %d\n", cnt, ch, value);
            printf("voltage : %d.%04d v\n", voltage / scale, voltage % scale);
        }
        sample_cnt--;
    }

    return 0;
}

static void gpai_irq_callback(void *arg)
{
    aicos_sem_give(g_gpai_sem);
}

static int gpai_get_adc_period(int ch, u32 cal_param)
{
    int cnt = 0;
    int voltage = 0;
    int scale = AIC_GPAI_VOLTAGE_ACCURACY;
    int sample_cnt = g_sample_num;
    struct aic_gpai_irq_info irq_info = {0};
    struct aic_gpai_ch_info ch_info = {0};

    irq_info.chan_id = ch;
    irq_info.callback = gpai_irq_callback;
    irq_info.callback_param = NULL;

    ch_info.chan_id = ch;

    g_gpai_sem = aicos_sem_create(0);

    drv_gpai_irq_callback(&irq_info);
    drv_gpai_enabled(ch, true);

    while (cnt < sample_cnt) {
        if (aicos_sem_take(g_gpai_sem, AICOS_WAIT_FOREVER) < 0) {
            break;
        }

        drv_gpai_get_ch_info(&ch_info);

        for (int i = 0; i < ch_info.fifo_valid_cnt; i++) {
            if (ch_info.adc_values[i]) {
                voltage = hal_adcim_adc2voltage(&ch_info.adc_values[i], cal_param,
                                                scale, g_def_voltage);
                printf("[%d] GPAI ch%d: %d\n", cnt, ch, ch_info.adc_values[i]);
                printf("GPAI voltage:%d.%04d v\n", voltage / scale, voltage % scale);
            }
            cnt++;
        }
    }

    drv_gpai_enabled(ch, false);

    return voltage;
}

int adc_check_gpai_by_cpu_mode(int ch, u32 cal_param)
{
    int mode_id = 0;

    mode_id = drv_gpai_get_mode(ch);
    switch (mode_id) {
    case AIC_GPAI_MODE_SINGLE:
        gpai_get_adc_single(ch, cal_param);
        break;
    case AIC_GPAI_MODE_PERIOD:
        gpai_get_adc_period(ch, cal_param);
        break;
    default:
        return -1;
    }
    return -1;
}

#ifdef AIC_GPAI_DRV_DMA
static int gpai_dma_get_data(int ch)
{
    int i;
    int *dma_data = (int *)g_dma_info.buf;

    drv_gpai_get_dma_data(ch);
    aicos_dcache_invalid_range(g_adc_buf, g_dma_info.buf_size);

        for(i = 0; i < g_dma_info.buf_size / sizeof(g_adc_buf[0]); i++) {
            printf("[%d] %d\n",i, dma_data[i]);
        }

    return 0;
}

static void gpai_dma_callback(void *arg)
{
    int ch = (int)arg;

    printf("dma callback happened\n");
    gpai_dma_get_data(ch);
}

static int gpai_get_adc_by_dma(int ch)
{
    int ret;

    drv_gpai_enabled(ch, true);

    g_dma_info.chan_id = ch;
    g_dma_info.buf = g_adc_buf;
    g_dma_info.buf_size = sizeof(g_adc_buf);
    g_dma_info.callback = gpai_dma_callback;
    g_dma_info.callback_param = (void *)ch;

    ret = drv_gpai_config_dma(&g_dma_info);
    if (ret) {
        printf("Failed to config DMA\n");
        return -1;
    }

    gpai_dma_get_data(ch);
    //drv_gpai_stop_dma(ch);// Use this interface at the point where you want to stop DMA

    return 0;
}
#endif

static int gpai_get_adc_by_poll(int ch, u32 cal_param)
{
    int cnt = 0;
    int voltage = 0;
    int scale = AIC_GPAI_VOLTAGE_ACCURACY;
    int sample_cnt = g_sample_num;
    u16 adc_val;
    struct aic_gpai_ch_info ch_info = {0};

    ch_info.chan_id = ch;
    drv_gpai_enabled(ch, true);

    while (cnt < sample_cnt) {
        drv_gpai_get_ch_info(&ch_info);

        adc_val = ch_info.adc_values[0];
        if (adc_val) {
            voltage = hal_adcim_adc2voltage(&adc_val, cal_param, scale, g_def_voltage);
            printf("[%d] GPAI ch%d: %d\n", cnt, ch, adc_val);
            printf("GPAI voltage:%d.%04d v\n", voltage / scale, voltage % scale);
            cnt++;
        }
    }

    drv_gpai_enabled(ch, false);

    return voltage;
}

static int cmd_gpai_read(int argc, char **argv)
{
    u32 ch = 0;
    u32 cal_param;
    struct aic_gpai_ch *chan = {0};
    int data_mode_id = 0;

    ch = strtod(argv[1], NULL);
    if ((ch < 0) || (ch >= AIC_GPAI_CH_NUM)) {
        printf("Invalid channel No.%d\n", ch);
        return -1;
    }

    if (g_inited) {
        drv_gpai_init();
        g_inited = 0;
    }

    chan = hal_gpai_ch_is_valid(ch);
    if (chan == NULL)
        return -1;

    cal_param = hal_adcim_auto_calibration();
    drv_gpai_chan_init(ch);

    data_mode_id = drv_gpai_get_data_mode(ch);
    switch (data_mode_id) {
    case AIC_GPAI_OBTAIN_DATA_BY_CPU:
        adc_check_gpai_by_cpu_mode(ch, cal_param);
        break;
#ifdef AIC_GPAI_DRV_DMA
    case AIC_GPAI_OBTAIN_DATA_BY_DMA:
        gpai_get_adc_by_dma(ch);
        break;
#endif
    case AIC_GPAI_OBTAIN_DATA_BY_POLL:
        gpai_get_adc_by_poll(ch, cal_param);
        break;
    default:
        return -1;
    }

    return 0;
}

static int cmd_test_gpai(int argc, char *argv[])
{
    if (argc < 3) {
        cmd_gpai_usage();
        return 0;
    }

    if (!strcmp(argv[1], "read")) {
        cmd_gpai_read(argc - 1, &argv[1]);
        return 0;
    }

    if (!strcmp(argv[1], "set")) {
        g_sample_num = atoi(argv[2]);
        if (g_sample_num <= 0)
            printf("Please set the number of samples\n");
        return 0;
    }

    if (!strcmp(argv[1], "modify")) {
        cmd_gpai_set(argc - 1, &argv[1]);
        return 0;
    }

    cmd_gpai_usage();

    return 0;
}

CONSOLE_CMD(test_gpai, cmd_test_gpai,  "GPAI test example");
