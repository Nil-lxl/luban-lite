/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2025-12-30        the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "jy6311_user_cfg.h"

static struct rt_i2c_bus_device * g_jy6311_client = RT_NULL;

void jy6311_delay_ms(int ms)
{
    rt_thread_mdelay(ms);
}

unsigned char jy6311_i2c_read_byte(unsigned char i2c_addr, unsigned char reg)
{
    struct rt_i2c_msg msg[2] = {0};
    uint8_t val = 0xff;

    msg[0].addr  = i2c_addr;
    msg[0].flags = RT_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = &reg;

    msg[1].addr  = i2c_addr;
    msg[1].flags = RT_I2C_RD;
    msg[1].len   = 1;
    msg[1].buf   = &val;

    if (rt_i2c_transfer(g_jy6311_client, msg, 2) != 2)
    {
        rt_kprintf("I2C read data failed, reg = 0x%02x. \n", i2c_addr);
        return 0xff;
    }

    return val;
}

signed char jy6311_i2c_write_byte(unsigned char i2c_addr, unsigned char reg, unsigned char val)
{
    struct rt_i2c_msg msgs[1] = {0};
    uint8_t buff[2] = {0};

    buff[0] = reg;
    buff[1] = val;

    msgs[0].addr  = i2c_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = buff;
    msgs[0].len   = 2;

    if (rt_i2c_transfer(g_jy6311_client, msgs, 1) != 1)
    {
        rt_kprintf("I2C write data failed, reg = 0x%2x. \n", reg);
        return RT_ERROR;
    }

    return RT_EOK;
}

void jy6311_i2c_get_client(void)
{
    g_jy6311_client = rt_i2c_bus_device_find(AIC_I2S_CODEC_JY6311_I2C);
    if (g_jy6311_client == RT_NULL) {
        rt_kprintf("Can't find %s device", AIC_I2S_CODEC_JY6311_I2C);
        return;
    }
}
