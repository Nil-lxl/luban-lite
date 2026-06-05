/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: geo <guojun.dong@artinchip.com>
 */

#define LOG_TAG     "BF3A03"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "drv_camera.h"
#include "camera_inner.h"

/* Default format configuration of BF3Axx */
#define BF3A_DFT_WIDTH        VGA_WIDTH
#define BE3A_DFT_HEIGHT       VGA_HEIGHT
#define BF3A_DFT_BUS_TYPE     MEDIA_BUS_PARALLEL
#define BF3A_DFT_CODE         MEDIA_BUS_FMT_YUYV8_2X8

#define BF3A_I2C_SLAVE_ID     0x6E
#define BF3A_CHIP_ID          0x22

static const struct reg8_info sensor_init_data[] =
{
    // Initail Sequence Write In.
    {0x09,0x55},
    {0x15,0x00},
    {0x1e,0x40},//HV mirror

    //Analog signals
    {0x06,0x78},//0x68
    {0x21,0x00},
    {0x3e,0x37},
    {0x29,0x2b},//0x29
    {0x27,0x98},

    //Clock
    {0x2f,0x4e},
    {0x11,0x10},
    {0x1b,0x09},

    {0x12,0x00},
    {0x3a,0x00},

    //Manual
    {0x13,0x08},

    {0x8c,0x02},
    {0x8d,0x4c},
    {0x87,0x16},//GLB GAIN0

    //Auto
    {0x13, 0x07},//set_BF3A03_awb(uint32 mode)
    {0x01, 0x19},
    {0x02, 0x15},
    {0x6a, 0x81},
    {0xff, 0xff},


    //Denoise
    {0x70,0x0f},
    {0x3b,0x00},
    {0x71,0x0c},
    {0x73,0x27},//Denoise
    {0x75,0x88},//Outdoor denoise
    {0x76,0xd8},
    {0x77,0x0a},//Low light denoise
    {0x78,0xff},
    {0x79,0x14},
    {0x7a,0x22},//{0x7a,0x24},
    {0x9e,0x04},//0xc4
    {0x7d,0x2a},

    //Gamma default
    {0x39,0xa0},//Gamma offset//c0//b0
    {0x3f,0xa0},//b0
    {0x90,0x20},
    {0x5f,0x03},//Dark_sel gamma// 0x01


    {0x40,0x22},
    {0x41,0x23},
    {0x42,0x28},
    {0x43,0x25},
    {0x44,0x1d},
    {0x45,0x17},
    {0x46,0x13},
    {0x47,0x12},
    {0x48,0x10},
    {0x49,0x0d},
    {0x4b,0x0b},
    {0x4c,0x0b},
    {0x4e,0x09},
    {0x4f,0x07},
    {0x50,0x06},

    //AE
    {0x24,0x50},
    {0x97,0x40},
    {0x25,0x88},
    {0x81,0x00},
    {0x82,0x18},
    {0x83,0x30},
    {0x84,0x20},
    {0x85,0x38},
    {0x86,0x55},
    {0x94,0x82},
    {0x80,0x92},
    {0x98,0x88},
    {0x8e,0x2c},
    {0x8f,0x86},

    //Banding
    {0x2b,0x20},
    {0x8a,0x93},//50HZ
    {0x8b,0x7a},//60HZ
    {0x92,0x6D},

    //Color
    {0x5a,0xec},//Outdoor color
    {0x51,0x90},
    {0x52,0x10},
    {0x53,0x8d},
    {0x54,0x88},
    {0x57,0x82},
    {0x58,0x8d},
    {0x5a,0x7c},//A light color
    {0x51,0x80},
    {0x52,0x04},
    {0x53,0x8d},
    {0x54,0x88},
    {0x57,0x82},
    {0x58,0x8d},

    //Color defult
    {0x5a,0x6c},//Indoor color
    {0x51,0x93},
    {0x52,0x04},
    {0x53,0x8a},
    {0x54,0x88},
    {0x57,0x02},
    {0x58,0x8d},

    //Saturation
    {0xb0,0xa0},
    {0xb1,0x26},
    {0xb2,0x1c},
    {0xb4,0xfd},
    {0xb0,0x30},
    {0xb1,0xd8},
    {0xb2,0xb0},
    {0xb4,0xf1},

    //Contrast
    {0x3c,0x40},//K1
    {0x56,0x48},//K2 0xb4[4] new or old  set_contrast
    {0x4d,0x40},//K3
    {0x59,0x40},//K4

    //G gain
    {0x35,0x56},//shading R
    {0x65,0x36},//shading G
    {0x66,0x44},//shading B

    {0x89,0x5d},//set_work_mode
    {0x86,0x77},//

    //AWB
    {0x6a,0x91},
    {0x23,0x44},
    {0xa2,0x04},
    {0xa3,0x26},
    {0xa4,0x04},
    {0xa5,0x26},
    {0xa7,0x1a},
    {0xa8,0x10},
    {0xa9,0x1f},
    {0xaa,0x16},
    {0xab,0x16},
    {0xac,0x30},
    {0xad,0xf0},
    {0xae,0x57},
    {0xc5,0xaa},
    {0xc7,0x38},
    {0xc8,0x0d},
    {0xc9,0x16},
    {0xd3,0x09},
    {0xd4,0x15},
    {0xd0,0x00},
    {0xd1,0x01},
    {0xd2,0x18},//58

    {0x20,0x00},
    {0x16,0x25},
};

