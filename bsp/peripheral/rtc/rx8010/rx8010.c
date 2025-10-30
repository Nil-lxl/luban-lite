/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-12-11        the first version
 */
#include "rx8010.h"
#define DBG_TAG "rx8010"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

struct rx8010_device
{
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c_device;
};
static struct rx8010_device g_rx8010_dev = {0};

/* bcd to bin */
static unsigned char bcd2bin(unsigned char data)
{
    unsigned char temp;

    temp = (((data) & 0x0f) + ((data) >> 4) * 10);
    return temp;
}

/* bin_to_bcd */
static unsigned char bin2bcd(unsigned char data)
{
    unsigned char temp;

    temp = ((((data) / 10) << 4) + (data) % 10);
    return temp;
}

/* rx8010 read register */
static rt_err_t rx8010_read_reg(rt_uint8_t reg, rt_uint8_t *data, rt_uint8_t data_size)
{
    struct rt_i2c_msg msg[2];

    msg[0].addr  = RX8010_ADDR;
    msg[0].flags = RT_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = &reg;

    msg[1].addr  = RX8010_ADDR;
    msg[1].flags = RT_I2C_RD;
    msg[1].len   = data_size;
    msg[1].buf   = data;

    if (rt_i2c_transfer(g_rx8010_dev.i2c_device, msg, 2) == 2) {
        return RT_EOK;
    } else {
        LOG_E("i2c bus read rx8010 failed!\r\n");
        return -RT_ERROR;
    }
}

/* rx8010 write register */
static rt_err_t rx8010_write_reg(rt_uint8_t reg, rt_uint8_t *data, rt_uint8_t data_size)
{
    struct rt_i2c_msg msg;
    rt_uint8_t buf[8] = {0};

    buf[0] = reg;
    rt_memcpy(&buf[1], data, data_size);

    msg.addr      = RX8010_ADDR;
    msg.flags     = RT_I2C_WR;
    msg.len       = data_size + 1;
    msg.buf       = buf;

    if (rt_i2c_transfer(g_rx8010_dev.i2c_device, &msg, 1) == 1) {
        return RT_EOK;
    } else {
        LOG_E("i2c bus write rx8010 failed!\r\n");
        return -RT_ERROR;
    }
}

