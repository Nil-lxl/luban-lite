/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <drv_qspi.h>
#include <drivers/spi.h>

/* Ensure that "Board options/Using SPI1" is enabled before running this demo. */
#define QSPI_BUS_NAME "qspi1"   /* QSPI bus name */
#define QSPI_DEV_NAME "qspidev" /* QSPI bus name */
#define TEST_DATA_LEN 10        /* Test data length */
#define THREAD_PREPARE_EVENT (1 << 0)
#define THREAD_SEND_EVENT    (1 << 1)

static struct rt_qspi_device *g_qspi_dev = RT_NULL;
static rt_mq_t data_mq;
static rt_event_t g_event;

/**
 * Initialize QSPI device with default configuration
 *
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t qspi_demo_init(void)
{
    rt_err_t ret = RT_EOK;
    struct rt_qspi_configuration cfg;

    /* If already initialized, configure the default values directly */
    if (g_qspi_dev != RT_NULL) {
        goto exit;
    }

    ret = aic_qspi_bus_attach_device(QSPI_BUS_NAME, QSPI_DEV_NAME, 0, AIC_QSPI0_BUS_WIDTH, RT_NULL,
                                     RT_NULL);
    if (ret != RT_EOK) {
        pr_err("Failed to attach device in bus_name %s\n", QSPI_BUS_NAME);
        return ret;
    }

    g_qspi_dev = (struct rt_qspi_device *)rt_device_find(QSPI_DEV_NAME);
    if (g_qspi_dev == RT_NULL) {
        pr_err("Failed to get device in name %s\n", QSPI_BUS_NAME);
        return -RT_ERROR;
    }

exit:
    // /* Default configuration (Mode 0, MSB first, 8-bit, 1MHz) */
    rt_memset(&cfg, 0, sizeof(cfg));
    cfg.parent.mode = RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.parent.max_hz = 100000000;
    cfg.parent.data_width = 8;
    cfg.qspi_dl_width = 4;
    cfg.ddr_mode = 0;
    ret = rt_qspi_configure(g_qspi_dev, &cfg);
    if (ret != RT_EOK) {
        pr_err("QSPI config failed: %d\n", ret);
        return -ret;
    }

    rt_kprintf("[OK] QSPI initialized (dev:%s)\n", QSPI_DEV_NAME);
    return RT_EOK;
}

static void qspi_prepare_thread(void *parameter)
{
    rt_uint8_t data_buf[TEST_DATA_LEN];
    rt_uint32_t cnt = 0;

    while (1) {
        /* Prepare the data, here using memset as a simple substitute. */
        rt_memset(data_buf, 0x9f, sizeof(rt_uint8_t) * TEST_DATA_LEN);

        rt_mq_send(data_mq, data_buf, TEST_DATA_LEN);

        rt_kprintf("qspi prepare data success, cnt: %d\n", ++cnt);
        if (cnt >= 5)
            break;
    }
    rt_event_send(g_event, THREAD_PREPARE_EVENT);
}

static void qspi_send_thread(void *parameter)
{
    struct rt_spi_device *spi_dev = &(g_qspi_dev->parent);
    struct rt_qspi_message message;
    rt_uint32_t cnt = 0;
    rt_err_t ret = 0;
    rt_uint8_t *data_buf = NULL;

    data_buf = rt_malloc_align(TEST_DATA_LEN, 8);
    if (data_buf == NULL) {
        pr_err("data_buf malloc failure.\n");
        return;
    }

    ret = rt_spi_nonblock_set(spi_dev, 1);
    if (ret < 0) {
        pr_err("spi nonblock set failure. ret = %ld\n", ret);
        goto exit;
    }

    rt_memset(&message, 0, sizeof(message));
    message.qspi_data_lines = 1;
    message.parent.send_buf = data_buf;
    message.parent.recv_buf = RT_NULL;
    message.parent.length = TEST_DATA_LEN;
    message.parent.cs_take = 1;

    /**
     * After asynchronous transmission completes,the CS pin will be released in
     * the callback function, so this configuration is invalid.
     */
    message.parent.cs_release = 1;
    message.parent.next = RT_NULL;

    while (1) {
        rt_mq_recv(data_mq, data_buf, TEST_DATA_LEN, RT_WAITING_FOREVER);
        ret = rt_spi_take_bus(spi_dev);
        if (ret < 0) {
            pr_err("rt_spi_take_bus failure. ret = %ld\n", ret);
            goto exit;
        }
        rt_qspi_transfer_message(g_qspi_dev, &message);
        ret = rt_spi_wait_completion(spi_dev);
        if (ret < 0) {
            pr_err("rt_spi_wait_completion failure. ret = %ld\n", ret);
            goto exit;
        }
        ret = rt_spi_release_bus(spi_dev);
        if (ret < 0) {
            pr_err("rt_spi_release_bus failure. ret = %ld\n", ret);
            goto exit;
        }
        rt_kprintf("qspi send data success, cnt: %d\n", ++cnt);
        if (cnt >= 5)
            goto exit;
    }

exit:
    rt_free_align(data_buf);
    rt_spi_nonblock_set(spi_dev, 0);
    rt_event_send(g_event, THREAD_SEND_EVENT);
}

