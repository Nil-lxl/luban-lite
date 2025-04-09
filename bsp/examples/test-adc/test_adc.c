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
#include <time.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "aic_log.h"
#include "hal_adcim.h"
#include "hal_gpai.h"

#include "aic_core.h"

/* Global macro and variables */
#define AIC_ADCIM_NAME              "adcim"
#define AIC_GPAI_NAME               "gpai"

#ifdef AIC_ADCIM_DM_DRV
#ifdef AIC_GPAI_DRV
#define ADC_CHAN_GPAI             AIC_GPAI_CH_NUM
#else
#define ADC_CHAN_GPAI             0
#endif
#endif

#define GPAI_AVG_SAMPLES_NUM        8
#define ADC_CHAN_NUM                16
#define ADC_DM_SRAM_SIZE            512
#define ADC_TEST_DATA_COUNT         64
#define GPAI_FIFO_MAX_DEPTH         32

static rt_adc_device_t gpai_dev;
static const char sopts[] = "c:hv";
static const struct option lopts[] = {
    {"channel", required_argument, NULL, 'c'},
    {"usage",   no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {0, 0, 0, 0}
};

static rt_sem_t g_gpai_sem = RT_NULL;
static int g_verbose = 0;
static int g_obtain_data_mode = 0;
static struct aic_dma_transfer_info dma_info;
static int g_cur_data[ADC_DM_SRAM_SIZE] = {0};
static int g_sram_data[ADC_DM_SRAM_SIZE] = {0};
static int g_expect_data[ADC_TEST_DATA_COUNT] = {0};
static rt_uint32_t g_adc_buf[ADC_TEST_DATA_COUNT] __attribute__((aligned(64)));


typedef void (*dma_callback)(void *dma_param);

static void cmd_adcim_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -c, --channel\t\tSelect one channel in [0, 15]\n");
    printf("\t -h, --usage \n");
    printf("\n");
    printf("Example: %s -c 0\n", program);
}

int abs(int val)
{
    if (val < 0)
        return -val;
    else
        return val;
}

static void gen_adc_data(u32 *size)
{
    int i;
    int *pdata = g_sram_data;

    if (g_verbose)
        pr_info("Generate %d data ...\n", ADC_DM_SRAM_SIZE);
    for (i = 0; i < ADC_DM_SRAM_SIZE; i++)
        *pdata++ = 1800 + abs(i % 256 - 128) - 64;

    *size = ADC_DM_SRAM_SIZE;
    if (g_verbose)
        pr_info("The ADC data, size %d:\n", *size);
}

int average(int *data, u32 size, int trim)
{
    int sum = 0, i, min, max = 0;

    if (!data || size < 3)
        return 0;

    min = *data;

    for (i = 0; i < size; i++) {
        if (data[i] < min)
            min = data[i];
        if (data[i] > max)
            max = data[i];
        sum += data[i];
    }

    if (trim)
        return (sum - min - max) / (size - 2);
    else
        return sum/size;
}

static void gpai_check_adc_by_cpu(void)
{
    int i;
    int failed_count = 0;

    for(i = 0;i < ADC_TEST_DATA_COUNT;i++) {
        if (g_cur_data[i] != g_expect_data[i]) {
            rt_kprintf("[%d] Failed%d/%d\n", i, g_cur_data[i], g_expect_data[i]);
            failed_count++;
        } else {
            rt_kprintf("[%d] OK! %d/%d\n",i, g_cur_data[i], g_expect_data[i]);
        }
    }

    if (!failed_count)
        rt_kprintf("All data is OK. Test Successfully!\n");
    else
        rt_kprintf("%d data is incorrect. Test Failed!\n", failed_count);
}

static void gpai_irq_callback(void *arg)
{
    rt_sem_release(g_gpai_sem);
}

