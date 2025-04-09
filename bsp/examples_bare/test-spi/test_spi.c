/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiji.chen <jiji.chen@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_errno.h>
#include <aic_utils.h>
#include <sfud.h>

#define SPI_HELP                           \
    "test_spi command:\n"            \
    "  test_spi init [spi bus id]\n"            \
    "  test_spi send  [addr] [size]\n"        \
    "  test_spi recv <len>\n"                   \
    "e.g.: \n"                                \
    "  test_spi init 0\n"        \
    "  mtd alloc 4096\n"         \
    "  Alloc buffer: 0x300422c0\n"  \
    "  mtd_gendata 0x300422c0 0x100 0xA0\n"\
    "  test_spi send  0x300422c0 0x100\n" \
    "  test_spi recv 0x100\n"

extern struct aic_qspi_bus *qspi_probe(u32 spi_bus);
extern int spi_write_read(struct aic_qspi_bus *qspi,
                               const uint8_t *write_buf, size_t write_size,
                               uint8_t *read_buf, size_t read_size);

static void spi_help(void)
{
    puts(SPI_HELP);
}

struct aic_qspi_bus *g_spi_device;

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



static int do_qspi_init(int argc, char *argv[])
{
    unsigned long id;
    struct aic_qspi_bus *qspi;

    if (argc < 2) {
        spi_help();
        return 0;
    }
    id = strtol(argv[1], NULL, 0);

    qspi = qspi_probe(id);
    if (qspi == NULL) {
        printf("Failed to probe spinor flash.\n");
        return -1;
    }

    printf("probe qspi device success.\n");

    g_spi_device = qspi;
    return 0;
}

static int test_spi_send(int argc, char **argv)
{
    int ret = 0;
    u32 send_buf;
    size_t send_len;

    if (argc < 3) {
        spi_help();
        return 0;
    }

    if (!g_spi_device) {
        printf("init spi device first\n");
        return -1;
    }
    send_buf = strtoul(argv[1], NULL, 0);
    send_len = strtoul(argv[2], NULL, 0);

    ret = spi_write_read(g_spi_device, (u8 *)send_buf, send_len, NULL, 0);

    if (send_buf)
        hex_dump((u8 *)send_buf, send_len);


    return ret;
}

static int test_spi_recv(int argc, char **argv)
{
    int ret = 0;
    u8 *data;
    size_t recv_len;

    if (argc < 2) {
        spi_help();
        return 0;
    }

    if (!g_spi_device) {
        printf("init spi device first\n");
        return -1;
    }
    recv_len = strtoul(argv[1], NULL, 0);
    data = aicos_malloc_align(0, roundup(recv_len, CACHE_LINE_SIZE), CACHE_LINE_SIZE);
    if (data == NULL) {
        printf("[Error]: malloc failed.\n");
        return -1;
    } else {
        memset(data, 0x77, recv_len);
    }

    ret = spi_write_read(g_spi_device, NULL, 0, data, recv_len);

    if (data)
        hex_dump(data, recv_len);

    return ret;
}

static int spi_do_test(int argc, char *argv[])
{
    if (argc < 3) {
        spi_help();
        return 0;
    }

    if (!strncmp(argv[1], "init", 4))
        return do_qspi_init(argc - 1, &argv[1]);
    if (!strncmp(argv[1], "send", 4))
        return test_spi_send(argc - 1, &argv[1]);
    if (!strncmp(argv[1], "recv", 4))
        return test_spi_recv(argc - 1, &argv[1]);

    spi_help();
    return 0;
}

CONSOLE_CMD(test_spi, spi_do_test,  "spi send, receive test");
