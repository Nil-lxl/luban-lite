/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
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

static struct rt_qspi_device *g_qspi = NULL;
static struct rt_spi_device *g_spi = NULL;
static int g_async = 0;
static int g_flag_use_qspi = 0;
#define PREFIX_QSPI 1
#define PREFIX_SPI 0

#define USAGE                                                                   \
    "spi_async help : Get this information.\n"                                \
    "spi_async attach <bus name> <dev name> : Attach device to SPI bus.\n"    \
    "spi_async init <name> <async_en>: Initialize SPI for async mode device.\n" \
    "spi_async send <send_len>: Send data\n" \
    "spi_async attach qspi1 qtestdev\n"                                       \
    "spi_async init qtestdev 1\n"                                             \
    "spi_async send 0x100\n"

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

static int check_use_qspi(const char *str) {
    if (strncmp(str, "qspi", 4) == 0) {
        return PREFIX_QSPI;
    }

    if (strncmp(str, "spi", 3) == 0) {
        return PREFIX_SPI;
    }

    return -1;
}

static int test_spi_attach_spi_device(char *bus_name, char *dev_name)
{
    struct rt_spi_device *spi_device = RT_NULL;
    rt_err_t result = RT_EOK;


    spi_device =
        (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (spi_device == RT_NULL) {
        printf("malloc failed.\n");
        return -RT_ERROR;
    }

    result = rt_spi_bus_attach_device(spi_device, dev_name,
        bus_name, RT_NULL);
    if (result < 0)
        printf("spi_async attach device failed! bus name: %s, device name: %s\n", bus_name, dev_name);

    if (result != RT_EOK)
        rt_free(spi_device);

    return 0;
}

static int test_spi_attach_qspi_device(char *bus_name, char *dev_name)
{
    struct rt_qspi_device *qspi_device = RT_NULL;
    rt_err_t result = RT_EOK;


    qspi_device = (struct rt_qspi_device *)rt_malloc(sizeof(struct rt_qspi_device));
    if (qspi_device == RT_NULL) {
        printf("malloc failed.\n");
        return -RT_ERROR;
    }

    qspi_device->enter_qspi_mode = NULL;
    qspi_device->exit_qspi_mode = NULL;
    qspi_device->config.qspi_dl_width = 1;  // data_width

    result = rt_spi_bus_attach_device(&qspi_device->parent, dev_name,
        bus_name, RT_NULL);
    if (result < 0)
        printf("spi_async attach device failed! bus name: %s, device name: %s\n", bus_name, dev_name);

    if (result != RT_EOK)
        rt_free(qspi_device);

    return 0;
}

static int test_spi_attach(int argc, char **argv)
{
    char *bus_name, *dev_name;

    if (argc != 3) {
        spi_usage();
        return -1;
    }
    bus_name = argv[1];
    dev_name = argv[2];

    g_flag_use_qspi = check_use_qspi(bus_name);
    if (g_flag_use_qspi == PREFIX_QSPI) {
        test_spi_attach_qspi_device(bus_name, dev_name);
        return 0;
    } else if (g_flag_use_qspi == PREFIX_SPI) {
        test_spi_attach_spi_device(bus_name, dev_name);
        return 0;
    }

    printf("[ERROR] bus name: (%s) unsupport! %d\n", bus_name, g_flag_use_qspi);
    return -1;
}

static int test_spi_init_spi_device(char *devname, int set_noblock)
{
    struct rt_spi_configuration spi_cfg;
    struct rt_device *dev;
    int ret = 0;

    g_spi = (struct rt_spi_device *)rt_device_find(devname);
    if (!g_spi) {
        printf("Failed to get device in name %s\n", devname);
        return -1;
    }

    dev = (struct rt_device *)g_spi;
    if (dev->type != RT_Device_Class_SPIDevice) {
        g_spi = NULL;
        printf("%s is not SPI device.\n", devname);
        return -1;
    }

    rt_memset(&spi_cfg, 0, sizeof(spi_cfg));
    spi_cfg.mode = 0;
    spi_cfg.max_hz = 50000000;
    ret = rt_spi_configure(g_spi, &spi_cfg);

    ret = rt_spi_nonblock_set(g_spi, set_noblock);

    if (ret < 0) {
        printf("spi nonblock set failure. ret = %d\n", ret);
        return ret;
    }

    return 0;
}

static int test_spi_init_qspi_device(char *devname, int set_noblock)
{
    struct rt_qspi_configuration qspi_cfg;
    struct rt_device *dev;
    int ret = 0;

    g_qspi = (struct rt_qspi_device *)rt_device_find(devname);
    if (!g_qspi) {
        printf("Failed to get device in name %s\n", devname);
        return -1;
    }

    dev = (struct rt_device *)g_qspi;
    if (dev->type != RT_Device_Class_SPIDevice) {
        g_qspi = NULL;
        printf("%s is not SPI device.\n", devname);
        return -1;
    }

    rt_memset(&qspi_cfg, 0, sizeof(qspi_cfg));
    qspi_cfg.qspi_dl_width = 1;
    qspi_cfg.parent.mode = 0;
    qspi_cfg.parent.max_hz = 50000000;
    ret = rt_qspi_configure(g_qspi, &qspi_cfg);

    g_spi = (struct rt_spi_device*) &g_qspi->parent;

    ret = rt_spi_nonblock_set(g_spi, set_noblock);

    if (ret < 0) {
        printf("spi nonblock set failure. ret = %d\n", ret);
        return ret;
    }

    return 0;
}

static int test_spi_init(int argc, char **argv)
{
    int set_noblock = 0;
    char *name;

    if (argc != 3) {
        spi_usage();
        return -1;
    }
    name = argv[1];

    if (atol(argv[2]) == 1) {
        set_noblock = 1;
    }
    g_async = set_noblock;

    if (g_flag_use_qspi == PREFIX_QSPI)
        test_spi_init_qspi_device(name, set_noblock);
    else
        test_spi_init_spi_device(name, set_noblock);

    return 0;
}

static int test_spi_async_send_spi_device(uint8_t *data, uint32_t send_len)
{
    uint32_t count = 0;
    rt_size_t ret;

    if (g_spi == NULL) {
        printf("[ERROR] g_spi did not init!\n");
        return -1;
    }

    rt_spi_take_bus((struct rt_spi_device *)g_spi);

    unsigned long start_us = aic_get_time_us();
    ret = rt_spi_transfer(g_spi, (void *)data, NULL, send_len);
    if (g_async == 1)
        show_speed("async speed", send_len, aic_get_time_us() - start_us);
    else
        show_speed("sync speed", send_len, aic_get_time_us() - start_us);

    if (g_async == 1) {
        while (rt_spi_get_transfer_status(g_spi) != 0) {
            count++;
        }
        show_speed("sync speed", send_len, aic_get_time_us() - start_us);
    }

    rt_spi_release_bus((struct rt_spi_device *)g_spi);

    if (ret != send_len) {
        printf("Send data failed. ret 0x%x\n", (int)ret);
    }

    return 0;
}

static int test_spi_async_send_qspi_device(uint8_t *data, uint32_t send_len)
{
    struct rt_qspi_message msg;
    uint32_t count = 0;
    rt_size_t ret;

    if (g_qspi == NULL) {
        printf("[ERROR] g_spi did not init!\n");
        return -1;
    }

    rt_memset(&msg, 0, sizeof(msg));
    msg.instruction.content = 0;
    msg.instruction.qspi_lines = 0;
    msg.dummy_cycles = 0;

    msg.qspi_data_lines = 1;
    msg.parent.send_buf = (void *)data;
    msg.parent.length = send_len;
    msg.parent.cs_take = 1;
    msg.parent.cs_release = 1;

    rt_spi_take_bus((struct rt_spi_device *)g_qspi);

    unsigned long start_us = aic_get_time_us();
    ret = rt_qspi_transfer_message(g_qspi, &msg);
    if (g_async == 1)
        show_speed("async speed", send_len, aic_get_time_us() - start_us);
    else
        show_speed("sync speed", send_len, aic_get_time_us() - start_us);

    if (g_async == 1) {
        while (rt_spi_get_transfer_status((struct rt_spi_device *)g_qspi)  != 0) {
            count++;
        }
        show_speed("async mode spi true speed", send_len, aic_get_time_us() - start_us);
    }

    rt_spi_release_bus((struct rt_spi_device *)g_qspi);

    if (ret != send_len) {
        printf("Send data failed. ret 0x%x\n", (int)ret);
    }

    return 0;
}

static int test_spi_async_send(int argc, char **argv)
{
    uint32_t align_len = 0, send_len = 0;
    uint8_t *data = 0;

    if (argc != 2) {
        spi_usage();
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

    if (g_flag_use_qspi == PREFIX_QSPI)
        test_spi_async_send_qspi_device(data, align_len);
    else
        test_spi_async_send_spi_device(data, align_len);

    aicos_free_align(MEM_DEFAULT, data);

    return 0;
}

static void cmd_test_spi_async(int argc, char **argv)
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
    } else if (!rt_strcmp(argv[1], "send")) {
        test_spi_async_send(argc - 1, &argv[1]);
        return;
    }
help:
    spi_usage();
}

