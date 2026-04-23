/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>

#include "rtdevice.h"
#include "aic_core.h"
#include "aic_log.h"
#include "rtdevice.h"
#include "drv_psadc.h"
#include "hal_psadc.h"
#ifdef AIC_SYSCFG_DRV
#include "hal_syscfg.h"
#endif

/* Global macro and variables */
#define AIC_PSADC_NAME               "psadc"
#define AIC_PSADC_ADC_MAX_VAL        0xFFF
#define AIC_PSADC_DEFAULT_VOLTAGE    3
#define AIC_PSADC_QC_MODE            0
#define AIC_PSADC_VOLTAGE_ACCURACY   10000
#define AIC_PSADC_BIT_MASK           0xFFF

struct psadc_data {
    struct drv_psadc_dma_info dma_info;
    rt_sem_t cplt_sem;
};

static rt_adc_device_t psadc_dev;
static const char sopts[] = "t:n:w:sh";
static const struct option lopts[] = {
    {"voltage", required_argument, NULL, 't'},
    {"number",  required_argument, NULL, 'n'},
    {"window",  required_argument, NULL, 'w'},
    {"status",        no_argument, NULL, 's'},
    {"help",          no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

/* Functions */

static void cmd_psadc_usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -t, --voltage\t\tSet default voltage\n");
    printf("\t -s, --status\t\tShow more hardware information\n");
    printf("\t -n, --number\t\tSet the number of samples, default is 10\n");
    printf("\t -w, --window\t\tOnly show sample rate within the window\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: %s -t 3 -n 10\n", program);
}

static int test_psadc_adc2voltage(float ref_voltage, int adc_value)
{
    adc_value &= AIC_PSADC_BIT_MASK;
    return (adc_value * (ref_voltage / 100)) / AIC_PSADC_ADC_MAX_VAL;
}

#ifdef AIC_PSADC_DRV_DMA
static void psadc_dma_cb(void *arg)
{
    struct psadc_data *data = (struct psadc_data *)arg;

    printf("PSADC DMA Callback.\n");
    rt_sem_release(data->cplt_sem);
}

static int psadc_get_adc_by_dma(float ref_voltage, int sample_num, int chan_cnt)
{
    int i = 0, j = 0, k = 0, voltage = 0;
    struct drv_psadc_dma_info info = {0};
    struct psadc_data psadc_data = {0};
    rt_err_t ret = RT_EOK;
    u32 *buf = NULL;

    psadc_data.cplt_sem = rt_sem_create("gpai_period_sem", 0, RT_IPC_FLAG_FIFO);

#ifdef AIC_PSADC_TRIG_BY_SOFT
    info.smp_cnt = 1;
#else
    info.smp_cnt = sample_num;
    sample_num = 1;
#endif
    info.callback = psadc_dma_cb;
    info.callback_param = &psadc_data;
    ret = rt_adc_control(psadc_dev, RT_ADC_CMD_CONFIG_DMA,
                         (void *)&info);
    if (ret != RT_EOK) {
        printf("Failed to config DMA. ret %ld\n", ret);
        goto out;
    }

    do {
        ret = rt_adc_control(psadc_dev, RT_ADC_CMD_ACTIVE_DMA,
                            NULL);
        if (ret != RT_EOK) {
            printf("Failed to active DMA. ret %ld\n", ret);
            goto out;
        }

        ret = rt_sem_take(psadc_data.cplt_sem, 100);
        if (ret != RT_EOK) {
            printf("wait for semaphore timeout\n");
            goto out;
        }

        buf = (u32 *)info.buf;
        for (j = 0; j < info.smp_cnt; j++) {
            for (k = 0; k < chan_cnt; k++) {
                voltage = test_psadc_adc2voltage(ref_voltage, buf[j * chan_cnt + k]);
                printf("[%d: %d.%d] %7d %2d.%02d V\n", i, j, k,
                       buf[j * chan_cnt + k], voltage / 100, voltage % 100);
            }
        }
        i++;
        sample_num--;
    } while (sample_num > 0);
out:
    rt_sem_delete(psadc_data.cplt_sem);
    return ret;
}
#else
static void show_sample_rate(u32 cnt, u32 window, u64 start_us)
{
    u64 elapse = aic_get_time_us() - start_us;

    printf("Cnt %d, Sample rate: %ld Hz\n", cnt,
           (long)(((u64)window * 1000000ULL) / elapse));
}
#endif

int psadc_get_adc(float def_voltage, int sample_num, u32 window)
{
#ifndef AIC_PSADC_DRV_DMA
    u32 adc_values[AIC_PSADC_CH_NUM] = {0};
    u64 start_us = 0, end_us = 0;
    int voltage = 0;
    int cnt = 0;
    int i = 0;
#endif
    int ref_voltage = 0;
    int chan_cnt = 0;
    int ret = 0;

    psadc_dev = (rt_adc_device_t)rt_device_find(AIC_PSADC_NAME);
    if (!psadc_dev) {
        rt_kprintf("Failed to open %s device\n", AIC_PSADC_NAME);
        return -RT_ERROR;
    }

#ifdef AIC_SYSCFG_DRV
    ref_voltage = hal_syscfg_read_ldo_cfg();
#endif
    if (!ref_voltage) {
        rt_kprintf("Failed to obtain reference voltage through eFuse\n");
        ref_voltage = (int)(def_voltage) * AIC_PSADC_VOLTAGE_ACCURACY;
    }
    rt_kprintf("Reference voltage: %d.%04d V\n",
               ref_voltage / AIC_PSADC_VOLTAGE_ACCURACY,
               ref_voltage % AIC_PSADC_VOLTAGE_ACCURACY);

    rt_adc_enable(psadc_dev, AIC_PSADC_QC_MODE);
    chan_cnt = rt_adc_control(psadc_dev, RT_ADC_CMD_GET_CHAN_COUNT, NULL);
    printf("Will get %d data from %d channels in %s mode\n\n", sample_num,
           chan_cnt,
#if defined(AIC_PSADC_DRV_POLL)
           "poll"
#elif defined(AIC_PSADC_DRV_DMA)
           "DMA"
#else
           "IRQ"
#endif
           );

#ifdef AIC_PSADC_DRV_DMA
    ret = psadc_get_adc_by_dma(ref_voltage, sample_num, chan_cnt);
#else
    if (!window) {
        printf("Cnt ");
        for (i = 0; i < chan_cnt; i++)
            printf("Ch%d_ADC Ch%d_Vol ", i, i);
        printf("Time(us)\n");
    }

    if (window)
        start_us = aic_get_time_us();

    while (cnt < sample_num) {
        cnt++;

        if (!window)
            start_us = aic_get_time_us();

#ifdef AIC_PSADC_DRV_POLL
        ret = rt_adc_control(psadc_dev, RT_ADC_CMD_GET_VALUES_POLL,
                             (void *)adc_values);
#else
        ret = rt_adc_control(psadc_dev, RT_ADC_CMD_GET_VALUES,
                             (void *)adc_values);
#endif
        if (ret < 0) {
            printf("Read timeout! return %d\n", ret);
            return -RT_ERROR;
        }

        if (window) {
            if (cnt && cnt % window == 0) {
                show_sample_rate(cnt, window, start_us);
                start_us = aic_get_time_us();
            }
        } else {
            end_us = aic_get_time_us();
            printf("%3d ", cnt);
            for (i = 0; i < chan_cnt; i++) {
                voltage = test_psadc_adc2voltage(ref_voltage, adc_values[i]);
                printf("%7d %2d.%02d V ", adc_values[i],
                           voltage / 100, voltage % 100);
            }
            if (start_us)
                printf("%8d\n", abs(end_us - start_us));
        }
#ifdef AIC_PSADC_TRIG_BY_SOFT
        aicos_msleep(500);
#endif
    }
#endif

    rt_adc_disable(psadc_dev, AIC_PSADC_QC_MODE);
    return ret;
}

static void cmd_test_psadc(int argc, char **argv)
{
    float def_voltage = AIC_PSADC_DEFAULT_VOLTAGE;
    bool show_status = false;
    int sample_num = 10;
    u32 window = 0;
    int c;

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 't':
            def_voltage = atof(optarg);
            break;
        case 's':
            show_status = true;
            break;
        case 'n':
            sample_num = atoi(optarg);
            break;
        case 'w':
            window = atoi(optarg);
            break;
        case 'h':
        default:
            cmd_psadc_usage(argv[0]);
            return;
        }
    }

    if (show_status) {
        aich_psadc_status_show();
        aicos_msleep(10);
        return;
    }

    if (def_voltage < 0) {
        rt_kprintf("Please set valid default voltage\n");
        return;
    }

    if (sample_num < 0) {
        rt_kprintf("Please set vaild sample count\n");
        return;
    }

    psadc_get_adc(def_voltage, sample_num, window);

}

MSH_CMD_EXPORT_ALIAS(cmd_test_psadc, test_psadc, psadc device sample);