static int gpai_get_adc_by_cpu_single(int chan)
{
    int count = 0;
    struct aic_gpai_ch_info ch_info;

    gpai_dev = (rt_adc_device_t)rt_device_find(AIC_GPAI_NAME);
    if (!gpai_dev) {
        rt_kprintf("Failed to open %s device\n", AIC_GPAI_NAME);
        return -RT_ERROR;
    }
    ch_info.chan_id = chan;
    ch_info.fifo_valid_cnt = 0;

    rt_adc_enable(gpai_dev, chan);

    while (count < ADC_TEST_DATA_COUNT) {
        rt_adc_control(gpai_dev, RT_ADC_CMD_GET_CH_INFO, (void *)&ch_info);
        g_cur_data[count] = ch_info.adc_values[0];
        count++;
    }

    for (int i = 0; i < ADC_DM_SRAM_SIZE / GPAI_AVG_SAMPLES_NUM; i++) {
        g_expect_data[i] = average(&g_sram_data[i * GPAI_AVG_SAMPLES_NUM],
                                   GPAI_AVG_SAMPLES_NUM, 0);
    }
    gpai_check_adc_by_cpu();

    rt_adc_disable(gpai_dev, chan);

    return 0;
}

static int gpai_get_adc_by_cpu_period(int chan)
{
    int count = 0;
    rt_base_t level;
    struct aic_gpai_irq_info irq_info;
    struct aic_gpai_ch_info ch_info;

    gpai_dev = (rt_adc_device_t)rt_device_find(AIC_GPAI_NAME);
    if (!gpai_dev) {
        rt_kprintf("Failed to open %s device\n", AIC_GPAI_NAME);
        return -RT_ERROR;
    }
    ch_info.chan_id = chan;
    ch_info.fifo_valid_cnt = 0;

    irq_info.chan_id = chan;
    irq_info.callback = gpai_irq_callback;
    irq_info.callback_param = NULL;

    if (!g_gpai_sem)
        g_gpai_sem = rt_sem_create("gpai_period_sem", 0, RT_IPC_FLAG_FIFO);

    rt_adc_control(gpai_dev, RT_ADC_CMD_IRQ_CALLBACK, &irq_info);
    rt_adc_enable(gpai_dev, chan);

    while (count < ADC_TEST_DATA_COUNT) {
        if (rt_sem_take(g_gpai_sem, RT_WAITING_FOREVER)!=RT_EOK)
            break;

        level = rt_hw_interrupt_disable();
        rt_adc_control(gpai_dev, RT_ADC_CMD_GET_CH_INFO, (void *)&ch_info);
        rt_hw_interrupt_enable(level);

        for (int i = 0; i < ch_info.fifo_valid_cnt; i++) {
            g_cur_data[count] = ch_info.adc_values[i];
            count++;
        }
    }

    for (int i = 0; i < ADC_DM_SRAM_SIZE / GPAI_AVG_SAMPLES_NUM; i++) {
        g_expect_data[i] = average(&g_sram_data[i * GPAI_AVG_SAMPLES_NUM],
                                   GPAI_AVG_SAMPLES_NUM, 0);
    }
    gpai_check_adc_by_cpu();

    rt_adc_disable(gpai_dev, chan);

    return 0;
}

static void gpai_dma_callback(void *arg)
{
    rt_kprintf("dma callback happened\n");
}

static void gpai_check_adc_by_dma(void)
{
    int i;
    int failed_count = 0;
    int *dma_data = (int *)dma_info.buf;

    for(i = 0; i < ADC_TEST_DATA_COUNT; i++) {
        if (dma_data[i] != g_expect_data[i]) {
            rt_kprintf("[%d] Failed%d/%d\n", i, dma_data[i], g_expect_data[i]);
            failed_count++;
        } else {
            rt_kprintf("[%d] OK! %d/%d\n",i, dma_data[i], g_expect_data[i]);
        }
    }

    if (!failed_count)
        rt_kprintf("All data is OK. Test Successfully!\n");
    else
        rt_kprintf("%d data is incorrect. Test Failed!\n", failed_count);

    aicos_msleep(10);
}