MSH_CMD_EXPORT_ALIAS(cmd_test_spi_async, spi_async, Test spi async transfer);

#define WORK_IN_SPI_MODE 0

void spi_thread_entry(void *parameter)
{
    u64 start_ms;
#if WORK_IN_SPI_MODE
    char *dev_name = "spi3_dev";
    char *bus_name = "spi3";
#else
    char *dev_name = "qspi1_dev";
    char *bus_name = "qspi1";
#endif
    u8 *data;
    u32 non_block = 1;

    g_spi = (struct rt_spi_device *)rt_device_find(dev_name);
    if (!g_spi) {
        printf("try to attach bus:%s dev: %s\n", bus_name, dev_name);
        test_spi_attach_spi_device(bus_name, dev_name);
    }
#if WORK_IN_SPI_MODE
    test_spi_init_spi_device(dev_name, non_block);
#else
    test_spi_init_qspi_device(dev_name, non_block);
#endif

    data = aicos_malloc_align(MEM_DEFAULT, 65536, CACHE_LINE_SIZE);

    rt_spi_take_bus((struct rt_spi_device *)g_spi);
    start_ms = aic_get_time_ms();

    while (1)
    {
        rt_spi_transfer(g_spi, (void *)data, NULL, 65536);
        if (non_block) {
            rt_spi_wait_completion(g_spi);
        }

        if (aic_get_time_ms() - start_ms > 30000)
            break;
    }

    rt_spi_release_bus((struct rt_spi_device *)g_spi);
    aicos_free_align(MEM_DEFAULT, data);
}

static void cmd_test_spi_async_thread(int argc, char **argv)
{
    rt_thread_t tid = RT_NULL;

    tid = rt_thread_create("spi_async_thread", spi_thread_entry, RT_NULL, 8192, 22, 5);
    if (tid != RT_NULL)
        rt_thread_startup(tid);

}

MSH_CMD_EXPORT_ALIAS(cmd_test_spi_async_thread, spi_async_thread, Test spi async transfer);
