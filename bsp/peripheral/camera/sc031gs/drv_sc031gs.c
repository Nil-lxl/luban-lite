/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#define LOG_TAG     "sc031gs"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "drv_camera.h"
#include "camera_inner.h"

/* Default format configuration of SC031GS */
#define SC031GS_DFT_WIDTH       VGA_WIDTH
#define SC031GS_DFT_HEIGHT      VGA_HEIGHT
#define SC031GS_DFT_BUS_TYPE    MEDIA_BUS_RAW8_MONO
#define SC031GS_DFT_CODE        MEDIA_BUS_FMT_Y8_1X8
// #define SC031GS_TEST_MODE

#define SC031GS_I2C_SLAVE_ID     0x30
#define SC031GS_CHIP_ID          0x0031

#define SC031GS_PID_HIGH_REG                    0x3107
#define SC031GS_PID_LOW_REG                     0x3108
#define SC031GS_MAX_FRAME_WIDTH                 (640)
#define SC031GS_MAX_FRAME_HIGH                  (480)

#define SC031GS_OUTPUT_WINDOW_WIDTH_H_REG       0x3208
#define SC031GS_OUTPUT_WINDOW_WIDTH_L_REG       0x3209
#define SC031GS_OUTPUT_WINDOW_HIGH_H_REG        0x320a
#define SC031GS_OUTPUT_WINDOW_HIGH_L_REG        0x320b
#define SC031GS_OUTPUT_WINDOW_START_Y_H_REG     0x3210
#define SC031GS_OUTPUT_WINDOW_START_Y_L_REG     0x3211
#define SC031GS_OUTPUT_WINDOW_START_X_H_REG     0x3212
#define SC031GS_OUTPUT_WINDOW_START_X_L_REG     0x3213
#define SC031GS_LED_STROBE_ENABLE_REG           0x3361 // When the camera is in exposure, this PAD LEDSTROBE will be high to drive the external LED.

#define REG_NULL            0xFFFF
#define REG_DELAY           0X0000