static int rx8010_hw_init(void)
{
    uint8_t data;
    uint8_t ctrl[2] = {0};
    int need_clear = 0;

    /* Initialize reserved registers as specified in datasheet */
    data = 0xD8;
    if (rx8010_write_reg(RX8010_RESV17, &data, 1) != RT_EOK) {
        return -RT_ERROR;
    }

    data = 0x00;
    if (rx8010_write_reg(RX8010_RESV30, &data, 1) != RT_EOK) {
        return -RT_ERROR;
    }

    data = 0x08;
    if (rx8010_write_reg(RX8010_RESV31, &data, 1) != RT_EOK) {
        return -RT_ERROR;
    }

    data = 0x00;
    if (rx8010_write_reg(RX8010_IRQ, &data, 1) != RT_EOK) {
        return -RT_ERROR;
    }

    if (rx8010_read_reg(RX8010_FLAG, ctrl, sizeof(ctrl)) != RT_EOK) {
        return -RT_ERROR;
    }

    if (ctrl[0] & RX8010_FLAG_VLF)
        LOG_I("Frequency Stop Was Detected\n");

    if (ctrl[0] & RX8010_FLAG_AF) {
        LOG_I("Alarm was detected\n");
        need_clear = 1;
    }

    if (ctrl[0] & RX8010_FLAG_TF)
        need_clear = 1;

    if (ctrl[0] & RX8010_FLAG_UF)
        need_clear = 1;

    if (need_clear) {
        ctrl[0] &= ~(RX8010_FLAG_AF | RX8010_FLAG_TF | RX8010_FLAG_UF);

        if (rx8010_write_reg(RX8010_FLAG, &ctrl[0], 1) != RT_EOK)
            return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t rt_rx8010_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_rx8010_open(rt_device_t dev, rt_uint16_t flag)
{
    return RT_EOK;
}

static rt_err_t rt_rx8010_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_rx8010_control(rt_device_t dev, int cmd, void *args)
{
    rt_err_t ret = RT_EOK;
    time_t *time;
    struct tm time_temp;
    rt_uint8_t buff[RX8010_YEAR - RX8010_SEC + 1] = {0};

    RT_ASSERT(dev != RT_NULL);
    rt_memset(&time_temp, 0, sizeof(struct tm));

    switch (cmd)
    {
        case RT_DEVICE_CTRL_RTC_GET_TIME:
        {
            time = (time_t *)args;

            ret = rx8010_read_reg(RX8010_SEC, buff, sizeof(buff));
            if(ret == RT_EOK) {
                time_temp.tm_year  = bcd2bin(buff[RX8010_YEAR - RX8010_SEC]) + 100;
                time_temp.tm_mon   = bcd2bin(buff[RX8010_MONTH - RX8010_SEC] & 0x1f) - 1;
                time_temp.tm_mday  = bcd2bin(buff[RX8010_MDAY - RX8010_SEC] & 0x3f);
                time_temp.tm_hour  = bcd2bin(buff[RX8010_HOUR - RX8010_SEC] & 0x3f);
                time_temp.tm_min   = bcd2bin(buff[RX8010_MIN - RX8010_SEC] & 0x7f);
                time_temp.tm_sec   = bcd2bin(buff[RX8010_SEC - RX8010_SEC] & 0x7f);
                *time = mktime(&time_temp);
            }
        }
            break;

        case RT_DEVICE_CTRL_RTC_SET_TIME:
        {
            struct tm *time_new;

            time = (time_t *)args;
            time_new = localtime(time);
            buff[RX8010_YEAR - RX8010_SEC] = bin2bcd(time_new->tm_year - 100);
            buff[RX8010_MONTH - RX8010_SEC] = bin2bcd(time_new->tm_mon + 1);
            buff[RX8010_WDAY - RX8010_SEC] = bin2bcd(time_new->tm_wday + 1);
            buff[RX8010_MDAY - RX8010_SEC] = bin2bcd(time_new->tm_mday);
            buff[RX8010_HOUR - RX8010_SEC] = bin2bcd(time_new->tm_hour);
            buff[RX8010_MIN - RX8010_SEC] = bin2bcd(time_new->tm_min);
            buff[RX8010_SEC - RX8010_SEC] = bin2bcd(time_new->tm_sec);
            ret = rx8010_write_reg(RX8010_SEC, buff , sizeof(buff));
        }
            break;

        default:
            break;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops rx8010_ops =
{
    .init = rt_rx8010_init,
    .open = rt_rx8010_open,
    .close = rt_rx8010_close,
    .control = rt_rx8010_control,
};
#endif

/* rx8010 device int  */
int rt_hw_rx8010_init(void)
{
    struct rt_i2c_bus_device *i2c_device;

    i2c_device = rt_i2c_bus_device_find(AIC_RTC_I2C_CHAN);
    if (i2c_device == RT_NULL) {
        rt_kprintf("i2c bus device not found!\r\n");
        return RT_ERROR;
    }
    g_rx8010_dev.i2c_device = i2c_device;
    /* register rtc device */
#ifdef RT_USING_DEVICE_OPS
    g_rx8010_dev.dev.ops        = &rx8010_ops;
#else
    g_rx8010_dev.dev.init       = rt_rx8010_init;
    g_rx8010_dev.dev.open       = rt_rx8010_open;
    g_rx8010_dev.dev.close      = rt_rx8010_close;
    g_rx8010_dev.dev.control    = rt_rx8010_control;
#endif
    g_rx8010_dev.dev.type       = RT_Device_Class_RTC;

    /* init rx8010 */
    rt_device_register(&g_rx8010_dev.dev, "rtc", RT_DEVICE_FLAG_RDWR);

    if (rx8010_hw_init() != RT_EOK) {
        LOG_E("RTC RX8010 I2C Init Fail!\n");
        return -RT_ERROR;
    }

    LOG_I("Init RTC RX8010 Success!\n");

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_rx8010_init);
