/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>
#include <rtthread.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "hal_i2c.h"

#define SAMPLE_I2C_NAME     "i2c0"
#define MAX_LEN             256

static struct rt_device_t *g_i2c_bus = RT_NULL;
static struct aic_i2c_slave_info g_slave_info;
static uint8_t g_write_index;
static uint8_t g_read_index;
static uint8_t g_slave_buf[MAX_LEN];
static uint8_t write_buf_index;
static uint8_t read_buf_index;

static int i2c_slave_cb(void *arg)
{
    struct slave_param *parm = (struct slave_param *)arg;

    uint32_t event = parm->cmd;
    uint8_t *val = parm->arg;

    switch (event)
    {
    case I2C_SLAVE_WRITE_RECEIVED:
        if (g_read_index == 0) {
            if ((*val) > (MAX_LEN -1)) {
                rt_kprintf("warning: the starting address of slave to be read is out of range!\n");
                return -RT_ERROR;
            }
            read_buf_index = *val;
            g_read_index++;
        }
        if (g_write_index == 0) {
            if ((*val) > (MAX_LEN -1)) {
                rt_kprintf("warning: the starting address of slave to be written is out of range!\n");
                return -RT_ERROR;
            }
            write_buf_index = *val;
            g_write_index++;
            break;
        }
        if (write_buf_index > (MAX_LEN -1)) {
            rt_kprintf("warning: the index of the slave to be written is out of range!\n");
            return -RT_ERROR;
        }
        g_slave_buf[write_buf_index++] = *val;
        break;
    case I2C_SLAVE_READ_PROCESSED:
        if (read_buf_index > (MAX_LEN -1)) {
            rt_kprintf("warning: the index of the slave to be read is out of range!\n");
            return -RT_ERROR;
        }
        read_buf_index++;
        break;
    case I2C_SLAVE_READ_REQUESTED:
        *val = g_slave_buf[read_buf_index];
        break;
    case I2C_SLAVE_STOP:
    case I2C_SLAVE_WRITE_REQUESTED:
        g_write_index = 0;
        g_read_index = 0;
        break;
    default:
        rt_kprintf("cmd parameter error\n");
        break;
    }

    return 0;
}

static int test_i2c_slave(int argc, char *argv[])
{
    int ret = RT_EOK;
    char i2c_name[RT_NAME_MAX] = SAMPLE_I2C_NAME;

    if (argc > 1)
        rt_strncpy(i2c_name, argv[1], RT_NAME_MAX);

    g_i2c_bus = (struct rt_device_t *)rt_device_find(i2c_name);
    if (!g_i2c_bus) {
        rt_kprintf("find %s failed!\n", g_i2c_bus);
        return -RT_ERROR;
    }

    rt_memset(g_slave_buf, 0xff, sizeof(g_slave_buf));
    g_slave_info.slave_cb = i2c_slave_cb;

    rt_device_control((struct rt_device *)g_i2c_bus, RT_I2C_DEV_SLAVE_CONFIG, &g_slave_info);

    return ret;
}

MSH_CMD_EXPORT_ALIAS(test_i2c_slave, test_i2c_slave, i2c slave test sample);