// 640*480, xclk=24M, fps=60fps
static const struct reg16_info sc031gs_640x480_init_regs[] = {
    {0x0103, 0x01},
    {0x0100, 0x00},
    {0x3018, 0x1f},
    {0x3019, 0xff},
    {0x301c, 0xb4},
    {0x363c, 0x08},
    {0x3630, 0x82},
    {0x3638, 0x0f},
    {0x3639, 0x08},
    {0x335b, 0x80},
    {0x3636, 0x25},
    {0x3640, 0x02},
    {0x3306, 0x38},
    {0x3304, 0x48},
    {0x3389, 0x01},
    {0x3385, 0x31},
    {0x330c, 0x18},
    {0x3315, 0x38},
    {0x3306, 0x28},
    {0x3309, 0x68},
    {0x3387, 0x51},
    {0x3306, 0x48},
    {0x3366, 0x04},
    {0x335f, 0x80},
    {0x363a, 0x00},
    {0x3622, 0x01},
    {0x3633, 0x62},
    {0x36f9, 0x20},
    {0x3637, 0x80},
    {0x363d, 0x04},
    {0x3e06, 0x00},
    {0x363c, 0x48},
    {0x320c, 0x03},
    {0x320e, 0x0e},
    {0x320f, 0xa8},
    {0x3306, 0x38},
    {0x330b, 0xb6},
    {0x36f9, 0x24},
    {0x363b, 0x4a},
    {0x3366, 0x02},
    {0x3316, 0x78},
    {0x3344, 0x74},
    {0x3335, 0x74},
    {0x332f, 0x70},
    {0x332d, 0x6c},
    {0x3329, 0x6c},
    {0x363c, 0x08},
    {0x3630, 0x81},
    {0x3366, 0x06},
    {0x3314, 0x3a},
    {0x3317, 0x28},
    {0x3622, 0x05},
    {0x363d, 0x00},
    {0x3637, 0x86},
    {0x3e01, 0x62},
    {0x3633, 0x52},
    {0x3630, 0x86},
    {0x3306, 0x4c},
    {0x330b, 0xa0},
    {0x3631, 0x48},
    {0x33b1, 0x03},
    {0x33b2, 0x06},
    {0x320c, 0x02},
    {0x320e, 0x02},
    {0x320f, 0x0d},
    {0x3e01, 0x20},
    {0x3e02, 0x20},
    {0x3316, 0x68},
    {0x3344, 0x64},
    {0x3335, 0x64},
    {0x332f, 0x60},
    {0x332d, 0x5c},
    {0x3329, 0x5c},
    {0x3310, 0x10},
    {0x3637, 0x87},
    {0x363e, 0xf8},
    {0x3254, 0x02},
    {0x3255, 0x07},
    {0x3252, 0x02},
    {0x3253, 0xa6},
    {0x3250, 0xf0},
    {0x3251, 0x02},
    {0x330f, 0x50},
    {0x3630, 0x46},
    {0x3621, 0xa2},
    {0x3621, 0xa0},
    {0x4500, 0x59},
    {0x3637, 0x88},
    {0x3908, 0x81},
    {0x3640, 0x00},
    {0x3641, 0x02},
    {0x363c, 0x05},
    {0x363b, 0x4c},
    {0x36e9, 0x40},
    {0x36ea, 0x36},
    {0x36ed, 0x13},
    {0x36f9, 0x04},
    {0x36fa, 0x38},
    {0x330b, 0x80},
    {0x3640, 0x00},
    {0x3641, 0x01},
    {0x3d08, 0x00},
    {0x3306, 0x48},
    {0x3621, 0xa4},
    {0x300f, 0x0f},
    {0x4837, 0x1b},
    {0x4809, 0x01},
    {0x363b, 0x48},
    {0x363c, 0x06},
    {0x36e9, 0x00},
    {0x36ea, 0x3b},
    {0x36eb, 0x1A},
    {0x36ec, 0x0A},
    {0x36ed, 0x33},
    {0x36f9, 0x00},
    {0x36fa, 0x3a},
    {0x36fc, 0x01},
    {0x320c, 0x03},
    {0x320d, 0x6e},
#if 0
    /* 120fps */
    {0x320e, 0x02},
    {0x320f, 0xab},
#else
    /* 60fps */
    {0x320e, 0x05},
    {0x320f, 0x56},
#endif
    {0x330b, 0x80},
    {0x330f, 0x50},
    {0x3637, 0x89},
    {0x3641, 0x01},
    {0x4501, 0xc4},
    {0x5011, 0x01},
    {0x3908, 0x21},
    // AEC
    {0x3e01, 0x18},
    {0x3e02, 0x80},

    {0x3306, 0x38},
    {0x330b, 0xe0},
    {0x330f, 0x20},
    {0x3d08, 0x01},
    {0x3314, 0x65},
    {0x3317, 0x10},
    {0x5011, 0x00},
    {0x3e06, 0x0c},
    {0x3908, 0x91},
    {0x3624, 0x47},
    {0x3220, 0x10},
    {0x3635, 0x18},
    {0x3223, 0x50},
    {0x301f, 0x01},
    {0x3028, 0x82},
    // AGC
    {0x3e09, 0x1f},

    {0x0100, 0x01},
    {REG_DELAY, 0x0a},
    {0x4418, 0x08},
    {0x4419, 0x8e},
    {0x4419, 0x80},
    {0x363d, 0x10},
    {0x3630, 0x48},

    {REG_NULL, 0x00},
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

static int sc031gs_read_reg(struct rt_i2c_bus_device *i2c, u16 reg, u8 *val)
{
    if (rt_i2c_read_reg16(i2c, SC031GS_I2C_SLAVE_ID, reg, val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    return 0;
}

static u16 sc031gs_read_u16(struct rt_i2c_bus_device *i2c, u16 reg_h, u16 reg_l)
{
    u8 val_h = 0, val_l = 0;

    if (sc031gs_read_reg(i2c, reg_h, &val_h) ||
        sc031gs_read_reg(i2c, reg_l, &val_l))
        return 0;

    return val_h << 8 | val_l;
}

static int sc031gs_write_reg(struct rt_i2c_bus_device *i2c, u16 reg, u8 val)
{
    if (rt_i2c_write_reg16(i2c, SC031GS_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return 0;
}

static int sc031gs_write_array(struct rt_i2c_bus_device *i2c,
                               const struct reg16_info *regs, u32 size)
{
    int i, ret = 0;

    for (i = 0; i < size; i++) {
        if (regs[i].reg == REG_NULL)
            break;

        if (regs[i].reg == REG_DELAY) {
            aicos_udelay(regs[i].val * 1000);
            continue;
        }

        ret = sc031gs_write_reg(i2c, regs[i].reg, regs[i].val);
        if (ret < 0)
            return ret;
    }

    return ret;
}

#ifdef SC031GS_TEST_MODE
static void sc031gs_test_mode(struct sc03_dev *sensor)
{
    sc031gs_write_reg(sensor->i2c, 0x4501, 0xcc);
    sc031gs_write_reg(sensor->i2c, 0x3902, 0x0);
    sc031gs_write_reg(sensor->i2c, 0x3e06, 0x3);
}
#endif

static int sc031gs_init(struct sc03_dev *sensor)
{
    if (sc031gs_write_array(sensor->i2c, sc031gs_640x480_init_regs, ARRAY_SIZE(sc031gs_640x480_init_regs)))
        return -1;

#ifdef SC031GS_TEST_MODE
    sc031gs_test_mode(sensor);
#endif

    return 0;
}

static int sc031gs_probe(struct sc03_dev *sensor)
{
    u8 id_h = 0, id_l = 0;

    if (sc031gs_read_reg(sensor->i2c, SC031GS_PID_LOW_REG, &id_l) ||
        sc031gs_read_reg(sensor->i2c, SC031GS_PID_HIGH_REG, &id_h))
        return -1;

    if ((id_h << 8 | id_l) != SC031GS_CHIP_ID) {
        LOG_E("Invalid chip ID: %02x %02x\n", id_h, id_l);
        return -1;
    }
    return sc031gs_init(sensor);
}

static void sc031gs_power_on(struct sc03_dev *sensor)
{
    if (sensor->on)
        return;

    camera_pin_set_high(sensor->pwdn_pin);
    aicos_udelay(2);

    sensor->on = true;
}

static void sc031gs_power_off(struct sc03_dev *sensor)
{
    if (!sensor->on)
        return;

    camera_pin_set_low(sensor->pwdn_pin);

    sensor->on = false;
}

static rt_err_t sc03_init(rt_device_t dev)
{
    struct sc03_dev *sensor = (struct sc03_dev *)dev;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    sensor->fmt.code     = SC031GS_DFT_CODE;
    sensor->fmt.width    = SC031GS_DFT_WIDTH;
    sensor->fmt.height   = SC031GS_DFT_HEIGHT;
    sensor->fmt.bus_type = SC031GS_DFT_BUS_TYPE;
    sensor->fmt.flags = MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_VSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;

    sensor->pwdn_pin = camera_pwdn_pin_get();
    if (!sensor->pwdn_pin)
        return -RT_EINVAL;

    sc031gs_power_on(sensor);

    if (sc031gs_probe(sensor))
        return -RT_ERROR;

    LOG_I("SC031GS inited");
    return RT_EOK;
}

static rt_err_t sc03_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t sc03_close(rt_device_t dev)
{
    struct sc03_dev *sensor = (struct sc03_dev *)dev;

    sc031gs_power_off(sensor);
    return RT_EOK;
}

static int sc03_get_fmt(struct sc03_dev *sensor, struct mpp_video_fmt *cfg)
{
    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int sc03_start(struct sc03_dev *sensor)
{
    return 0;
}

static int sc03_stop(struct sc03_dev *sensor)
{
    return 0;
}

static int sc03_set_fps(struct sc03_dev *sensor, u32 fps)
{
    u16 cur = 0, base = 0x2ab, val = 0;
    u32 i, avail_fps[] = {120, 60, 30, 15};

    if (fps < 15)
        fps = 15;

    for (i = 0; i < ARRAY_SIZE(avail_fps); i++) {
        if (fps >= avail_fps[i]) {
            val = base << i;
            break;
        }
    }

    cur = sc031gs_read_u16(sensor->i2c, 0x320e, 0x320f);
    if (!cur) {
        LOG_E("Failed to read FPS\n");
        return -1;
    }

    LOG_I("Set FPS %d[0x%03x] -> %d[0x%03x]\n",
          120 / (cur / 0x2ab), cur, 120 / (val / 0x2ab), val);

    if (cur == val)
        return 0;

    if (sc031gs_write_reg(sensor->i2c, 0x320e, val >> 8) ||
        sc031gs_write_reg(sensor->i2c, 0x320f, val & 0xFF))
        return -1;
    else
        return 0;
}

/* Adjust AEC preferentially, and then adjust AGC */
static int sc03_set_agc(struct sc03_dev *sensor, u32 percent)
{
    u16 cur = 0, val = 0, gain = PERCENT_TO_INT(0x10, 0xFF, percent);
    u8 val_h = 0, val_l = 0;

    if (sc031gs_read_reg(sensor->i2c, 0x3e08, &val_h)
        || sc031gs_read_reg(sensor->i2c, 0x3e09, &val_l)) {
        LOG_E("Failed to get current AGC\n");
        return -1;
    }
    cur = ((val_h & 0xC) << 6) | val_l; /* {16'h3e08[4:2],16h3e09} */

    sc031gs_write_reg(sensor->i2c, 0x3303, 0x0b);

    if (gain < 0x20) { /* 1x ~ 2x */
        val_h = 0x03;
        val_l = 0x10 | ((gain - 0x10) & 0xF);
    } else if (gain < 0x40) { /* 2x ~ 4x */
        val_h = 0x7;
        val_l = 0x10 | (((gain - 0x20) >> 1) & 0xF);
    } else if (gain < 0x80) { /* 4x ~ 8x */
        val_h = 0xf;
        val_l = 0x10 | (((gain - 0x40) >> 2) & 0xF);
    } else { /* 8x ~ 16x */
        val_h = 0x1f;
        val_l = 0x10 | (((gain - 0x80) >> 3) & 0xF);
    }

    val = (val_h << 8) | val_l;
    LOG_I("Set AGC %d[0x%03x] -> %d[0x%03x]\n", cur, cur, val, val);

    if (cur == val)
        return 0;

    if (sc031gs_write_reg(sensor->i2c, 0x3e08, val_h) ||
        sc031gs_write_reg(sensor->i2c, 0x3e09, val_l))
        return -1;
    else
        return 0;
}

/* SC031GS doesn't support AEC. Adjust the exposure time to control brightness. */
static int sc03_set_aec(struct sc03_dev *sensor, u32 percent)
{
    u16 frame_len = 0, cur = 0, val = 0;
    u8 val_h = 0, val_l = 0;

    frame_len = sc031gs_read_u16(sensor->i2c, 0x320e, 0x320f);
    if (!frame_len) {
        LOG_E("Failed to get frame length\n");
        return -1;
    }
    if (frame_len - 6 > 0xFFF) {
        LOG_W("Frame length %d is too large\n", frame_len);
        frame_len = 0xFFF + 6;
    }

    if (sc031gs_read_reg(sensor->i2c, 0x3e01, &val_h)
        || sc031gs_read_reg(sensor->i2c, 0x3e02, &val_l)) {
        LOG_E("Failed to get current AEC\n");
        return -1;
    }
    cur = (val_h << 4) | (val_l >> 4); // {16'h3e01,16'h3e02[7:4]}

    val = PERCENT_TO_INT(1, frame_len - 6, percent);
    LOG_I("Set AEC %d[0x%03x] -> %d[0x%03x] (Frame length %d[0x%03x])\n",
          cur, cur, val, val, frame_len, frame_len);

    if (cur == val)
        return 0;

    if (sc031gs_write_reg(sensor->i2c, 0x3e01, (val >> 4) & 0xFF) ||
        sc031gs_write_reg(sensor->i2c, 0x3e02, (val & 0xF) << 4))
        return -1;
    else
        return 0;
}

static int sc03_enable_h_flip(struct sc03_dev *sensor, bool enable)
{
    u8 cur = 0, mask = 0x6;

    if (sc031gs_read_reg(sensor->i2c, 0x3221, &cur)) {
        LOG_E("Failed to get the H flip status\n");
        return -1;
    }

    LOG_I("Set H flip 0x%x -> 0x%x\n", (cur & mask) >> 1, enable ? 3 : 0);
    if (enable)
        return sc031gs_write_reg(sensor->i2c, 0x3221, cur | mask);
    else
        return sc031gs_write_reg(sensor->i2c, 0x3221, cur & ~mask);
}

static int sc03_enable_v_flip(struct sc03_dev *sensor, bool enable)
{
    u8 cur = 0, mask = 0x60;

    if (sc031gs_read_reg(sensor->i2c, 0x3221, &cur)) {
        LOG_E("Failed to get the V flip status\n");
        return -1;
    }

    LOG_I("Set V flip 0x%x -> 0x%x\n", (cur & mask) >> 5, enable ? 3 : 0);
    if (enable)
        return sc031gs_write_reg(sensor->i2c, 0x3221, cur | mask);
    else
        return sc031gs_write_reg(sensor->i2c, 0x3221, cur & ~mask);
}

static rt_err_t sc03_control(rt_device_t dev, int cmd, void *args)
{
    struct sc03_dev *sensor = (struct sc03_dev *)dev;

    switch (cmd) {
    case CAMERA_CMD_START:
        return sc03_start(sensor);
    case CAMERA_CMD_STOP:
        return sc03_stop(sensor);
    case CAMERA_CMD_GET_FMT:
        return sc03_get_fmt(sensor, (struct mpp_video_fmt *)args);
    case CAMERA_CMD_SET_FPS:
        return sc03_set_fps(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_AUTOGAIN:
        return sc03_set_agc(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_AEC_VAL:
        return sc03_set_aec(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_H_FLIP:
        return sc03_enable_h_flip(sensor, *(bool *)args);
    case CAMERA_CMD_SET_V_FLIP:
        return sc03_enable_v_flip(sensor, *(bool *)args);
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
