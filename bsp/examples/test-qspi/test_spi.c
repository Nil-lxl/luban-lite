/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji Chen <jiji.chen@artinchip.com>
 */

#include <string.h>
#include <finsh.h>
#include <rtdevice.h>
#include <aic_core.h>

#define USAGE \
    "test_spi help : Get this information.\n" \
    "test_spi attach <bus name> <dev name> : Attach device to SPI bus.\n" \
    "test_spi init <name> <mode> <freq> : Initialize SPI for device.\n" \
    "test_spi send <len> : Send data.\n" \
    "example:\n" \
    "test_spi attach spi3 spidev\n" \
    "test_spi init spidev 0 50000000\n" \
    "test_spi send 32\n" \
    "test_spi transfer 32\n" \
    "test_spi send_recv 33 128\n" \


static void spi_usage(void)
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

static void hex_dump(uint8_t *data, unsigned long len)
{
    unsigned long i = 0;
    printf("\n");
    for (i = 0; i < len; i++) {
        if (i && (i % 16) == 0)
            printf("\n");
        printf("%02x ", data[i]);
    }
    printf("\n");
}

static struct rt_spi_device *g_spi;


static int test_spi_attach(int argc, char **argv)
{
    struct rt_spi_device *spi_device = RT_NULL;
    char *bus_name, *dev_name;
    rt_err_t result = RT_EOK;

    if (argc != 3) {
        spi_usage();
        return -1;
    }
    bus_name = argv[1];
    dev_name = argv[2];

    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(dev_name != RT_NULL);

    spi_device =
        (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (spi_device == RT_NULL) {
        printf("malloc failed.\n");
        return -RT_ERROR;
    }

    result = rt_spi_bus_attach_device(spi_device, dev_name,
                                      bus_name, RT_NULL);

    if (result != RT_EOK && spi_device != NULL)
        rt_free(spi_device);

    return result;
}

static int test_spi_init(int argc, char **argv)
{
    struct rt_spi_configuration spi_cfg;
    struct rt_device *dev;
    char *name;
    int ret = 0;

    if (argc != 4) {
        printf("Argument error, please see help information.\n");
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
    rt_memset(&spi_cfg, 0, sizeof(spi_cfg));
    spi_cfg.mode = atol(argv[2]);
    spi_cfg.max_hz = atol(argv[3]);
    ret = rt_spi_configure(g_spi, &spi_cfg);
    if (ret < 0) {
        printf("qspi configure failure.\n");
        return ret;
    }
    return 0;
}

static void test_spi_send_recv(int argc, char **argv)
{
    unsigned long  send_len, recv_len, align_len, start_us;
    uint8_t *send_buf, *recv_buf;
    rt_err_t ret;

    if (!g_spi) {
        printf("SPI device is not init yet.\n");
        return;
    }
    if (argc < 3) {
        printf("Argument is not correct, please see help for more information.\n");
        return;
    }

    send_len = 0;
    send_len = strtoul(argv[1], NULL, 0);
    recv_len = strtoul(argv[2], NULL, 0);

    /* transfer len can not be 0 */
    if (!send_len || !recv_len)
        return;

    align_len = roundup(send_len, CACHE_LINE_SIZE);
    send_buf = aicos_malloc_align(0, align_len, CACHE_LINE_SIZE);
    u8 *temp = send_buf;
    int k;
    for (k = 0; k < send_len; k++) {
        *temp = k & 0xff;
        temp++;
    }
    align_len = roundup(recv_len, CACHE_LINE_SIZE);
    recv_buf = aicos_malloc_align(0, align_len, CACHE_LINE_SIZE);
    rt_memset(recv_buf, 0xee, recv_len);

    if (send_buf == NULL || recv_buf == NULL) {
        printf("Low memory!\n");
        return;
    } else {
        printf("send len %ld, recv len %ld\n", send_len, recv_len);
    }

    rt_spi_take_bus((struct rt_spi_device *)g_spi);
    start_us = aic_get_time_us();
    ret = rt_spi_send_then_recv(g_spi, (void *)send_buf, send_len, (void *)recv_buf, recv_len);
    show_speed("spi send recv speed", send_len + recv_len, aic_get_time_us() - start_us);
    if (ret != 0) {
        printf("Send_recv data failed. ret = %ld\n", ret);
    }
    rt_spi_release_bus((struct rt_spi_device *)g_spi);
    if (send_buf)
        aicos_free_align(0, send_buf);
    if (recv_buf) {
        printf("receive data:\n");
        hex_dump(recv_buf, recv_len);
        printf("\n");
        aicos_free_align(0, recv_buf);
    }
}

static void test_spi_send(int argc, char **argv)
{
    unsigned long  data_len, align_len, start_us;
    uint8_t *data;
    rt_size_t ret;

    if (!g_spi) {
        printf("SPI device is not init yet.\n");
        return;
    }
    if (argc < 2) {
        printf("Argument is not correct, please see help for more information.\n");
        return;
    }

    data_len = 0;
    data_len = strtoul(argv[1], NULL, 0);
    data = RT_NULL;
    if (data_len) {
        align_len = roundup(data_len, CACHE_LINE_SIZE);
        data = aicos_malloc_align(0, align_len, CACHE_LINE_SIZE);
        u8 *temp = data;
        int k;
        for (k = 0; k < data_len; k++) {
            *temp = k & 0xff;
            temp++;
        }
    }
    if (data == NULL) {
        printf("Low memory!\n");
        return;
    } else {
        printf("data len %ld\n", data_len);
    }

    rt_spi_take_bus((struct rt_spi_device *)g_spi);
    start_us = aic_get_time_us();
    ret = rt_spi_transfer(g_spi, (void *)data, NULL, data_len);
    show_speed("spi send speed", data_len, aic_get_time_us() - start_us);
    if (ret != data_len) {
        printf("Send data failed. ret 0x%x\n", (int)ret);
    }
    rt_spi_release_bus((struct rt_spi_device *)g_spi);
    if (data)
        aicos_free_align(0, data);
}

static void test_spi_treansfer(int argc, char **argv)
{
    unsigned long  data_len, align_len, start_us;
    uint8_t *data, *recv;
    rt_size_t ret;

    if (!g_spi) {
        printf("SPI device is not init yet.\n");
        return;
    }
    if (argc < 2) {
        printf("Argument is not correct, please see help for more information.\n");
        return;
    }

    data_len = 0;
    data_len = strtoul(argv[1], NULL, 0);
    data = RT_NULL;
    if (data_len) {
        align_len = roundup(data_len, CACHE_LINE_SIZE);
        data = aicos_malloc_align(0, align_len, CACHE_LINE_SIZE);
        recv = aicos_malloc_align(0, align_len, CACHE_LINE_SIZE);
        u8 *temp = data;
        int k;
        for (k = 0; k < data_len; k++) {
            *temp = k & 0xff;
            temp++;
        }
        rt_memset(recv, 0xee, align_len);
    }
    if (data == NULL) {
        printf("Low memory!\n");
        return;
    } else {
        printf("data len %ld\n", data_len);
    }

    rt_spi_take_bus((struct rt_spi_device *)g_spi);
    start_us = aic_get_time_us();
    ret = rt_spi_transfer(g_spi, (void *)data, (void *)recv, data_len);
    show_speed("spi transfer speed", data_len, aic_get_time_us() - start_us);
    if (ret != data_len) {
        printf("Transfer data failed. ret 0x%x\n", (int)ret);
    }
    rt_spi_release_bus((struct rt_spi_device *)g_spi);
    if (data)
        aicos_free_align(0, data);

    if (recv) {
        printf("receive data:\n");
        hex_dump(recv, data_len);
        printf("\n");
        aicos_free_align(0, recv);
    }
}

static void cmd_test_spi(int argc, char **argv)
{
    if (argc < 2)
        goto help;

    if (!rt_strcmp(argv[1], "help")) {
        goto help;
    } else if (!rt_strcmp(argv[1], "attach")) {
        test_spi_attach(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "init")) {
        test_spi_init(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "send_recv")) {
        test_spi_send_recv(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "send")) {
        test_spi_send(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "transfer")) {
        test_spi_treansfer(argc - 1, &argv[1]);
        return;
    }

help:
    spi_usage();
}

MSH_CMD_EXPORT_ALIAS(cmd_test_spi, test_spi, Test SPI);
