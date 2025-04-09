/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#define LOG_TAG     "sc030iot"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "drv_camera.h"
#include "camera_inner.h"

/* Default format configuration of SC030IOT */
#define SC030IOT_DFT_WIDTH       VGA_WIDTH
#define SC030IOT_DFT_HEIGHT      VGA_HEIGHT
#define SC030IOT_DFT_BUS_TYPE    MEDIA_BUS_PARALLEL
#define SC030IOT_DFT_CODE        MEDIA_BUS_FMT_YUYV8_2X8

#define SC030IOT_I2C_SLAVE_ID    0x68
#define SC030IOT_CHIP_ID         0x9A46

#define SC030_SENSOR_ID_HIGH_REG 0xF7
#define SC030_SENSOR_ID_LOW_REG  0xF8

// 640*480, xclk=20M, fps=50fps, xclk=10M, fps=25fps
static const struct reg8_info sc030iot_init_regs[] = {
    {0xf0, 0x30},
    {0x01, 0xff},
    {0x02, 0xff},
    {0x22, 0x07},
    {0x19, 0xff},
    {0x3f, 0x82},
    {0x30, 0x02},
    {0xf0, 0x01},
    {0x70, 0x00},
    {0x71, 0x80},
    {0x72, 0x20},
    {0x73, 0x00},
    {0x74, 0xe0},
    {0x75, 0x10},
    {0x76, 0x81},
    {0x77, 0x88},
    {0x78, 0xe1},
    {0x79, 0x01},
    {0xf5, 0x01},
    {0xf4, 0x0a},
    {0xf0, 0x36},
    {0x37, 0x79},
    {0x31, 0x82},
    {0x3e, 0x60},
    {0x30, 0xf0},
    {0x33, 0x33},
    {0xf0, 0x32},
    {0x48, 0x02},
    {0xf0, 0x33},
    {0x02, 0x12},
    {0x7c, 0x02},
    {0x7d, 0x0e},
    {0xa2, 0x04},
    {0x5e, 0x06},
    {0x5f, 0x0a},
    {0x0b, 0x58},
    {0x06, 0x38},
    {0xf0, 0x32},
    {0x48, 0x02},
    {0xf0, 0x39},
    {0x02, 0x70},
    {0xf0, 0x45},
    {0x09, 0x1c},
    {0xf0, 0x37},
    {0x22, 0x0d},
    {0xf0, 0x33},
    {0x33, 0x10},
    {0xb1, 0x80},
    {0x34, 0x40},
    {0x0b, 0x54},
    {0xb2, 0x78},
    {0xf0, 0x36},
    {0x11, 0x80},
    {0xf0, 0x30},
    {0x38, 0x44},
    {0xf0, 0x33},
    {0xb3, 0x51},
    {0x01, 0x10},
    {0x0b, 0x6c},
    {0x06, 0x24},
    {0xf0, 0x36},
    {0x31, 0x82},
    {0x3e, 0x60},
    {0x30, 0xf0},
    {0x33, 0x33},
    {0xf0, 0x34},
    {0x9f, 0x02},
    {0xa6, 0x40},
    {0xa7, 0x47},
    {0xe8, 0x5f},
    {0xa8, 0x51},
    {0xa9, 0x44},
    {0xe9, 0x36},
    {0xf0, 0x33},
    {0xb3, 0x51},
    {0x64, 0x17},
    {0x90, 0x01},
    {0x91, 0x03},
    {0x92, 0x07},
    {0x01, 0x10},
    {0x93, 0x10},
    {0x94, 0x10},
    {0x95, 0x10},
    {0x96, 0x01},
    {0x97, 0x07},
    {0x98, 0x1f},
    {0x99, 0x10},
    {0x9a, 0x20},
    {0x9b, 0x28},
    {0x9c, 0x28},
    {0xf0, 0x36},
    {0x70, 0x54},
    {0xb6, 0x40},
    {0xb7, 0x41},
    {0xb8, 0x43},
    {0xb9, 0x47},
    {0xba, 0x4f},
    {0xb0, 0x8b},
    {0xb1, 0x8b},
    {0xb2, 0x8b},
    {0xb3, 0x9b},
    {0xb4, 0xb8},
    {0xb5, 0xf0},
    {0x7e, 0x41},
    {0x7f, 0x47},
    {0x77, 0x80},
    {0x78, 0x84},
    {0x79, 0x8a},
    {0xa0, 0x47},
    {0xa1, 0x5f},
    {0x96, 0x43},
    {0x97, 0x44},
    {0x98, 0x54},
    {0xf0, 0x00},
    {0xf0, 0x01},
    {0x73, 0x00},
    {0x74, 0xe0},
    {0x70, 0x00},
    {0x71, 0x80},
    {0xf0, 0x36},
    {0x37, 0x74},
    {0xf0, 0x3f},
    {0x03, 0xa1},

    {0xf0, 0x36},//cvbs_off
    {0x11, 0x80},
    {0xf0, 0x01},
    {0x79, 0xc1},

    {0xf0, 0x37},
    {0x24, 0x21},
    {0xf0, 0x36},
    {0x41, 0x00},
    {0xea, 0x09},
    {0xeb, 0x03},
    {0xec, 0x19},
    {0xed, 0x38},
    {0xe9, 0x30},
    {0xf0, 0x33},
    {0x33, 0x00},
    {0x34, 0x00},
    {0xb1, 0x00},
    {0xf0, 0x00},
    {0xe0, 0x04},
    {0xf0, 0x01},
    {0x73, 0x00},
    {0x74, 0xe0},
    {0x70, 0x00},
    {0x71, 0x80},
    {0xf0, 0x36},
    {0x32, 0x44},
    {0xf0, 0x36},
    {0x3e, 0xe0},
    {0x70, 0x56},
    {0x7c, 0x43},
    {0x7d, 0x47},
    {0x74, 0x00},
    {0x75, 0x00},
    {0x76, 0x00},
    {0xa0, 0x47},
    {0xa1, 0x5f},
    {0x96, 0x22},
    {0x97, 0x22},
    {0x98, 0x22},
    {0xf0, 0x00},
    {0x72, 0x38},
    {0x7a, 0x80},
    {0x85, 0x18},
    {0x9b, 0x35},
    {0x9e, 0x20},
    {0xd0, 0x66},
    {0xd1, 0x34},
    {0Xd3, 0x44},
    {0xd6, 0x44},
    {0xb0, 0x41},
    {0xb2, 0x48},
    {0xb3, 0xf4},
    {0xb4, 0x0b},
    {0xb5, 0x78},
    {0xba, 0xff},
    {0xbb, 0xc0},
    {0xbc, 0x90},
    {0xbd, 0x3a},
    {0xc1, 0x67},
    {0xf0, 0x01},
    {0x20, 0x11},
    {0x23, 0x90},
    {0x24, 0x15},
    {0x25, 0x87},
    {0xbc, 0x9f},
    {0xbd, 0x3a},
    {0x48, 0xe6},
    {0x49, 0xc0},
    {0x4a, 0xd0},
    {0x4b, 0x48},
};

