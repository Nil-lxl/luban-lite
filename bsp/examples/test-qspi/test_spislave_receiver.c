/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xuan.Wen <xuan.wen@artinchip.com>
 */

#include <string.h>
#include <finsh.h>
#include <rtconfig.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <hal_qspi.h>
#include <rtthread.h>
#include "test_spislave.h"

#define RUN_STATE_IN 0
#define RUN_STATE_OUT 1

#define MEM_STATE_IDLE 0
#define MEM_STATE_CMD 1
#define MEM_STATE_DATA 2

struct qspirecv_state {
    u32 qspi_id;
    u32 bus_width;
    qspi_slave_handle handle;
    u8 *work_buf;
    u8 *tx_buf;
    u32 data_len;
};

static struct qspirecv_state g_state;

extern void slave_dump_data(char *msg, u8 *buf, u32 len);
#define USAGE \
    "spirecv help : Get this information.\n" \
    "spirecv start <spi_id> <bus_width> <rxtx> <data_len>\n" \
    "   rxtx: 0(default) - rx only; 1 - tx only; 2 - full duplex transfer, bus_width should be 1.\n" \
    "   data_len: 256 as default, the len master transfer should be 4 align, if not, you have to set it.\n" \
    "spirecv stop : stop the spi slave using.\n" \
    "example:\n" \
    "spirecv start 3 1 2 2048\n" \
    "spirecv stop\n" \

static void qspi_usage(void)
{
    printf("%s", USAGE);
}

static int recv_new_data(struct qspirecv_state *state, u8 *tx, u8 *rx, u32 len)
{
    struct qspi_transfer t;
    int ret;

    memset(&t, 0, sizeof(t));
    t.tx_data = tx;
    t.rx_data = rx;
    t.data_len = len;

    hal_qspi_slave_fifo_reset(&state->handle, HAL_QSPI_RX_FIFO);
    ret = hal_qspi_slave_transfer_async(&state->handle, &t);
    if (ret < 0)
        return -1;
    return 0;
}

static void qspirecv_slave_async_callback(qspi_slave_handle *h, void *priv)
{
    struct qspirecv_state *state = priv;
    int status, cnt;
    u32 *p32, cksum;
    u8 *p;

    status = hal_qspi_slave_get_status(&state->handle);
    cnt = 0;
    if (status == HAL_QSPI_STATUS_OK) {
        /*
         * status OK:
         *   TRANSFER DONE or CS INVALID
         */
        p = state->work_buf;
        cnt = hal_qspi_slave_transfer_count(&state->handle);
        printf("%s, status %d, cnt %d\n", __func__, status, cnt);
        if (state->work_buf) {
            p32 = (void *)state->work_buf;
            cksum = 0;
            for (int i = 0; i<g_state.data_len/4; i++) {
                cksum += *p32;
                p32++;
            }
            printf("cksum 0x%x\n", cksum);
            slave_dump_data("Recv data", p, cnt);
        }

        if (g_state.work_buf) {
            aicos_free_align(0, g_state.work_buf);
            g_state.work_buf = aicos_malloc_align(0, g_state.data_len, CACHE_LINE_SIZE);

            if (g_state.work_buf == NULL) {
                printf("malloc failure.\n");
                return;
            }
            rt_memset(g_state.work_buf, 0x2E, g_state.data_len);
        }
        if (g_state.tx_buf) {
            aicos_free_align(0, g_state.tx_buf);
            g_state.tx_buf = aicos_malloc_align(0, g_state.data_len, CACHE_LINE_SIZE);

            if (g_state.tx_buf == NULL) {
                printf("malloc failure.\n");
                return;
            }
            rt_memset(g_state.tx_buf, 0x2E, g_state.data_len);
            rt_memset(g_state.tx_buf, 0xA4, 16);
        }

        recv_new_data(state, state->tx_buf, state->work_buf, g_state.data_len);
    } else {
        /* Error process */
        printf("%s, status %d\n", __func__, status);
    }
}

static int test_qspirecv_start(int argc, char **argv)
{
    unsigned long val, rxtx = 0;
    int ret;

    if (argc < 2) {
        qspi_usage();
        return -1;
    }
    val = strtol(argv[1], NULL, 10);
    g_state.qspi_id = val;
    g_state.bus_width = 1; // Default is 1
    if (argc >= 3) {
        val = strtol(argv[2], NULL, 10);
        g_state.bus_width = val;
    }

    if (argc >= 4) {
        rxtx = strtol(argv[3], NULL, 10);
    }

    if (argc >= 5)
        g_state.data_len = strtol(argv[4], NULL, 10);
    else
        g_state.data_len = PKT_SIZE;

    /* rx or Full duplex mode */
    if (rxtx == 0 || rxtx == 2) {
        if (g_state.work_buf == NULL)
            g_state.work_buf = aicos_malloc_align(0, g_state.data_len, CACHE_LINE_SIZE);

        if (g_state.work_buf == NULL) {
            printf("malloc failure.\n");
            return -1;
        }
        rt_memset(g_state.work_buf, 0x2E, g_state.data_len);
    }

    /* tx or Full duplex mode */
    if (rxtx == 1 || rxtx == 2) {
        if (g_state.tx_buf == NULL)
            g_state.tx_buf = aicos_malloc_align(0, g_state.data_len, CACHE_LINE_SIZE);

        if (g_state.tx_buf == NULL) {
            printf("malloc failure.\n");
            return -1;
        }
        rt_memset(g_state.tx_buf, 0x2E, g_state.data_len);
        rt_memset(g_state.tx_buf, 0xA4, 16);
    }

    ret = test_qspi_slave_controller_init(g_state.qspi_id, g_state.bus_width,
                                          qspirecv_slave_async_callback, &g_state,
                                          &g_state.handle);
    if (ret) {
        printf("QSPI Slave init failure.\n");
        return -1;
    }

    /* Start with waiting command */
    recv_new_data(&g_state, g_state.tx_buf, g_state.work_buf, g_state.data_len);
    return 0;
}

static int test_qspirecv_stop(int argc, char **argv)
{
    test_qspi_slave_controller_deinit(&g_state.handle);
    if (g_state.work_buf) {
        aicos_free_align(0, g_state.work_buf);
        g_state.work_buf = NULL;
    }
    if (g_state.tx_buf) {
        aicos_free_align(0, g_state.tx_buf);
        g_state.tx_buf = NULL;
    }
    return 0;
}

static void cmd_test_qspislave_receiver(int argc, char **argv)
{
    if (argc < 2)
        goto help;

    if (!rt_strcmp(argv[1], "help")) {
        goto help;
    } else if (!rt_strcmp(argv[1], "start")) {
        test_qspirecv_start(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "stop")) {
        test_qspirecv_stop(argc - 1, &argv[1]);
        return;
    }
help:
    qspi_usage();
}

MSH_CMD_EXPORT_ALIAS(cmd_test_qspislave_receiver, spirecv, Test QSPI Slave);
