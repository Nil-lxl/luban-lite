/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "boot_param.h"

#define WATER_MARK      64

#ifdef AIC_DMA_DRV
#ifdef FPGA_BOARD_ARTINCHIP
#define INPUTCAP_CLK_RATE          48000000 /* 48 MHz */
#else
#define INPUTCAP_CLK_RATE          200000000 /* 200 MHz */
#endif
rt_uint32_t data_buf0[WATER_MARK] __attribute__((aligned(32))) = {0};
rt_uint32_t data_buf1[WATER_MARK] __attribute__((aligned(32))) = {0};
rt_uint8_t event0_flag = 0;
rt_uint8_t event1_flag = 0;
#endif

/* callback function */
static rt_err_t inputcap_cb(rt_device_t dev, rt_size_t size)
{
#ifdef AIC_DMA_DRV
    rt_uint8_t *p = dev->user_data;

    if ((rt_uint32_t)*p == 0)
        event0_flag = 1;
    else
        event1_flag = 1;

#ifdef ULOG_USING_ISR_LOG
    rt_uint32_t temp_cnt;
    if ((rt_uint32_t)*p == 0) {
        for (int i = 0; i < WATER_MARK - 1; i++) {
            if (data_buf0[i + 1] > data_buf0[i])
                temp_cnt = data_buf0[i + 1] - data_buf0[i];
            else
                temp_cnt = data_buf0[i + 1] + ((0xFFFFFFFF - data_buf0[i]) + 1);

            rt_kprintf("%s event%d: pulsewidth:%d us\n", &dev->parent.name, (rt_uint32_t)*p, temp_cnt / (INPUTCAP_CLK_RATE / 1000000));
        }
    } else {
        for (int i = 0; i < WATER_MARK - 1; i++) {
            if (data_buf1[i + 1] > data_buf1[i])
                temp_cnt = data_buf1[i + 1] - data_buf1[i];
            else
                temp_cnt = data_buf1[i + 1] + ((0xFFFFFFFF - data_buf1[i]) + 1);

            rt_kprintf("%s event%d: pulsewidth:%d us\n", &dev->parent.name, (rt_uint32_t)*p, temp_cnt / (INPUTCAP_CLK_RATE / 1000000));
        }
    }
#endif

#else
    struct rt_inputcapture_data inputcap_data[WATER_MARK];
    rt_device_read(dev, 0, (void *)inputcap_data, size);
#ifdef ULOG_USING_ISR_LOG
    for (int i = 0; i < size; i++)
        rt_kprintf("%s: pulsewidth:%d us\n", &dev->parent.name, inputcap_data[i].pulsewidth_us);
#endif
#endif
    return RT_EOK;
}

int test_inputcap(int argc, char **argv)
{
    rt_uint32_t watermark = WATER_MARK;
    rt_device_t inputcap_dev = RT_NULL;
    char device_name[8] = {"incap"};
    int ret;

    if (argc != 2) {
        rt_kprintf("Usage: test_inputcap 0\n");
        return -RT_EINVAL;
    }

    strcat(device_name, argv[1]);

    inputcap_dev =  rt_device_find(device_name);
    if (inputcap_dev == RT_NULL) {
        rt_kprintf("Can't find %s device!\n", device_name);
        return -RT_EEMPTY;
    }

#ifdef AIC_DMA_DRV
    struct rt_inputcapture_fifo_buf buf_para;
    buf_para.event0_buf = data_buf0;
    buf_para.event0_buflen = sizeof(data_buf0);
    buf_para.event1_buf = data_buf1;
    buf_para.event1_buflen = sizeof(data_buf1);
    ret = rt_device_control(inputcap_dev, INPUTCAPTURE_CMD_SET_DATA_BUF, (void *)&buf_para);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to set %s device watermark!\n", device_name);
        return ret;
    }
#endif
    ret = rt_device_control(inputcap_dev, INPUTCAPTURE_CMD_SET_WATERMARK, &watermark);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to set %s device watermark!\n", device_name);
        return ret;
    }

    /* set callback function */
    rt_device_set_rx_indicate(inputcap_dev, inputcap_cb);

    ret = rt_device_open(inputcap_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to open %s device!\n", device_name);
        return ret;
    }

    rt_kprintf("inputcap%d open.\n", atoi(argv[1]));

#ifdef AIC_DMA_DRV
    int wait_count = 1000;
    while (1) {
        if (event0_flag == 1 && event1_flag == 1) {
            event0_flag = 0;
            event1_flag = 0;
            rt_device_close(inputcap_dev);
            break;
        }
        rt_thread_mdelay(10);
        wait_count--;
        if (wait_count == 0) {
            rt_kprintf("no signal input!\n");
            rt_device_close(inputcap_dev);
            break;
        }
    }
    rt_kprintf("inputcap%d close.\n", atoi(argv[1]));
#endif

    return 0;
}
MSH_CMD_EXPORT_ALIAS(test_inputcap, test_inputcap, Test the inputcap);