struct bf3a_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 rst_pin;
    u32 pwdn_pin;
    struct clk *clk;

    struct mpp_video_fmt fmt;

    bool on;
    bool streaming;
};

static struct bf3a_dev g_bf3a_dev = {0};

static int bf3a_write_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 val)
{
    if (rt_i2c_write_reg(i2c, BF3A_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return 0;
}

static int bf3a_read_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 *val)
{
    if (rt_i2c_read_reg(i2c, BF3A_I2C_SLAVE_ID, reg, val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    return 0;
}

static void bf3a03_reset(struct bf3a_dev *sensor)
{
    return;
}

static int bf3a03_init(struct bf3a_dev *sensor)
{
    int i = 0;
    const struct reg8_info *info = sensor_init_data;

    bf3a03_reset(sensor);
    aicos_udelay(1000);

    for (i = 0; i < ARRAY_SIZE(sensor_init_data); i++, info++) {
        if (bf3a_write_reg(sensor->i2c, info->reg, info->val))
            return -1;
    }

    return 0;
}

static int bf3a03_probe(struct bf3a_dev *sensor)
{
    u8 id = 0;

    if (bf3a_read_reg(sensor->i2c, 0x0, &id))
        return -1;

    if (id != BF3A_CHIP_ID) {
        LOG_E("Invalid chip ID: %02x\n", id);
        return -1;
    }
    return bf3a03_init(sensor);
}

static bool bf3a03_is_open(struct bf3a_dev *sensor)
{
    return sensor->on;
}

static void bf3a03_power_on(struct bf3a_dev *sensor)
{
    if (sensor->on)
        return;

    camera_pin_set_low(sensor->pwdn_pin);
    aicos_udelay(1);
    camera_pin_set_high(sensor->rst_pin);

    LOG_I("Power on");
    sensor->on = true;
}

static void bf3a03_power_off(struct bf3a_dev *sensor)
{
    if (!sensor->on)
        return;

    camera_pin_set_high(sensor->pwdn_pin);

    LOG_I("Power off");
    sensor->on = false;
}

static rt_err_t bf3a_init(rt_device_t dev)
{
    struct bf3a_dev *sensor = &g_bf3a_dev;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    sensor->fmt.code   = BF3A_DFT_CODE;
    sensor->fmt.width  = BF3A_DFT_WIDTH;
    sensor->fmt.height = BE3A_DFT_HEIGHT;
    sensor->fmt.bus_type = BF3A_DFT_BUS_TYPE;
    sensor->fmt.flags = MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_VSYNC_ACTIVE_LOW |
                        MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;

    sensor->rst_pin = camera_rst_pin_get();
    sensor->pwdn_pin = camera_pwdn_pin_get();
    if (!sensor->rst_pin || !sensor->pwdn_pin)
        return -RT_EINVAL;

    return RT_EOK;
}

static rt_err_t bf3a_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct bf3a_dev *sensor = (struct bf3a_dev *)dev;

    if (bf3a03_is_open(sensor))
        return RT_EOK;

    bf3a03_power_on(sensor);

    if (bf3a03_probe(sensor)) {
        bf3a03_power_off(sensor);
        return -RT_ERROR;
    }

    LOG_I("BF3A03 inited");
    return RT_EOK;
}

static rt_err_t bf3a_close(rt_device_t dev)
{
    struct bf3a_dev *sensor = (struct bf3a_dev *)dev;

    if (!bf3a03_is_open(sensor))
        return -RT_ERROR;

    bf3a03_power_off(sensor);
    return RT_EOK;
}

static int bf3a_get_fmt(struct bf3a_dev *sensor, struct mpp_video_fmt *cfg)
{
    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int bf3a_start(struct bf3a_dev *sensor)
{
    return 0;
}

static int bf3a_stop(struct bf3a_dev *sensor)
{
    return 0;
}

static int bf3a_pause(rt_device_t dev)
{
    return bf3a_close(dev);
}

static int bf3a_resume(rt_device_t dev)
{
    return bf3a_open(dev, 0);
}

static int bf3a_set_ee(struct bf3a_dev *sensor, u32 percent)
{
    u8 cur = 0, val = PERCENT_TO_INT(0, 0xFF, percent);

    if (bf3a_read_reg(sensor->i2c, 0x77, &cur)) {
        LOG_E("Failed to get current EE\n");
        return -1;
    }

    LOG_I("Set Edge Enhancement 0x%02x -> 0x%02x\n", cur, val);
    if (cur == val)
        return 0;

    return bf3a_write_reg(sensor->i2c, 0x77, val);
}

static int bf3a_enable_flip(struct bf3a_dev *sensor, bool enable,
                            u8 mask, u8 shift, char *name)
{
    u8 cur = 0;

    if (bf3a_read_reg(sensor->i2c, 0x14, &cur)) {
        LOG_E("Failed to get current flip\n");
        return -1;
    }

    LOG_I("Set %s flip %d -> %d\n", name, (cur & mask) >> shift, enable);
    if ((cur & mask) >> shift == (u8)enable)
        return 0;

    if (enable)
        return bf3a_write_reg(sensor->i2c, 0x14, cur | mask);
    else
        return bf3a_write_reg(sensor->i2c, 0x14, cur & ~mask);
}

static int bf3a_enable_h_flip(struct bf3a_dev *sensor, bool enable)
{
    return bf3a_enable_flip(sensor, enable, 1, 0, "H");
}

static int bf3a_enable_v_flip(struct bf3a_dev *sensor, bool enable)
{
    return bf3a_enable_flip(sensor, enable, 2, 1, "V");
}

static rt_err_t bf3a_control(rt_device_t dev, int cmd, void *args)
{
    struct bf3a_dev *sensor = (struct bf3a_dev *)dev;

    switch (cmd) {
    case CAMERA_CMD_START:
        return bf3a_start(sensor);
    case CAMERA_CMD_STOP:
        return bf3a_stop(sensor);
    case CAMERA_CMD_PAUSE:
        return bf3a_pause(dev);
    case CAMERA_CMD_RESUME:
        return bf3a_resume(dev);
    case CAMERA_CMD_GET_FMT:
        return bf3a_get_fmt(sensor, (struct mpp_video_fmt *)args);
    case CAMERA_CMD_SET_SHARPNESS:
        return bf3a_set_ee(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_H_FLIP:
        return bf3a_enable_h_flip(sensor, *(bool *)args);
    case CAMERA_CMD_SET_V_FLIP:
        return bf3a_enable_v_flip(sensor, *(bool *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops bf3a_ops =
{
    .init = bf3a_init,
    .open = bf3a_open,
    .close = bf3a_close,
    .control = bf3a_control,
};
#endif

#ifdef RT_USING_PM
static int bf3a_pm_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    return RT_EOK;
}

static void bf3a_pm_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct bf3a_dev *sensor = (struct bf3a_dev *)device;

    switch (mode) {
    case PM_SLEEP_MODE_LIGHT:
        break;
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        sensor->rst_pin = camera_rst_pin_get();
        sensor->pwdn_pin = camera_pwdn_pin_get();
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops bf3a_pm_ops =
{
    SET_DEVICE_PM_OPS(bf3a_pm_suspend, bf3a_pm_resume)
    NULL,
};
#endif /* RT_USING_PM */

int rt_hw_bf3a_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_bf3a_dev.dev.ops = &bf3a_ops;
#else
    g_bf3a_dev.dev.init = bf3a_init;
    g_bf3a_dev.dev.open = bf3a_open;
    g_bf3a_dev.dev.close = bf3a_close;
    g_bf3a_dev.dev.control = bf3a_control;
#endif
    g_bf3a_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_bf3a_dev.dev, CAMERA_DEV_NAME, 0);

#ifdef RT_USING_PM
    rt_pm_device_register(&g_bf3a_dev.dev, &bf3a_pm_ops);
#endif

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_bf3a_init);