/**
 * Create two threads: one for asynchronous transmission and another for data preparation.
 * Using asynchronous sending allows the CPU to free up time to handle other matters
 * (such as preparing the data to be sent).
 *
 * Note: QSPI does not support asynchronous reception.
 *
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t qspi_async_send(void)
{
    rt_thread_t send_thread = NULL, prepare_thread = NULL;
    rt_uint32_t recv_event = 0;

    /* Create an event to manage two threads. */
    g_event = rt_event_create("qspi_manage_evt", RT_IPC_FLAG_PRIO);
    if (g_event == RT_NULL) {
        pr_err("Failed to create event g_event\n");
        return -RT_ERROR;
    }

    data_mq = rt_mq_create("data_mq", TEST_DATA_LEN, 1, RT_IPC_FLAG_FIFO);
    if (data_mq == RT_NULL) {
        pr_err("Failed to create message queue\n");
        return -RT_ERROR;
    }

    send_thread = rt_thread_create("qspi_send_thread", qspi_send_thread, RT_NULL, 1024 * 4, 24, 10);
    if (send_thread != RT_NULL) {
        rt_thread_startup(send_thread);
    } else {
        pr_err("Failed to create thread: qspi_send_thread\n");
        return -RT_ERROR;
    }

    prepare_thread =
        rt_thread_create("qspi_prepare_thread", qspi_prepare_thread, RT_NULL, 1024 * 4, 25, 10);
    if (prepare_thread != RT_NULL) {
        rt_thread_startup(prepare_thread);
    } else {
        pr_err("Failed to create thread: qspi_prepare_thread\n");
        rt_thread_delete(send_thread);
        return -RT_ERROR;
    }

    /* Wait for two threads to finish, then clean up resources. */
    rt_event_recv(g_event, THREAD_PREPARE_EVENT | THREAD_SEND_EVENT,
                  RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &recv_event);
    rt_mq_delete(data_mq);
    rt_event_delete(g_event);

    rt_kprintf("[OK] QSPI async send finish\n");
    return RT_EOK;
}

/**
 * Change QSPI transfer mode (0-3)
 *
 * @param mode Target QSPI mode (0-3)
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t qspi_change_mode(rt_uint8_t mode)
{
    struct rt_qspi_configuration cfg;
    rt_err_t ret = RT_EOK;

    if (!g_qspi_dev)
        return -RT_ENOSYS;

    rt_memcpy(&cfg, &g_qspi_dev->config, sizeof(cfg));

    /* Clear existing mode settings */
    cfg.parent.mode &= ~(RT_SPI_CPHA | RT_SPI_CPOL);

    /* Set new mode */
    switch (mode) {
        case 0:
            cfg.parent.mode |= RT_SPI_MODE_0;
            break;
        case 1:
            cfg.parent.mode |= RT_SPI_MODE_1;
            break;
        case 2:
            cfg.parent.mode |= RT_SPI_MODE_2;
            break;
        case 3:
            cfg.parent.mode |= RT_SPI_MODE_3;
            break;
        default:
            pr_err("Invalid mode %d\n", mode);
            return -RT_EINVAL;
    }

    ret = rt_qspi_configure(g_qspi_dev, &cfg);
    if (ret == RT_EOK) {
        rt_kprintf("[OK] QSPI mode changed to %d\n", mode);
    }
    return ret;
}

/**
 * Set bit transmission order (LSB/MSB first)
 *
 * @param lsb_first RT_TRUE for LSB first, RT_FALSE for MSB first
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t qspi_set_bit_order(rt_bool_t lsb_first)
{
    struct rt_qspi_configuration cfg;
    rt_err_t ret = RT_EOK;

    if (!g_qspi_dev)
        return -RT_ENOSYS;

    rt_memcpy(&cfg, &g_qspi_dev->config, sizeof(cfg));

    /* Set bit order */
    if (lsb_first) {
        cfg.parent.mode &= ~RT_SPI_MSB;

    } else {
        cfg.parent.mode |= RT_SPI_MSB;
    }

    ret = rt_qspi_configure(g_qspi_dev, &cfg);
    if (ret == RT_EOK) {
        rt_kprintf("[OK] Bit order set to %s first\n", lsb_first ? "LSB" : "MSB");
    }
    return ret;
}

/**
 * Adjust QSPI clock frequency
 *
 * @param hz New clock frequency in Hz
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t qspi_set_freq(uint32_t hz)
{
    struct rt_qspi_configuration cfg;
    rt_err_t ret = RT_EOK;

    if (!g_qspi_dev)
        return -RT_ENOSYS;

    rt_memcpy(&cfg, &g_qspi_dev->config, sizeof(cfg));
    cfg.parent.max_hz = hz;

    ret = rt_qspi_configure(g_qspi_dev, &cfg);
    if (ret == RT_EOK) {
        rt_kprintf("[OK] QSPI speed set to %d Hz\n", hz);
    }
    return ret;
}

/**
 * Comprehensive test function demonstrating:
 * 1. Async send
 * 2. Mode switching
 * 3. Bit order change
 * 4. Speed adjustment
 */
static int cmd_qspi_dev_usage(void)
{
    int err = RT_EOK;

    /* Initialize */
    err = qspi_demo_init();
    if (err != RT_EOK) {
        pr_err("qspi_demo_init failed!,err code:%d\n", err);
        return err;
    }

    /* Perform async send */
    err = qspi_async_send();
    if (err != RT_EOK) {
        pr_err("qspi_async_send failed!,err code:%d\n", err);
        return err;
    }

    /* Change to Mode 0 */
    err = qspi_change_mode(0);
    if (err != RT_EOK) {
        pr_err("qspi_change_mode failed!,err code:%d\n", err);
        return err;
    }

    /* Set MSB first */
    err = qspi_set_bit_order(RT_FALSE);
    if (err != RT_EOK) {
        pr_err("qspi_set_bit_order failed!,err code:%d\n", err);
        return err;
    }

    /* Adjust speed to 100MHz */
    err = qspi_set_freq(100000000);
    if (err != RT_EOK) {
        pr_err("qspi_set_freq failed!,err code:%d\n", err);
        return err;
    }

    rt_kprintf("[OK] All QSPI tests passed!\n");
    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_qspi_dev_usage, qspi_dev_usage, QSPI device example);