static int gpai_get_adc_by_dma(int chan)
{
    int ret;
    int i;

    gpai_dev = (rt_adc_device_t)rt_device_find(AIC_GPAI_NAME);
    if (!gpai_dev) {
        rt_kprintf("Failed to open %s device\n", AIC_GPAI_NAME);
        return -RT_ERROR;
    }

    ret = rt_adc_enable(gpai_dev, chan);
    if (ret) {
        rt_kprintf("Failed to enable %s device\n", AIC_GPAI_NAME);
        return -RT_ERROR;
    }

    dma_info.chan_id = chan;
    dma_info.buf = g_adc_buf;
    dma_info.buf_size = sizeof(g_adc_buf);
    dma_info.callback = gpai_dma_callback;

    ret = rt_adc_control(gpai_dev, RT_ADC_CMD_CONFIG_DMA, &dma_info);
    if (ret) {
        rt_kprintf("Failed to config DMA\n");
        return -RT_ERROR;
    }

    ret = rt_adc_control(gpai_dev, RT_ADC_CMD_GET_DMA_DATA, (void *)chan);
    if (ret) {
        rt_kprintf("Failed to get DMA data\n");
        return -RT_ERROR;
    }

    for (i = 0; i < ADC_DM_SRAM_SIZE / GPAI_AVG_SAMPLES_NUM; i++) {
        g_expect_data[i] = average(&g_sram_data[i * GPAI_AVG_SAMPLES_NUM],
                                   GPAI_AVG_SAMPLES_NUM, 0);
    }
    g_obtain_data_mode = 0;
    aicos_msleep(100);
    gpai_check_adc_by_dma();
    rt_adc_disable(gpai_dev, chan);

    return 0;
}

static int adc_check_gpai_by_cpu_mode(long ch)
{
    int ret = 0;
    struct aic_gpai_ch_info ch_info = {0};

    ch_info.chan_id = ch;
    ret = rt_adc_control(gpai_dev, RT_ADC_CMD_GET_MODE, (void *)&ch_info);
    if (ret) {
        rt_kprintf("Failed to get GPAI mode\n");
        return -RT_ERROR;
    }

    switch (ch_info.mode) {
    case AIC_GPAI_MODE_SINGLE:
        rt_kprintf("Starting gpai single mode\n");
        gpai_get_adc_by_cpu_single(ch);
        break;
    case AIC_GPAI_MODE_PERIOD:
        rt_kprintf("Starting gpai period mode\n");
        gpai_get_adc_by_cpu_period(ch);
        break;
    default:
        rt_kprintf("Unknown mode: %#x\n");
        break;
    }
    return RT_EOK;
}

static int adc_dm_test(u32 chan)
{
    u32 size = 0;

    if (chan <= ADC_CHAN_GPAI) {
        gen_adc_data(&size);
        hal_dm_chan_store(chan);
        hal_adcdm_sram_write(g_sram_data, 0, ADC_DM_SRAM_SIZE);

        gpai_dev = (rt_adc_device_t)rt_device_find(AIC_GPAI_NAME);
        if (!gpai_dev) {
            rt_kprintf("Failed to open %s device\n", AIC_GPAI_NAME);
            return -RT_ERROR;
        }

        g_obtain_data_mode = rt_adc_control(gpai_dev,
                                            RT_ADC_CMD_OBTAIN_DATA_MODE,
                                            (void *)chan);
        if (g_obtain_data_mode < 0) {
            rt_kprintf("Failed to get obtain data mode\n");
            return -RT_ERROR;
        }

        switch (g_obtain_data_mode) {
        case AIC_GPAI_OBTAIN_DATA_BY_CPU:
            rt_kprintf("Starting CPU interrupt mode\n");
            adc_check_gpai_by_cpu_mode(chan);
            break;
        case AIC_GPAI_OBTAIN_DATA_BY_DMA:
            rt_kprintf("Starting DMA mode\n");
            gpai_get_adc_by_dma(chan);
            break;
        default:
            rt_kprintf("The current mode%d is not supported\n",
                       g_obtain_data_mode);
            break;
        }
    }
    return 0;
}

static void cmd_test_adc(int argc, char **argv)
{
    int c;
    u32 ch = 0;

    if (argc < 2) {
        cmd_adcim_usage(argv[0]);
        return;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'c':
            ch = atoi(optarg);
            if ((ch < 0) || (ch >= AIC_GPAI_CH_NUM))
                pr_err("Invalid channel No.%s\n", optarg);
            adc_dm_test(ch);
            continue;
        case 'v':
            g_verbose = 1;
            continue;
        case 'h':
        default:
            cmd_adcim_usage(argv[0]);
            return;
        }
    }

    return;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_adc, test_adc, adcim dm device sample);
