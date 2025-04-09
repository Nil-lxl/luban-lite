/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#define LOG_TAG     "gc0308"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "drv_camera.h"
#include "camera_inner.h"

/* Default format configuration of GC03xx */
#define GC03_DFT_WIDTH        VGA_WIDTH
#define GC03_DFT_HEIGHT       VGA_HEIGHT
#define GC03_DFT_BUS_TYPE     MEDIA_BUS_PARALLEL
#define GC03_DFT_CODE         MEDIA_BUS_FMT_YUYV8_2X8

#define GC03_I2C_SLAVE_ID     0x21
#define GC0308_CHIP_ID        0x9B

static const struct reg8_info sensor_init_data[] = {
    {0xfe, 0x00},
    {0xec, 0x20},
    {0x05, 0x00},
    {0x06, 0x00},
    {0x07, 0x00},
    {0x08, 0x00},
    {0x09, 0x01},
    {0x0a, 0xe8},
    {0x0b, 0x02},
    {0x0c, 0x88},
    {0x0d, 0x02},
    {0x0e, 0x02},
    {0x10, 0x26},
    {0x11, 0x0d},
    {0x12, 0x2a},
    {0x13, 0x00},
    {0x14, 0x10}, // bit0: mirror
    {0x15, 0x0a},
    {0x16, 0x05},
    {0x17, 0x01},
    {0x18, 0x44},
    {0x19, 0x44},
    {0x1a, 0x2a},
    {0x1b, 0x00},
    {0x1c, 0x49},
    {0x1d, 0x9a},
    {0x1e, 0x61},
    {0x1f, 0x00}, //pad drv <=24MHz, use 0x00 is ok
    {0x20, 0x7f},
    {0x21, 0xfa},
    {0x22, 0x57},
    {0x24, 0xa2},   //YCbYCr
    {0x25, 0x0f},
    {0x26, 0x03}, // 0x01
    {0x28, 0x00},
    {0x2d, 0x0a},
    {0x2f, 0x01},
    {0x30, 0xf7},
    {0x31, 0x50},
    {0x32, 0x00},
    {0x33, 0x28},
    {0x34, 0x2a},
    {0x35, 0x28},
    {0x39, 0x04},
    {0x3a, 0x20},
    {0x3b, 0x20},
    {0x3c, 0x00},
    {0x3d, 0x00},
    {0x3e, 0x00},
    {0x3f, 0x00},
    {0x50, 0x14}, // 0x14
    {0x52, 0x41},
    {0x53, 0x80},
    {0x54, 0x80},
    {0x55, 0x80},
    {0x56, 0x80},
    {0x8b, 0x20},
    {0x8c, 0x20},
    {0x8d, 0x20},
    {0x8e, 0x14},
    {0x8f, 0x10},
    {0x90, 0x14},
    {0x91, 0x3c},
    {0x92, 0x50},
//{0x8b,0x10},
//{0x8c,0x10},
//{0x8d,0x10},
//{0x8e,0x10},
//{0x8f,0x10},
//{0x90,0x10},
//{0x91,0x3c},
//{0x92,0x50},
    {0x5d, 0x12},
    {0x5e, 0x1a},
    {0x5f, 0x24},
    {0x60, 0x07},
    {0x61, 0x15},
    {0x62, 0x08}, // 0x08
    {0x64, 0x03}, // 0x03
    {0x66, 0xe8},
    {0x67, 0x86},
    {0x68, 0x82},
    {0x69, 0x18},
    {0x6a, 0x0f},
    {0x6b, 0x00},
    {0x6c, 0x5f},
    {0x6d, 0x8f},
    {0x6e, 0x55},
    {0x6f, 0x38},
    {0x70, 0x15},
    {0x71, 0x33},
    {0x72, 0xdc},
    {0x73, 0x00},
    {0x74, 0x02},
    {0x75, 0x3f},
    {0x76, 0x02},
    {0x77, 0x38}, // 0x47
    {0x78, 0x88},
    {0x79, 0x81},
    {0x7a, 0x81},
    {0x7b, 0x22},
    {0x7c, 0xff},
    {0x93, 0x48}, //color matrix default
    {0x94, 0x02},
    {0x95, 0x07},
    {0x96, 0xe0},
    {0x97, 0x40},
    {0x98, 0xf0},
    {0xb1, 0x40},
    {0xb2, 0x40},
    {0xb3, 0x40}, //0x40
    {0xb6, 0xe0},
    {0xbd, 0x38},
    {0xbe, 0x36},
    {0xd0, 0xCB},
    {0xd1, 0x10},
    {0xd2, 0x90},
    {0xd3, 0x48},
    {0xd5, 0xF2},
    {0xd6, 0x16},
    {0xdb, 0x92},
    {0xdc, 0xA5},
    {0xdf, 0x23},
    {0xd9, 0x00},
    {0xda, 0x00},
    {0xe0, 0x09},
    {0xed, 0x04},
    {0xee, 0xa0},
    {0xef, 0x40},
    {0x80, 0x03},

    {0x9F, 0x10},
    {0xA0, 0x20},
    {0xA1, 0x38},
    {0xA2, 0x4e},
    {0xA3, 0x63},
    {0xA4, 0x76},
    {0xA5, 0x87},
    {0xA6, 0xa2},
    {0xA7, 0xb8},
    {0xA8, 0xca},
    {0xA9, 0xd8},
    {0xAA, 0xe3},
    {0xAB, 0xeb},
    {0xAC, 0xf0},
    {0xAD, 0xF8},
    {0xAE, 0xFd},
    {0xAF, 0xFF},

    {0xc0, 0x00},
    {0xc1, 0x10},
    {0xc2, 0x1c},
    {0xc3, 0x30},
    {0xc4, 0x43},
    {0xc5, 0x54},
    {0xc6, 0x65},
    {0xc7, 0x75},
    {0xc8, 0x93},
    {0xc9, 0xB0},
    {0xca, 0xCB},
    {0xcb, 0xE6},
    {0xcc, 0xFF},
    {0xf0, 0x02},
    {0xf1, 0x01},
    {0xf2, 0x02},
    {0xf3, 0x30},
    {0xf7, 0x04},
    {0xf8, 0x02},
    {0xf9, 0x9f},
    {0xfa, 0x78},
    {0xfe, 0x01},
    {0x00, 0xf5},
    {0x02, 0x20},
    {0x04, 0x10},
    {0x05, 0x08},
    {0x06, 0x20},
    {0x08, 0x0a},
    {0x0a, 0xa0},
    {0x0b, 0x60},
    {0x0c, 0x08},
    {0x0e, 0x44},
    {0x0f, 0x32},
    {0x10, 0x41},
    {0x11, 0x37},
    {0x12, 0x22},
    {0x13, 0x19},
    {0x14, 0x44},
    {0x15, 0x44},
    {0x16, 0xc2},
    {0x17, 0xA8},
    {0x18, 0x18},
    {0x19, 0x50},
    {0x1a, 0xd8},
    {0x1b, 0xf5},
    {0x70, 0x40},
    {0x71, 0x58},
    {0x72, 0x30},
    {0x73, 0x48},
    {0x74, 0x20},
    {0x75, 0x60},
    {0x77, 0x20},
    {0x78, 0x32},
    {0x30, 0x03},
    {0x31, 0x40},
    {0x32, 0x10},
    {0x33, 0xe0},
    {0x34, 0xe0},
    {0x35, 0x00},
    {0x36, 0x80},
    {0x37, 0x00},
    {0x38, 0x04},
    {0x39, 0x09},
    {0x3a, 0x12},
    {0x3b, 0x1C},
    {0x3c, 0x28},
    {0x3d, 0x31},
    {0x3e, 0x44},
    {0x3f, 0x57},
    {0x40, 0x6C},
    {0x41, 0x81},
    {0x42, 0x94},
    {0x43, 0xA7},
    {0x44, 0xB8},
    {0x45, 0xD6},
    {0x46, 0xEE},
    {0x47, 0x0d},
    {0x62, 0xf7},
    {0x63, 0x68},
    {0x64, 0xd3},
    {0x65, 0xd3},
    {0x66, 0x60},
    {0xfe, 0x00},

    {0x01, 0x32},   //frame setting
    {0x02, 0x0c},
    {0x0f, 0x01},
    {0xe2, 0x00},
    {0xe3, 0x78},
    {0xe4, 0x00},
    {0xe5, 0xfe},
    {0xe6, 0x01},
    {0xe7, 0xe0},
    {0xe8, 0x01},
    {0xe9, 0xe0},
    {0xea, 0x01},
    {0xeb, 0xe0},
    {0xfe, 0x00},
};