struct sc03_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 pwdn_pin;
    struct clk *clk;

    struct mpp_video_fmt fmt;

    bool on;
    bool streaming;
};

static struct sc03_dev g_sc03_dev = {0};

static int sc030_write_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 val)
{
    if (rt_i2c_write_reg(i2c, SC030IOT_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return 0;
}

static int sc030_read_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 *val)
{
    if (rt_i2c_read_reg(i2c, SC030IOT_I2C_SLAVE_ID, reg, val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    return 0;
}

static int sc030iot_init(struct sc03_dev *sensor)
{
    int i = 0;
    const struct reg8_info *info = sc030iot_init_regs;

    for (i = 0; i < ARRAY_SIZE(sc030iot_init_regs); i++, info++) {
        if (sc030_write_reg(sensor->i2c, info->reg, info->val))
            return -1;
    }

    return 0;
}

static int sc030iot_probe(struct sc03_dev *sensor)
{
    u8 id_h = 0, id_l = 0;

    if (sc030_read_reg(sensor->i2c, SC030_SENSOR_ID_LOW_REG, &id_l) ||
        sc030_read_reg(sensor->i2c, SC030_SENSOR_ID_HIGH_REG, &id_h))
        return -1;

    if ((id_h << 8 | id_l) != SC030IOT_CHIP_ID) {
        LOG_E("Invalid chip ID: %02x %02x\n", id_h, id_l);
        return -1;
    }
    return sc030iot_init(sensor);
}

static void sc030iot_power_on(struct sc03_dev *sensor)
{
    if (sensor->on)
        return;

    camera_pin_set_high(sensor->pwdn_pin);
    aicos_udelay(2);

    sensor->on = true;
}

static void sc030iot_power_off(struct sc03_dev *sensor)
{
#if 0
    if (!sensor->on)
        return;

    camera_pin_set_low(sensor->pwdn_pin);

    sensor->on = false;
#endif
}

static rt_err_t sc03_init(rt_device_t dev)
{
    int ret = 0;
    struct sc03_dev *sensor = (struct sc03_dev *)dev;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    sensor->fmt.code     = SC030IOT_DFT_CODE;
    sensor->fmt.width    = SC030IOT_DFT_WIDTH;
    sensor->fmt.height   = SC030IOT_DFT_HEIGHT;
    sensor->fmt.bus_type = SC030IOT_DFT_BUS_TYPE;
    sensor->fmt.flags = MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_VSYNC_ACTIVE_LOW |
                        MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;

    sensor->pwdn_pin = camera_pwdn_pin_get();
    if (!sensor->pwdn_pin)
        return -RT_EINVAL;

    sc030iot_power_on(sensor);

    ret = sc030iot_probe(sensor);
    if (ret)
        return -RT_ERROR;

    LOG_I("SC030IOT inited");
    return RT_EOK;
}

static rt_err_t sc03_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t sc03_close(rt_device_t dev)
{
    struct sc03_dev *sensor = (struct sc03_dev *)dev;

    sc030iot_power_off(sensor);
    return RT_EOK;
}

static int sc03_get_fmt(rt_device_t dev, struct mpp_video_fmt *cfg)
{
    struct sc03_dev *sensor = (struct sc03_dev *)dev;

    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int sc03_start(rt_device_t dev)
{
    return 0;
}

static int sc03_stop(rt_device_t dev)
{
    return 0;
}

static rt_err_t sc03_control(rt_device_t dev, int cmd, void *args)
{
    switch (cmd) {
    case CAMERA_CMD_START:
        return sc03_start(dev);
    case CAMERA_CMD_STOP:
        return sc03_stop(dev);
    case CAMERA_CMD_GET_FMT:
        return sc03_get_fmt(dev, (struct mpp_video_fmt *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops sc03_ops =
{
    .init = sc03_init,
    .open = sc03_open,
    .close = sc03_close,
    .control = sc03_control,
};
#endif

int rt_hw_sc03_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_sc03_dev.dev.ops = &sc03_ops;
#else
    g_sc03_dev.dev.init = sc03_init;
    g_sc03_dev.dev.open = sc03_open;
    g_sc03_dev.dev.close = sc03_close;
    g_sc03_dev.dev.control = sc03_control;
#endif
    g_sc03_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_sc03_dev.dev, CAMERA_DEV_NAME, 0);

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_sc03_init);
