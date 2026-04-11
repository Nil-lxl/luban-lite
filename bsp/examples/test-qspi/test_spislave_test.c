/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
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
#include "test_spislave.h"

struct bus_cfg {
    u8 cmd; /* CMD bus width */
    u8 addr; /* Address bus width */
    u8 dmycyc; /* Dummy clock cycle */
    u8 data; /* Data bus width */
};

extern void slave_dump_data(char *msg, u8 *buf, u32 len);

void master_tx(struct rt_qspi_device *qspi, u8 bus_width, u8 *buf, u32 datalen)
{
    struct rt_qspi_message msg;
    struct bus_cfg cfg;
    rt_size_t ret;

    cfg.cmd = 0;
    cfg.addr = 0;
    cfg.dmycyc = 0;
    cfg.data = bus_width;

    rt_memset(&msg, 0, sizeof(msg));
    msg.qspi_data_lines = cfg.data;
    msg.parent.recv_buf = NULL;
    msg.parent.send_buf = buf;
    msg.parent.length = datalen;
    msg.parent.cs_take = 1;
    msg.parent.cs_release = 1;
    rt_spi_take_bus((struct rt_spi_device *)qspi);
    ret = rt_qspi_transfer_message(qspi, &msg);
    if (ret != datalen) {
        printf("master tx failed. ret 0x%x\n", ret);
    }
    rt_spi_release_bus((struct rt_spi_device *)qspi);
}

void master_rx(struct rt_qspi_device *qspi, u8 bus_width, u8 *buf, u32 datalen)
{
    struct rt_qspi_message msg;
    struct bus_cfg cfg;
    rt_size_t ret;

    cfg.cmd = 0;
    cfg.addr = 0;
    cfg.dmycyc = 0;
    cfg.data = bus_width;

    rt_memset(&msg, 0, sizeof(msg));
    msg.qspi_data_lines = cfg.data;
    msg.parent.recv_buf = buf;
    msg.parent.send_buf = NULL;
    msg.parent.length = datalen;
    msg.parent.cs_take = 1;
    msg.parent.cs_release = 1;
    rt_spi_take_bus((struct rt_spi_device *)qspi);
    ret = rt_qspi_transfer_message(qspi, &msg);
    if (ret != datalen) {
        printf("master tx failed. ret 0x%x\n", ret);
    }
    rt_spi_release_bus((struct rt_spi_device *)qspi);
}

static void slave_get_status(struct rt_qspi_device *qspi, u8 bus_width, u8 *buf,
                             u32 datalen)
{
    master_rx(qspi, bus_width, buf, datalen);
}

void slave_write(struct rt_qspi_device *qspi, u8 bus_width, u8 *buf, u32 datalen, u32 addr)
{
    u8 *work_buf;
    u32 status;

    work_buf = aicos_malloc_align(0, datalen + 4, CACHE_LINE_SIZE);

    work_buf[0] = MEM_CMD_WRITE;
    memcpy(&work_buf[1], &addr, 3);
    memcpy(&work_buf[4], buf, datalen);
    master_tx(qspi, bus_width, work_buf, datalen + 4);

    rt_thread_mdelay(10);
    do {
        status = 0;
        slave_get_status(qspi, bus_width, (void *)&status, 4);
        printf("write status 0x%x\n", status);
        rt_thread_mdelay(1000);
    } while (status != WRITE_STATUS_VAL);

    aicos_free_align(0, work_buf);
}

static void slave_load_data(struct rt_qspi_device *qspi, u8 bus_width, u32 addr)
{
    u8 work_buf[4];

    work_buf[0] = MEM_CMD_LOAD;
    memcpy(&work_buf[1], &addr, 3);
    master_tx(qspi, bus_width, work_buf, 4);
}

static void slave_read_data(struct rt_qspi_device *qspi, u8 bus_width,
                            u8 *buf, u32 datalen)
{
    master_rx(qspi, bus_width, buf, datalen);
}

void slave_read(struct rt_qspi_device *qspi, u8 bus_width, u8 *buf, u32 datalen,
                u32 addr)
{
    u32 status;

    slave_load_data(qspi, bus_width, addr);
    rt_thread_mdelay(100);
    do {
        status = 0;
        slave_get_status(qspi, bus_width, (void *)&status, 4);
        printf("load status 0x%x\n", status);
        rt_thread_mdelay(1000);
    } while (status != LOAD_STATUS_VAL);

    slave_read_data(qspi, bus_width, buf, datalen);
}
