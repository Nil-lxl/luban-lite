/*
 * Copyright (c) 2023-2024S, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xuan.Wen <xuan.wen@artinchip.com>
 */

#include <string.h>
#include <finsh.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <drv_qspi.h>

static struct rt_spi_device *g_spi;

#define USAGE                                                                   \
    "test_spibit help : Get this information.\n"                                \
    "test_spibit attach <bus name> <dev name> : Attach device to SPI bus.\n"    \
    "test_spibit init <name> <bit_mode> <freq>: Initialize SPI for bit mode device.\n" \
    "test_spibit send_recv <send_val> <send_len> <recv_len>: Write the addr first, then read the data\n" \
    "test_spibit attach qspi0 qtestdev\n"                                       \
    "test_spibit init qtestdev 1 20000000\n"                                             \
    "test_spibit send_recv 0x0f 10 20\n"

static void spibit_usage(void)
{
    printf("%s", USAGE);
}

static int test_spibit_attach(int argc, char **argv)
{
    char *bus_name, *dev_name;

    if (argc != 3) {
        spibit_usage();
        return -1;
    }
    bus_name = argv[1];
    dev_name = argv[2];

    /* Attach/Create dev to spi bus */
    aic_qspi_bus_attach_device(bus_name, dev_name, 0, 1, RT_NULL, RT_NULL);
    return 0;
}

static int test_spibit_init(int argc, char **argv)
{
    struct rt_spi_configuration spi_cfg;
    struct rt_device *dev;
    char *name;
    int ret = 0;

    if (argc < 3) {
        spibit_usage();
        return -1;
    }
    name = argv[1];

    g_spi = (struct rt_spi_device *)rt_device_find(name);
    if (!g_spi) {
        printf("Failed to get device in name %s\n", name);
        return -1;
    }

    dev = (struct rt_device *)g_spi;
    if (dev->type != RT_Device_Class_SPIDevice) {
        g_spi = NULL;
        printf("%s is not SPI device.\n", name);
        return -1;
    }

    if (atol(argv[2]))
        spi_cfg.mode = 3 | RT_SPI_3WIRE;
    else
        spi_cfg.mode = 3;

    if (argc > 3) {
        spi_cfg.max_hz = atol(argv[3]);
    } else {
        /* default 50MHz */
        spi_cfg.max_hz = 50000000;
    }

    ret = rt_spi_configure(g_spi, &spi_cfg);
    if (ret < 0) {
        printf("spi configure failure.\n");
        return ret;
    }
    return 0;
}

static int test_spibit_send_recv(int argc, char **argv)
{
    uint32_t send_val = 0;
    uint32_t send_len = 0;
    uint32_t recv_len = 0;
    uint32_t data = 0;

    if (argc != 4) {
        spibit_usage();
        return -1;
    }

    send_val = strtoul(argv[1], NULL, 0);
    send_len = strtol(argv[2], NULL, 0);
    recv_len = strtol(argv[3], NULL, 0);

    pr_info("send_val = %d, send_len = %d, recv_len = %d\n", send_val, send_len, recv_len);

    rt_spi_take_bus((struct rt_spi_device *)g_spi);
    /*note: the length units of the following parameters are bits,
      not bytes*/
    rt_spi_send_then_recv(g_spi, &send_val, send_len, &data, recv_len);
    rt_spi_release_bus((struct rt_spi_device *)g_spi);
    pr_info("data = 0x%x\n", data);

    return 0;
}

static void cmd_test_spibit(int argc, char **argv)
{
    if (argc < 2)
        goto help;

    if (!rt_strcmp(argv[1], "help")) {
        goto help;
    } else if (!rt_strcmp(argv[1], "attach")) {
        test_spibit_attach(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "init")) {
        test_spibit_init(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "send_recv")) {
        test_spibit_send_recv(argc - 1, &argv[1]);
        return;
    }
help:
    spibit_usage();
}

MSH_CMD_EXPORT_ALIAS(cmd_test_spibit, test_spibit, Test spi bit mode);
