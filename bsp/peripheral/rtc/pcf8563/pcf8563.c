/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-11-15        the first version
 */
#include "pcf8563.h"
#define DBG_TAG "pcf8563"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

struct pcf8563_device
{
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c_device;
};
static struct pcf8563_device g_pcf8563_dev = {0};

/* bcd to hex */
static unsigned char bcd_to_hex(unsigned char data)
{
    unsigned char temp;

    temp = ((data >> 4) * 10 + (data & 0x0f));
    return temp;
}

/* hex_to_bcd */
static unsigned char hex_to_bcd(unsigned char data)
{
    unsigned char temp;

    temp = (((data / 10) << 4) + (data % 10));
    return temp;
}

/* pcf8563 read register */
static rt_err_t pcf8563_read_reg(rt_uint8_t reg, rt_uint8_t *data, rt_uint8_t data_size)
{
    struct rt_i2c_msg msg[2];

    msg[0].addr  = PCF8563_ARRD;
    msg[0].flags = RT_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = &reg;

    msg[1].addr  = PCF8563_ARRD;
    msg[1].flags = RT_I2C_RD;
    msg[1].len   = data_size;
    msg[1].buf   = data;

    if (rt_i2c_transfer(g_pcf8563_dev.i2c_device, msg, 2) == 2) {
        return RT_EOK;
    } else {
        LOG_E("i2c bus read pcf8563 failed!\r\n");
        return -RT_ERROR;
    }
}

/* pcf8563 write register */
static rt_err_t pcf8563_write_reg(rt_uint8_t reg, rt_uint8_t *data, rt_uint8_t data_size)
{
    struct rt_i2c_msg msg;
    rt_uint8_t buf[8] = {0};

    buf[0] = reg;
    rt_memcpy(&buf[1], data, data_size);

    msg.addr      = PCF8563_ARRD;
    msg.flags     = RT_I2C_WR;
    msg.len       = data_size + 1;
    msg.buf       = buf;

    if (rt_i2c_transfer(g_pcf8563_dev.i2c_device, &msg, 1) == 1) {
        return RT_EOK;
    } else {
        LOG_E("i2c bus write pcf8563 failed!\r\n");
        return -RT_ERROR;
    }
}

static rt_err_t rt_pcf8563_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_pcf8563_open(rt_device_t dev, rt_uint16_t flag)
{
    return RT_EOK;
}

static rt_err_t rt_pcf8563_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_pcf8563_control(rt_device_t dev, int cmd, void *args)
{
    rt_err_t ret = RT_EOK;
    time_t *time;
    struct tm time_temp;
    rt_uint8_t buff[7] = {0};

    RT_ASSERT(dev != RT_NULL);
    rt_memset(&time_temp, 0, sizeof(struct tm));

    switch (cmd)
    {
        case RT_DEVICE_CTRL_RTC_GET_TIME:
        {
            time = (time_t *)args;
            ret = pcf8563_read_reg(REG_PCF8563_SEC,buff,7);

            if(ret == RT_EOK) {
                time_temp.tm_year  = bcd_to_hex(buff[6] &  SHIELD_PCF8563_YEAR) + 2000 - 1900;
                time_temp.tm_mon   = bcd_to_hex(buff[5] &  SHIELD_PCF8563_MON) - 1;
                time_temp.tm_mday  = bcd_to_hex(buff[3] &  SHIELD_PCF8563_DAY);
                time_temp.tm_hour  = bcd_to_hex(buff[2] &  SHIELD_PCF8563_HOUR);
                time_temp.tm_min   = bcd_to_hex(buff[1] &  SHIELD_PCF8563_MIN);
                time_temp.tm_sec   = bcd_to_hex(buff[0] &  SHIELD_PCF8563_SEC);
                *time = mktime(&time_temp);
            }
        }
            break;

        case RT_DEVICE_CTRL_RTC_SET_TIME:
        {
            struct tm *time_new;

            time = (time_t *)args;
            time_new = localtime(time);
            buff[6] = hex_to_bcd(time_new->tm_year + 1900 - 2000);
            buff[5] = hex_to_bcd(time_new->tm_mon + 1);
            buff[3] = hex_to_bcd(time_new->tm_mday);
            buff[4] = hex_to_bcd(time_new->tm_wday+1);
            buff[2] = hex_to_bcd(time_new->tm_hour);
            buff[1] = hex_to_bcd(time_new->tm_min);
            buff[0] = hex_to_bcd(time_new->tm_sec);
            ret = pcf8563_write_reg(REG_PCF8563_SEC, buff , 7);
        }
            break;

        default:
            break;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops pcf8563_ops =
{
    .init = rt_pcf8563_init,
    .open = rt_pcf8563_open,
    .close = rt_pcf8563_close,
    .control = rt_pcf8563_control,
};
#endif

/* pcf8563 device int  */
int rt_hw_pcf8563_init(void)
{
    struct rt_i2c_bus_device *i2c_device;
    uint8_t data;

    i2c_device = rt_i2c_bus_device_find(AIC_RTC_I2C_CHAN);
    if (i2c_device == RT_NULL) {
        rt_kprintf("i2c bus device not found!\r\n");
        return RT_ERROR;
    }
    g_pcf8563_dev.i2c_device = i2c_device;
    /* register rtc device */
#ifdef RT_USING_DEVICE_OPS
    g_pcf8563_dev.dev.ops        = &pcf8563_ops;
#else
    g_pcf8563_dev.dev.init       = rt_pcf8563_init;
    g_pcf8563_dev.dev.open       = rt_pcf8563_open;
    g_pcf8563_dev.dev.close      = rt_pcf8563_close;
    g_pcf8563_dev.dev.control    = rt_pcf8563_control;
#endif
    g_pcf8563_dev.dev.type       = RT_Device_Class_RTC;

    /* init pcf8563 */
    rt_device_register(&g_pcf8563_dev.dev, "rtc", RT_DEVICE_FLAG_RDWR);
    data = 0x7f;    /* close clock out */
    if (pcf8563_write_reg(REG_PCF8563_CLKOUT, &data, 1) != RT_EOK) {
        LOG_E("Disabled RTC Clock Out Fail!\n");
        return -RT_ERROR;
    }
    LOG_I("Init RTC PCF8563 Success!\n");

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_pcf8563_init);
