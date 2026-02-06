/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji.CHen <jiji.chen@artinchip.com>
 */


#include <stdint.h>
#include <string.h>
#include <rtdevice.h>
#include <aic_utils.h>
#include <aic_core.h>
#include <finsh.h>
#include "xip_ramcode.h"

#define __is_print(ch)                ((unsigned int)((ch) - ' ') < 127u - ' ')

#define HEXDUMP_WIDTH                 16
#define CMD_PROBE_INDEX               0
#define CMD_READ_INDEX                1
#define CMD_WRITE_INDEX               2
#define CMD_ERASE_INDEX               3

const char* test_xip_ramcode_help_info[] = {
    [CMD_PROBE_INDEX]     = "xip_ramcode init <addr_4bmode> <spienc_en>  - init xip_ramcode handler",
    [CMD_READ_INDEX]      = "xip_ramcode read [addr] [size]              - read 'size' bytes starting at 'addr' data",
    [CMD_WRITE_INDEX]     = "xip_ramcode write [addr] [size] [src_addr]  - write 'size' bytes to 'addr' from 'src_addr'",
    [CMD_ERASE_INDEX]     = "xip_ramcode erase [addr] [size]             - erase 'size' bytes starting at 'addr'",
};

static int xip_ramcode_inited = 0;

static void xip_ramcode_usage()
{
    int i;

    rt_kprintf("Usage:\n");
    for (i = 0; i < sizeof(test_xip_ramcode_help_info) / sizeof(char*); i++) {
        rt_kprintf("%s\n", test_xip_ramcode_help_info[i]);
    }
    rt_kprintf("\n");
    return;
}

static void hex_dump(uint8_t *data, uint32_t addr, uint32_t size, uint32_t dump_width)
{
    uint32_t i, j;

    for (i = 0; i < size; i += dump_width)
    {
        rt_kprintf("[%08X] ", addr + i);
        /* dump hex */
        for (j = 0; j < dump_width; j++) {
            if (i + j < size) {
                rt_kprintf("%02X ", data[i + j]);
            } else {
                rt_kprintf("   ");
            }
        }
        /* dump char for hex */
        for (j = 0; j < dump_width; j++) {
            if (i + j < size) {
                rt_kprintf("%c", __is_print(data[i + j]) ? data[i + j] : '.');
            }
        }
        rt_kprintf("\n");
    }
}

static void xip_ramcode_init(uint8_t argc, char **argv)
{
    u32 addr_4bmode = 0, spienc = 0;
    int ret = 0;

    if (xip_ramcode_inited) {
        rt_kprintf("xip write has been inited.\n");
        return;
    }

    if (argc >= 2) {
        addr_4bmode = strtol(argv[0], NULL, 0);
        spienc = strtol(argv[1], NULL, 0);
    }

    aic_xip_ramcode_prepare();

    ret = aic_xip_ramcode_init(addr_4bmode, spienc);
    if (ret) {
        rt_kprintf("xip_ramcode_init failed!\n");
        return;
    }
    xip_ramcode_inited = 1;
    return;
}

static void xip_ramcode_read(uint8_t argc, char **argv)
{
    int result = 0;
    uint32_t addr, size;
    uint64_t start_us;
    uint8_t *data;

    if (argc < 2) {
        rt_kprintf("Usage: %s.\n", test_xip_ramcode_help_info[CMD_READ_INDEX]);
        return;
    }

    addr = strtol(argv[0], NULL, 0);
    size = strtol(argv[1], NULL, 0);
    data = aicos_malloc_align(0, size, CACHE_LINE_SIZE);

    if (data) {
        start_us =  aic_get_time_us();
        result = aic_xip_ramcode_spinor_read(addr, size, data);
        show_speed("xip_code read flash speed", size, aic_get_time_us() - start_us);

        if (result == 0) {
            rt_kprintf("Read flash data success. Start from 0x%08X, size is %ld.\n", addr, size);
            rt_kprintf("The data is:\n");
            rt_kprintf("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
            hex_dump(data, addr, size, HEXDUMP_WIDTH);
            rt_kprintf("\n");
        }
        aicos_free_align(0, data);
    } else {
        rt_kprintf("Low memory!\n");
    }
}

static void xip_ramcode_write(uint8_t argc, char **argv)
{
    uint32_t addr, size, srcd;
    uint8_t *data;
    int i;
    if (argc < 3) {
        rt_kprintf("Usage: %s.\n", test_xip_ramcode_help_info[CMD_READ_INDEX]);
        return;
    }

    addr = strtol(argv[0], NULL, 0);
    size = strtol(argv[1], NULL, 0);
    srcd = strtol(argv[2], NULL, 0);
    data = aicos_malloc_align(0, size, CACHE_LINE_SIZE);
    if (data) {
        memcpy(data, (void *)srcd, size);
        if (0 == aic_xip_ramcode_spinor_write(addr, size, data)) {
            rt_kprintf("Write the flash data success. Start from 0x%08X, size is %ld.\n", addr, size);
            rt_kprintf("Write data: ");
            for (i = 0; i < size; i++) {
                if (i % 16 == 0)
                    rt_kprintf("\n");
                rt_kprintf("%02x ", data[i]);
            }
            rt_kprintf(".\n");
        }
        aicos_free_align(0, data);
    } else {
        rt_kprintf("Low memory!\n");
    }
}

static void xip_ramcode_erase(uint8_t argc, char **argv)
{
    uint32_t addr, size;

    if (argc < 2) {
        rt_kprintf("Usage: %s.\n", test_xip_ramcode_help_info[CMD_ERASE_INDEX]);
        return;
    }

    addr = strtol(argv[0], NULL, 0);
    size = strtol(argv[1], NULL, 0);
    if (0 == aic_xip_ramcode_spinor_erase(addr, size)) {
        rt_kprintf("Erase flash data success. Start from 0x%08X, size is %ld.\n", addr, size);
    }
}

static void xip_ramcode(uint8_t argc, char **argv)
{
    if (argc < 2) {
        xip_ramcode_usage();
        return;
    }
    const char *operator = argv[1];

    if (!strcmp(operator, "init")) {
        xip_ramcode_init(argc - 2, &argv[2]);
    } else if (!rt_strcmp(operator, "read")) {
        xip_ramcode_read(argc - 2, &argv[2]);
    } else if (!rt_strcmp(operator, "write")) {
        xip_ramcode_write(argc - 2, &argv[2]);
    } else if (!rt_strcmp(operator, "erase")) {
        xip_ramcode_erase(argc - 2, &argv[2]);
    } else {
        xip_ramcode_usage();
    }
}
MSH_CMD_EXPORT(xip_ramcode, Write Flash with XIP mode);
