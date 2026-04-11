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

/* Ensure that "Board options/Using SPI3" is enabled before running this demo. */
#define SPI_BUS_NAME    "spi3"   /* SPI bus name */
#define SPI_DEVICE_NAME "spidev" /* SPI device name */
#define TEST_DATA_SIZE  4        /* Test data length */

static struct rt_spi_device *g_spi_dev = RT_NULL;

/**
 * Initialize SPI device with default configuration
 *
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t spi_demo_init(void)
{
    rt_err_t ret = RT_EOK;
    struct rt_spi_configuration cfg;

    /* If already initialized, configure the default values directly */
    if (g_spi_dev != RT_NULL) {
        goto config;
    }

    g_spi_dev = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (g_spi_dev == RT_NULL) {
        rt_kprintf("malloc failed.\n");
        return -RT_ERROR;
    }

    ret = rt_spi_bus_attach_device(g_spi_dev, SPI_DEVICE_NAME, SPI_BUS_NAME, RT_NULL);
    if (ret != RT_EOK && g_spi_dev != NULL) {
        rt_free(g_spi_dev);
        return -RT_ERROR;
    }

config:
    /* Default configuration (Mode 0, MSB first, 8-bit, 1MHz) */
    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.max_hz = 1 * 1000 * 1000;
    cfg.data_width = 8;
    ret = rt_spi_configure(g_spi_dev, &cfg);
    if (ret != RT_EOK) {
        pr_err("SPI config failed: %d\n", ret);
        return -ret;
    }

    rt_kprintf("[OK] SPI initialized (dev:%s)\n", SPI_DEVICE_NAME);
    return RT_EOK;
}

/**
 * Perform full-duplex SPI transfer
 *
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t spi_full_duplex_transfer(void)
{
    struct rt_spi_message msg;
    rt_uint32_t i = 0;
    rt_uint8_t *tx_buf = NULL;
    rt_uint8_t *rx_buf = NULL;
    rt_err_t err = RT_EOK;

    if (!g_spi_dev)
        return -RT_ENOSYS;

    tx_buf = aicos_malloc_align(0, TEST_DATA_SIZE, CACHE_LINE_SIZE);
    rx_buf = aicos_malloc_align(0, TEST_DATA_SIZE, CACHE_LINE_SIZE);
    if (tx_buf == NULL || rx_buf == NULL) {
        printf("Low memory!\n");
        err = RT_ENOMEM;
        goto exit;
    }
    for (i = 0; i < TEST_DATA_SIZE; i++) {
        tx_buf[i] = i % 0x100;
    }

    msg.send_buf = &tx_buf;
    msg.recv_buf = &rx_buf;
    msg.length = sizeof(tx_buf);
    msg.cs_take = 1;
    msg.cs_release = 0;
    msg.next = RT_NULL;

    rt_kprintf("Transfer start...\n");
    rt_spi_transfer_message(g_spi_dev, &msg);

    rt_kprintf("[OK] Transfer data success\n");
    rt_kprintf("TX: ");
    for (i = 0; i < TEST_DATA_SIZE; i++) {
        rt_kprintf("%02X ", tx_buf[i]);
    }
    rt_kprintf("\nRX: ");
    for (i = 0; i < TEST_DATA_SIZE; i++) {
        rt_kprintf("%02X ", rx_buf[i]);
    }
    rt_kprintf("\n");

exit:
    if (tx_buf)
        aicos_free_align(0, tx_buf);
    if (rx_buf)
        aicos_free_align(0, rx_buf);
    return -err;
}

/**
 * Change SPI transfer mode (0-3)
 *
 * @param mode Target SPI mode (0-3)
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t spi_change_mode(rt_uint8_t mode)
{
    struct rt_spi_configuration cfg;
    rt_err_t ret = RT_EOK;

    if (!g_spi_dev)
        return -RT_ENOSYS;

    rt_memcpy(&cfg, &g_spi_dev->config, sizeof(cfg));

    /* Clear existing mode settings */
    cfg.mode &= ~(RT_SPI_CPHA | RT_SPI_CPOL);

    /* Set new mode */
    switch (mode) {
        case 0:
            cfg.mode |= RT_SPI_MODE_0;
            break;
        case 1:
            cfg.mode |= RT_SPI_MODE_1;
            break;
        case 2:
            cfg.mode |= RT_SPI_MODE_2;
            break;
        case 3:
            cfg.mode |= RT_SPI_MODE_3;
            break;
        default:
            pr_err("Invalid mode %d\n", mode);
            return -RT_EINVAL;
    }

    ret = rt_spi_configure(g_spi_dev, &cfg);
    if (ret == RT_EOK) {
        rt_kprintf("[OK] SPI mode changed to %d\n", mode);
    }
    return ret;
}

