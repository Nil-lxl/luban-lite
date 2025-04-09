/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
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

static struct rt_qspi_device *g_qspi;
struct rt_spi_device *g_spi;
int g_async = 0;

#define USAGE                                                                   \
    "spi_async help : Get this information.\n"                                \
    "spi_async attach <bus name> <dev name> : Attach device to SPI bus.\n"    \
    "spi_async init <name> <async_en>: Initialize SPI for async mode device.\n" \
    "spi_async send <send_len>: Send data\n" \
    "spi_async attach qspi1 qtestdev\n"                                       \
    "spi_async init qtestdev 1\n"                                             \
    "spi_async send 0x100\n"

static void spibit_usage(void)
{
    printf("%s", USAGE);
}

static void show_speed(char *msg, u32 len, u32 us)
{
    u32 tmp, speed;

    /* Split to serval step to avoid overflow */
    tmp = 1000 * len;
    tmp = tmp / us;
    tmp = 1000 * tmp;
    speed = tmp / 1024;

    printf("%s: %d byte, %d us -> %d KB/s\n", msg, len, us, speed);
}

static int test_spibit_attach(int argc, char **argv)
{
    char *bus_name, *dev_name;
    rt_err_t result = RT_EOK;

    if (argc != 3) {
        spibit_usage();
        return -1;
    }
    bus_name = argv[1];
    dev_name = argv[2];

    /* Attach/Create dev to spi bus */
    result = aic_qspi_bus_attach_device(bus_name, dev_name, 0, 1, RT_NULL, RT_NULL);
    if (result < 0)
        printf("spi_async attach device failed! bus name: %s, device name: %s\n", bus_name, dev_name);
    return 0;
}

static int test_spibit_init(int argc, char **argv)
{
    struct rt_qspi_configuration qspi_cfg;
    struct rt_device *dev;
    int ret = 0;
    char *name;

    if (argc != 3) {
        spibit_usage();
        return -1;
    }
    name = argv[1];

    g_qspi = (struct rt_qspi_device *)rt_device_find(name);
    if (!g_qspi) {
        printf("Failed to get device in name %s\n", name);
        return -1;
    }

    dev = (struct rt_device *)g_qspi;
    if (dev->type != RT_Device_Class_SPIDevice) {
        g_qspi = NULL;
        printf("%s is not SPI device.\n", name);
        return -1;
    }

    rt_memset(&qspi_cfg, 0, sizeof(qspi_cfg));
    qspi_cfg.qspi_dl_width = 1;
    qspi_cfg.parent.mode = 0;
    qspi_cfg.parent.max_hz = 50000000;
    ret = rt_qspi_configure(g_qspi, &qspi_cfg);

    g_spi = (struct rt_spi_device*) &g_qspi->parent;

    if (atol(argv[2]) == 1) {
        ret = rt_spi_nonblock_set(g_spi, 1);
        g_async = 1;
    } else {
        ret = rt_spi_nonblock_set(g_spi, 0);
        g_async = 0;
    }

    if (ret < 0) {
        printf("spi nonblock set failure. ret = %d\n", ret);
        return ret;
    }

    return 0;
}

static int test_spi_async_send(int argc, char **argv)
{
    uint32_t align_len = 0, send_len = 0;
    uint8_t *data = 0;
    uint32_t count = 0;
    struct rt_qspi_message msg;
    rt_size_t ret;

    if (argc != 2) {
        spibit_usage();
        return -1;
    }

    send_len = strtoul(argv[1], NULL, 0);

    if (send_len) {
        align_len = roundup(send_len, CACHE_LINE_SIZE);
        data = aicos_malloc_align(MEM_DEFAULT, align_len, CACHE_LINE_SIZE);
    }
    if (!data) {
        printf("send data alloc failed.\n");
        return 0;
    }

    rt_memset(&msg, 0, sizeof(msg));
    msg.instruction.content = 0;
    msg.instruction.qspi_lines = 0;
    msg.dummy_cycles = 0;

    msg.qspi_data_lines = 1;
    msg.parent.send_buf = (void *)data;
    msg.parent.length = align_len;
    msg.parent.cs_take = 1;
    msg.parent.cs_release = 1;
    rt_spi_take_bus((struct rt_spi_device *)g_qspi);

    unsigned long start_us = aic_get_time_us();
    ret = rt_qspi_transfer_message(g_qspi, &msg);
    if (g_async == 1)
        show_speed("async speed", align_len, aic_get_time_us() - start_us);
    else
        show_speed("sync speed", align_len, aic_get_time_us() - start_us);

    rt_spi_release_bus((struct rt_spi_device *)g_qspi);
    if (g_async == 1) {
        while (rt_spi_get_transfer_status(g_spi) != 0) {
            count++;
        }
    show_speed("sync speed", align_len, aic_get_time_us() - start_us);
    }

    aicos_free_align(MEM_DEFAULT, data);
    if (ret != align_len) {
        printf("Send data failed. ret 0x%x\n", (int)ret);
    }

    return 0;
}

static void cmd_test_spi_async(int argc, char **argv)
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
    } else if (!rt_strcmp(argv[1], "send")) {
        test_spi_async_send(argc - 1, &argv[1]);
        return;
    }
help:
    spibit_usage();
}

MSH_CMD_EXPORT_ALIAS(cmd_test_spi_async, spi_async, Test spi async transfer);