struct gc03_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 rst_pin;
    u32 pwdn_pin;
    struct clk *clk;

    struct mpp_video_fmt fmt;

    bool on;
    bool streaming;
};

static struct gc03_dev g_gc03_dev = {0};

static int gc03_write_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 val)
{
    if (rt_i2c_write_reg(i2c, GC03_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return 0;
}

static int gc03_read_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 *val)
{
    if (rt_i2c_read_reg(i2c, GC03_I2C_SLAVE_ID, reg, val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    return 0;
}

static void gc0308_reset(struct gc03_dev *sensor)
{
    // gc03_write_reg(sensor->i2c, 0xFE, 0xF0);
}

static int gc0308_init(struct gc03_dev *sensor)
{
    int i = 0;
    const struct reg8_info *info = sensor_init_data;

    gc0308_reset(sensor);
    aicos_udelay(1000);

    for (i = 0; i < ARRAY_SIZE(sensor_init_data); i++, info++) {
        if (gc03_write_reg(sensor->i2c, info->reg, info->val))
            return -1;
    }

    return 0;
}

static int gc0308_probe(struct gc03_dev *sensor)
{
    u8 id = 0;

    if (gc03_read_reg(sensor->i2c, 0x0, &id))
        return -1;

    if (id != GC0308_CHIP_ID) {
        LOG_E("Invalid chip ID: %02x\n", id);
        return -1;
    }
    return gc0308_init(sensor);
}

static void gc0308_power_on(struct gc03_dev *sensor)
{
    if (sensor->on)
        return;

    camera_pin_set_low(sensor->pwdn_pin);
    aicos_udelay(1);
    camera_pin_set_high(sensor->rst_pin);

    sensor->on = true;
}

static void gc0308_power_off(struct gc03_dev *sensor)
{
#if 0
    if (!sensor->on)
        return;

    camera_pin_set_high(sensor->pwdn_pin);

    sensor->on = false;
#endif
}

static rt_err_t gc03_init(rt_device_t dev)
{
    int ret = 0;
    struct gc03_dev *sensor = &g_gc03_dev;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    sensor->fmt.code   = GC03_DFT_CODE;
    sensor->fmt.width  = GC03_DFT_WIDTH;
    sensor->fmt.height = GC03_DFT_HEIGHT;
    sensor->fmt.bus_type = GC03_DFT_BUS_TYPE;
    sensor->fmt.flags = MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_VSYNC_ACTIVE_LOW |
                        MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;

    sensor->rst_pin = camera_rst_pin_get();
    sensor->pwdn_pin = camera_pwdn_pin_get();
    if (!sensor->rst_pin || !sensor->pwdn_pin)
        return -RT_EINVAL;

    gc0308_power_on(sensor);

    ret = gc0308_probe(sensor);
    if (ret)
        return -RT_ERROR;

    LOG_I("GC0308 inited");
    return RT_EOK;
}

static rt_err_t gc03_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t gc03_close(rt_device_t dev)
{
    struct gc03_dev *sensor = (struct gc03_dev *)dev;

    gc0308_power_off(sensor);
    return RT_EOK;
}

static int gc03_get_fmt(struct gc03_dev *sensor, struct mpp_video_fmt *cfg)
{
    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int gc03_start(struct gc03_dev *sensor)
{
    return 0;
}

static int gc03_stop(struct gc03_dev *sensor)
{
    return 0;
}

/* Edge Enhancement */
static int gc03_set_ee(struct gc03_dev *sensor, u32 percent)
{
    u8 cur = 0, val = PERCENT_TO_INT(0, 0xFF, percent);

    if (gc03_read_reg(sensor->i2c, 0x77, &cur)) {
        LOG_E("Failed to get current EE\n");
        return -1;
    }

    LOG_I("Set Edge Enhancement 0x%02x -> 0x%02x\n", cur, val);
    if (cur == val)
        return 0;

    return gc03_write_reg(sensor->i2c, 0x77, val);
}

static int gc03_enable_flip(struct gc03_dev *sensor, bool enable,
                            u8 mask, u8 shift, char *name)
{
    u8 cur = 0;

    if (gc03_read_reg(sensor->i2c, 0x14, &cur)) {
        LOG_E("Failed to get current flip\n");
        return -1;
    }

    LOG_I("Set %s flip %d -> %d\n", name, (cur & mask) >> shift, enable);
    if ((cur & mask) >> shift == (u8)enable)
        return 0;

    if (enable)
        return gc03_write_reg(sensor->i2c, 0x14, cur | mask);
    else
        return gc03_write_reg(sensor->i2c, 0x14, cur & ~mask);
}

static int gc03_enable_h_flip(struct gc03_dev *sensor, bool enable)
{
    return gc03_enable_flip(sensor, enable, 1, 0, "H");
}

static int gc03_enable_v_flip(struct gc03_dev *sensor, bool enable)
{
    return gc03_enable_flip(sensor, enable, 2, 1, "V");
}

static rt_err_t gc03_control(rt_device_t dev, int cmd, void *args)
{
    struct gc03_dev *sensor = (struct gc03_dev *)dev;

    switch (cmd) {
    case CAMERA_CMD_START:
        return gc03_start(sensor);
    case CAMERA_CMD_STOP:
        return gc03_stop(sensor);
    case CAMERA_CMD_GET_FMT:
        return gc03_get_fmt(sensor, (struct mpp_video_fmt *)args);
    case CAMERA_CMD_SET_SHARPNESS:
        return gc03_set_ee(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_H_FLIP:
        return gc03_enable_h_flip(sensor, *(bool *)args);
    case CAMERA_CMD_SET_V_FLIP:
        return gc03_enable_v_flip(sensor, *(bool *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops gc03_ops =
{
    .init = gc03_init,
    .open = gc03_open,
    .close = gc03_close,
    .control = gc03_control,
};
#endif

int rt_hw_gc03_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_gc03_dev.dev.ops = &gc03_ops;
#else
    g_gc03_dev.dev.init = gc03_init;
    g_gc03_dev.dev.open = gc03_open;
    g_gc03_dev.dev.close = gc03_close;
    g_gc03_dev.dev.control = gc03_control;
#endif
    g_gc03_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_gc03_dev.dev, CAMERA_DEV_NAME, 0);

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_gc03_init);