/**
 * Set bit transmission order (LSB/MSB first)
 *
 * @param lsb_first RT_TRUE for LSB first, RT_FALSE for MSB first
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t spi_set_bit_order(rt_bool_t lsb_first)
{
    struct rt_spi_configuration cfg;
    rt_err_t ret = RT_EOK;

    if (!g_spi_dev)
        return -RT_ENOSYS;

    rt_memcpy(&cfg, &g_spi_dev->config, sizeof(cfg));

    /* Set bit order */
    if (lsb_first) {
        cfg.mode &= ~RT_SPI_MSB;

    } else {
        cfg.mode |= RT_SPI_MSB;
    }

    ret = rt_spi_configure(g_spi_dev, &cfg);
    if (ret == RT_EOK) {
        rt_kprintf("[OK] Bit order set to %s first\n", lsb_first ? "LSB" : "MSB");
    }
    return ret;
}

/**
 * Enable/disable 3-wire half-duplex mode
 *
 * @param enable RT_TRUE to enable, RT_FALSE to disable
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t spi_set_3wire_mode(rt_bool_t enable)
{
    rt_err_t ret = RT_EOK;

    if (!g_spi_dev)
        return -RT_ENOSYS;

    struct rt_spi_configuration cfg;
    rt_memcpy(&cfg, &g_spi_dev->config, sizeof(cfg));

    /* Set 3-wire mode */
    if (enable) {
        cfg.mode |= RT_SPI_3WIRE;
        rt_kprintf("[WARN] MOSI/MISO merged to single line\n");
    } else {
        cfg.mode &= ~RT_SPI_3WIRE;
    }

    ret = rt_spi_configure(g_spi_dev, &cfg);
    if (ret == RT_EOK) {
        rt_kprintf("[OK] 3-wire mode %s\n", enable ? "enabled" : "disabled");
    }
    return ret;
}

/**
 * Adjust SPI clock frequency
 *
 * @param hz New clock frequency in Hz
 * @return RT_EOK on success, error code on failure
 */
static rt_err_t spi_set_freq(uint32_t hz)
{
    struct rt_spi_configuration cfg;
    rt_err_t ret = RT_EOK;

    if (!g_spi_dev)
        return -RT_ENOSYS;

    rt_memcpy(&cfg, &g_spi_dev->config, sizeof(cfg));
    cfg.max_hz = hz;

    ret = rt_spi_configure(g_spi_dev, &cfg);
    if (ret == RT_EOK) {
        rt_kprintf("[OK] SPI speed set to %d Hz\n", hz);
    }
    return ret;
}

/**
 * Comprehensive test function demonstrating:
 * 1. Full-duplex transfer
 * 2. Mode switching (to Mode 2)
 * 3. Bit order change (to LSB first)
 * 4. 3-wire mode enable
 * 5. Speed adjustment (to 10MHz)
 */
static int cmd_spi_dev_usage(void)
{
    int err = RT_EOK;

    /* Initialize if needed */
    err = spi_demo_init();
    if (err != RT_EOK) {
        pr_err("spi_demo_init failed!,err code:%d\n", err);
        return err;
    }

    /* Perform full-duplex transfer */
    err = spi_full_duplex_transfer();
    if (err != RT_EOK) {
        pr_err("spi_full_duplex_transfer failed!,err code:%d\n", err);
        return err;
    }

    /* Change to Mode 2 */
    err = spi_change_mode(2);
    if (err != RT_EOK) {
        pr_err("spi_change_mode failed!,err code:%d\n", err);
        return err;
    }

    /* Set LSB first */
    err = spi_set_bit_order(RT_TRUE);
    if (err != RT_EOK) {
        pr_err("spi_set_bit_order failed!,err code:%d\n", err);
        return err;
    }

    /* Enable 3-wire mode */
    err = spi_set_3wire_mode(RT_TRUE);
    if (err != RT_EOK) {
        pr_err("spi_set_3wire_mode failed!,err code:%d\n", err);
        return err;
    }

    /* Adjust speed to 10MHz */
    err = spi_set_freq(10 * 1000 * 1000);
    if (err != RT_EOK) {
        pr_err("spi_set_freq failed!,err code:%d\n", err);
        return err;
    }

    rt_kprintf("[OK] All SPI tests passed!\n");
    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_spi_dev_usage, spi_dev_usage, SPI device example);
