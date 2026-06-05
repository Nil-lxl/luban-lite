/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#define LOG_TAG     "nvp6158"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "drv_camera.h"
#include "camera_inner.h"

#include "nvp6158_regs.h"

/* Default format configuration of NVP6158 */
#define NVP6158_DFT_WIDTH           PAL_WIDTH
#define NVP6158_DFT_HEIGHT          PAL_HEIGHT
#define NVP6158_DFT_BUS_TYPE        MEDIA_BUS_BT656
#define NVP6158_DFT_FPS             NVP6158_FPS_25
#define NVP6158_DFT_CODE            MEDIA_BUS_FMT_UYVY8_2X8
#define NVP6158_DFT_MUX             NVP6158_MUX_1

#define NVP6158_I2C_SLAVE_ID        0x30
#define NVP6158_CHIP_ID             0xA000

/* Register of BANK0 */
#define NVP6158_BRIGHTNESS_REG(ch)  (0x0C + (ch))
#define NVP6158_CONTRAST_REG(ch)    (0x10 + (ch))
#define NVP6158_SHARPNESS_REG(ch)   (0x14 + (ch))
#define NVP6158_SATURATION_REG(ch)  (0x3C + (ch))
#define NVP6158_HUE_REG(ch)         (0x40 + (ch))
#define NVP6158_U_GAIN_REG(ch)      (0x44 + (ch))
#define NVP6158_V_GAIN_REG(ch)      (0x48 + (ch))
/* Register of BANK1 */
#define NVP6158_VDO_EN_REG          (0xCA)

/* NVP6158 resolution types */
enum nvp6158_res {
    NVP6158_RES_1080P = 0,
    NVP6158_RES_720P,
    NVP6158_RES_720H,
    NVP6158_RES_MAX
};

#define NVP_MBUS(n)     ((n) == MEDIA_BUS_BT1120) ? "BT1120" : "BT656"

enum nvp6158_fps {
    NVP6158_FPS_25 = 0,
    NVP6158_FPS_30,
    NVP6158_FPS_MAX
};
#define NVP_FPS(n)     ((n) == NVP6158_FPS_30) ? 30 : 25

/* NVP6158 mux types */
enum nvp6158_mux {
    NVP6158_MUX_1 = 0,
    NVP6158_MUX_2,
    NVP6158_MUX_4,
    NVP6158_MUX_MAX
};
#define NVP_MUX(n)     ((n) == NVP6158_MUX_1) ? 1 : ((n) * 2)

struct nvp_mode {
    enum nvp6158_res res;
    enum mpp_mbus_type mbus;
    enum nvp6158_fps fps;
    enum nvp6158_mux mux;
    bool is_interlaced;
    bool is_pal;
    u32 width;
    u32 height;
    struct reg8_info *regs;
    int reg_num;
};

#define NVP6158_DEFAULT_MODE (struct nvp_mode) {\
    .mbus = NVP6158_DFT_BUS_TYPE,\
    .fps = NVP6158_DFT_FPS,\
    .mux = NVP6158_DFT_MUX,\
    .width = NVP6158_DFT_WIDTH,\
    .height = NVP6158_DFT_HEIGHT,\
}

struct nvp_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 resetb_pin;

    struct mpp_video_fmt fmt;
    struct nvp_mode *cur_mode;
    enum nvp6158_fps fps;
    enum nvp6158_mux mux;
    u32 cur_chan;

    bool on;
    bool streaming;
};

static struct nvp_dev g_nvp6158_dev = {0};

static struct nvp_mode g_nvp_modes[] = {
    // 1080p BT656 formats
#ifdef CONFIG_NVP6158_1080P25_BT656_2MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT656, NVP6158_FPS_25, NVP6158_MUX_2,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_1080p25_bt656_2mux_regs, ARRAY_SIZE(nvp6158_1080p25_bt656_2mux_regs)},
#endif
#ifdef CONFIG_NVP6158_1080P25_BT656_1MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT656, NVP6158_FPS_25, NVP6158_MUX_1,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_1080p25_bt656_1mux_regs, ARRAY_SIZE(nvp6158_1080p25_bt656_1mux_regs)},
#endif

#ifdef CONFIG_NVP6158_1080P30_BT656_2MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT656, NVP6158_FPS_30, NVP6158_MUX_2,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_1080p30_bt656_2mux_regs, ARRAY_SIZE(nvp6158_1080p30_bt656_2mux_regs)},
#endif
#ifdef CONFIG_NVP6158_1080P30_BT656_1MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT656, NVP6158_FPS_30, NVP6158_MUX_1,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_1080p30_bt656_1mux_regs, ARRAY_SIZE(nvp6158_1080p30_bt656_1mux_regs)},
#endif

    // TVI 1080p BT656 formats
#ifdef CONFIG_NVP6158_TVI1080P25_BT656_2MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT656, NVP6158_FPS_25, NVP6158_MUX_2,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_tvi1080p25_bt656_2mux_regs, ARRAY_SIZE(nvp6158_tvi1080p25_bt656_2mux_regs)},
#endif
#ifdef CONFIG_NVP6158_TVI1080P25_BT656_1MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT656, NVP6158_FPS_25, NVP6158_MUX_1,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_tvi1080p25_bt656_1mux_regs, ARRAY_SIZE(nvp6158_tvi1080p25_bt656_1mux_regs)},
#endif

#ifdef CONFIG_NVP6158_TVI1080P30_BT656_2MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT656, NVP6158_FPS_30, NVP6158_MUX_2,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_tvi1080p30_bt656_2mux_regs, ARRAY_SIZE(nvp6158_tvi1080p30_bt656_2mux_regs)},
#endif
#ifdef CONFIG_NVP6158_TVI1080P30_BT656_1MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT656, NVP6158_FPS_30, NVP6158_MUX_1,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_tvi1080p30_bt656_1mux_regs, ARRAY_SIZE(nvp6158_tvi1080p30_bt656_1mux_regs)},
#endif

    // 1080p BT1120 formats
#ifdef CONFIG_NVP6158_1080P25_BT1120_4MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT1120, NVP6158_FPS_25, NVP6158_MUX_4,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_1080p25_bt1120_4mux_regs, ARRAY_SIZE(nvp6158_1080p25_bt1120_4mux_regs)},
#endif
#ifdef CONFIG_NVP6158_1080P25_BT1120_2MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT1120, NVP6158_FPS_25, NVP6158_MUX_2,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_1080p25_bt1120_2mux_regs, ARRAY_SIZE(nvp6158_1080p25_bt1120_2mux_regs)},
#endif
#ifdef CONFIG_NVP6158_1080P25_BT1120_1MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT1120, NVP6158_FPS_25, NVP6158_MUX_1,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_1080p25_bt1120_1mux_regs, ARRAY_SIZE(nvp6158_1080p25_bt1120_1mux_regs)},
#endif

#ifdef CONFIG_NVP6158_1080P30_BT1120_4MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT1120, NVP6158_FPS_30, NVP6158_MUX_4,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_1080p30_bt1120_4mux_regs, ARRAY_SIZE(nvp6158_1080p30_bt1120_4mux_regs)},
#endif
#ifdef CONFIG_NVP6158_1080P30_BT1120_2MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT1120, NVP6158_FPS_30, NVP6158_MUX_2,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_1080p30_bt1120_2mux_regs, ARRAY_SIZE(nvp6158_1080p30_bt1120_2mux_regs)},
#endif
#ifdef CONFIG_NVP6158_1080P30_BT1120_1MUX
    {NVP6158_RES_1080P, MEDIA_BUS_BT1120, NVP6158_FPS_30, NVP6158_MUX_1,
     false, false, HD_1080_WIDTH, HD_1080_HEIGHT,
     nvp6158_1080p30_bt1120_1mux_regs, ARRAY_SIZE(nvp6158_1080p30_bt1120_1mux_regs)},
#endif

    // 720p BT1120 formats
#ifdef CONFIG_NVP6158_720P25_BT1120_4MUX
    {NVP6158_RES_720P, MEDIA_BUS_BT1120, NVP6158_FPS_25, NVP6158_MUX_4,
     false, false, HD_720_WIDTH, HD_720_HEIGHT,
     nvp6158_720p25_bt1120_4mux_regs, ARRAY_SIZE(nvp6158_720p25_bt1120_4mux_regs)},
#endif
#ifdef CONFIG_NVP6158_720P25_BT1120_2MUX
    {NVP6158_RES_720P, MEDIA_BUS_BT1120, NVP6158_FPS_25, NVP6158_MUX_2,
     false, false, HD_720_WIDTH, HD_720_HEIGHT,
     nvp6158_720p25_bt1120_2mux_regs, ARRAY_SIZE(nvp6158_720p25_bt1120_2mux_regs)},
#endif
#ifdef CONFIG_NVP6158_720P25_BT1120_1MUX
    {NVP6158_RES_720P, MEDIA_BUS_BT1120, NVP6158_FPS_25, NVP6158_MUX_1,
     false, false, HD_720_WIDTH, HD_720_HEIGHT,
     nvp6158_720p25_bt1120_1mux_regs, ARRAY_SIZE(nvp6158_720p25_bt1120_1mux_regs)},
#endif

#ifdef CONFIG_NVP6158_720P30_BT1120_4MUX
    {NVP6158_RES_720P, MEDIA_BUS_BT1120, NVP6158_FPS_30, NVP6158_MUX_4,
     false, false, HD_720_WIDTH, HD_720_HEIGHT,
     nvp6158_720p30_bt1120_4mux_regs, ARRAY_SIZE(nvp6158_720p30_bt1120_4mux_regs)},
#endif
#ifdef CONFIG_NVP6158_720P30_BT1120_2MUX
    {NVP6158_RES_720P, MEDIA_BUS_BT1120, NVP6158_FPS_30, NVP6158_MUX_2,
     false, false, HD_720_WIDTH, HD_720_HEIGHT,
     nvp6158_720p30_bt1120_2mux_regs, ARRAY_SIZE(nvp6158_720p30_bt1120_2mux_regs)},
#endif
#ifdef CONFIG_NVP6158_720P30_BT1120_1MUX
    {NVP6158_RES_720P, MEDIA_BUS_BT1120, NVP6158_FPS_30, NVP6158_MUX_1,
     false, false, HD_720_WIDTH, HD_720_HEIGHT,
     nvp6158_720p30_bt1120_1mux_regs, ARRAY_SIZE(nvp6158_720p30_bt1120_1mux_regs)},
#endif

    // 720H BT656 formats
#ifdef CONFIG_NVP6158_720H_PAL_BT656_4MUX
    {NVP6158_RES_720H, MEDIA_BUS_BT656, NVP6158_FPS_25, NVP6158_MUX_4,
     true, true, PAL_WIDTH, PAL_HEIGHT,
     nvp6158_720h_pal_bt656_4mux_regs, ARRAY_SIZE(nvp6158_720h_pal_bt656_4mux_regs)},
#endif
#ifdef CONFIG_NVP6158_720H_PAL_BT656_2MUX
    {NVP6158_RES_720H, MEDIA_BUS_BT656, NVP6158_FPS_25, NVP6158_MUX_2,
     true, true, PAL_WIDTH, PAL_HEIGHT,
     nvp6158_720h_pal_bt656_2mux_regs, ARRAY_SIZE(nvp6158_720h_pal_bt656_2mux_regs)},
#endif
#ifdef CONFIG_NVP6158_720H_PAL_BT656_1MUX
    {NVP6158_RES_720H, MEDIA_BUS_BT656, NVP6158_FPS_25, NVP6158_MUX_1,
     true, true, PAL_WIDTH, PAL_HEIGHT,
     nvp6158_720h_pal_bt656_1mux_regs, ARRAY_SIZE(nvp6158_720h_pal_bt656_1mux_regs)},
#endif

#ifdef CONFIG_NVP6158_720H_NTSC_BT656_4MUX
    {NVP6158_RES_720H, MEDIA_BUS_BT656, NVP6158_FPS_30, NVP6158_MUX_4,
     true, false, NTSC_WIDTH, NTSC_HEIGHT,
     nvp6158_720h_ntsc_bt656_4mux_regs, ARRAY_SIZE(nvp6158_720h_ntsc_bt656_4mux_regs)},
#endif
#ifdef CONFIG_NVP6158_720H_NTSC_BT656_2MUX
    {NVP6158_RES_720H, MEDIA_BUS_BT656, NVP6158_FPS_30, NVP6158_MUX_2,
     true, false, NTSC_WIDTH, NTSC_HEIGHT,
     nvp6158_720h_ntsc_bt656_2mux_regs, ARRAY_SIZE(nvp6158_720h_ntsc_bt656_2mux_regs)},
#endif
#ifdef CONFIG_NVP6158_720H_NTSC_BT656_1MUX
    {NVP6158_RES_720H, MEDIA_BUS_BT656, NVP6158_FPS_30, NVP6158_MUX_1,
     true, false, NTSC_WIDTH, NTSC_HEIGHT,
     nvp6158_720h_ntsc_bt656_1mux_regs, ARRAY_SIZE(nvp6158_720h_ntsc_bt656_1mux_regs)},
#endif
};

static int nvp6158_write_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 val)
{
    if (rt_i2c_write_reg(i2c, NVP6158_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return 0;
}

static int nvp6158_read_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 *val)
{
    if (rt_i2c_read_reg(i2c, NVP6158_I2C_SLAVE_ID, reg, val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    return 0;
}

static void nvp6158_apply_cfg(struct nvp_dev *sensor, struct nvp_mode *mode)
{
    struct reg8_info *info = mode->regs;
    int i;

    for (i = 0; i < mode->reg_num; i++, info++)
        nvp6158_write_reg(sensor->i2c, info->reg, info->val);

    LOG_I("Current mode: %dx%d@%d, mbus %s, mux %d%s",
          mode->width, mode->height, NVP_FPS(mode->fps),
          NVP_MBUS(mode->mbus), NVP_MUX(mode->mux),
          mode->res == NVP6158_RES_720H ? (mode->is_pal ? ", PAL" : ", NTSC") : "");
}

/**
 * @brief Convert video format to NVP mode configuration
 *
 * This function converts the given video format parameters to a matching NVP6158 mode
 * configuration. It searches through the available modes to find one that matches
 * the specified configuration：fmt + fps + mux -> mode
 *
 * @param fmt Pointer to the video format structure containing width, height, and bus type
 * @param fps Frame rate type (NVP6158_FPS_25 or NVP6158_FPS_30)
 * @param mux Multiplexing type (NVP6158_MUX_1, NVP6158_MUX_2, or NVP6158_MUX_4)
 *
 * @return Pointer to the matching nvp_mode structure if found, NULL otherwise
 *
 * @note For 720H resolution, the function also checks PAL/NTSC format matching
 */
static struct nvp_mode *nvp6158_fmt_to_mode(struct mpp_video_fmt *fmt,
                                            enum nvp6158_fps fps, enum nvp6158_mux mux)
{
    enum mpp_mbus_type mbus = fmt->bus_type;
    int i;

    if (!fmt) {
        LOG_E("Invalid parameters");
        return NULL;
    }

    if (fmt->bus_type != MEDIA_BUS_BT1120 && fmt->bus_type != MEDIA_BUS_BT656) {
        LOG_E("Unsupported bus type: %d", fmt->bus_type);
        return NULL;
    }

    for (i = 0; i < ARRAY_SIZE(g_nvp_modes); i++) {
        struct nvp_mode *m = &g_nvp_modes[i];

        if (m->width != fmt->width || m->height != fmt->height)
            continue;

        if (m->mbus != mbus || m->fps != fps || m->mux != mux)
            continue;

        return m;
    }

    LOG_E("Unsupported format: %dx%d@%d, mbus %s, mux %d",
          fmt->width, fmt->height, NVP_FPS(fps), NVP_MBUS(mbus), NVP_MUX(mux));
    return NULL;
}

static void nvp6158_cur_status(struct nvp_dev *sensor)
{
    u8 video = 0, motion = 0;

    nvp6158_write_reg(sensor->i2c, 0xFF, 0x00);
    nvp6158_read_reg(sensor->i2c, 0xA8, &video);
    nvp6158_read_reg(sensor->i2c, 0xA9, &motion);
    LOG_I("Video Status: lost 0x%x, motion 0x%x", video, motion);
}

static bool nvp6158_is_open(struct nvp_dev *sensor)
{
    return sensor->on;
}

static void nvp6158_power_on(struct nvp_dev *sensor)
{
    if (sensor->on || !sensor->resetb_pin)
        return;

    camera_pin_set_high(sensor->resetb_pin);
    aicos_msleep(1);
    camera_pin_set_low(sensor->resetb_pin);
    aicos_msleep(1);
    camera_pin_set_high(sensor->resetb_pin);

    LOG_I("Power on");
    sensor->on = true;
}

static void nvp6158_power_off(struct nvp_dev *sensor)
{
    if (!sensor->on || !sensor->resetb_pin)
        return;

    camera_pin_set_low(sensor->resetb_pin);

    LOG_I("Power off");
    sensor->on = false;
}

static int nvp6158_probe(struct nvp_dev *sensor)
{
    u8 id = 0, rev = 0;

    nvp6158_power_on(sensor);

    nvp6158_write_reg(sensor->i2c, 0xFF, 0x00);
    if (nvp6158_read_reg(sensor->i2c, 0xf4, &id) ||
        nvp6158_read_reg(sensor->i2c, 0xf5, &rev)) {
        nvp6158_power_off(sensor);
        return -1;
    }
    if ((id << 8 | rev) != NVP6158_CHIP_ID) {
        LOG_E("Invalid chip ID: 0x%02x 0x%02x\n", id, rev);
        nvp6158_power_off(sensor);
        return -1;
    }

    nvp6158_apply_cfg(sensor, sensor->cur_mode);
    nvp6158_cur_status(sensor);

    return 0;
}

static rt_err_t nvp6158_init(rt_device_t dev)
{
    struct nvp_dev *sensor = (struct nvp_dev *)dev;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    sensor->fps = NVP6158_DFT_FPS;
    sensor->mux = NVP6158_DFT_MUX;
    sensor->fmt.code   = NVP6158_DFT_CODE;
    sensor->fmt.width  = NVP6158_DFT_WIDTH;
    sensor->fmt.height = NVP6158_DFT_HEIGHT;
    sensor->fmt.bus_type = NVP6158_DFT_BUS_TYPE;
    sensor->fmt.flags = MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_VSYNC_ACTIVE_LOW  |
                        MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;
    sensor->cur_mode = nvp6158_fmt_to_mode(&sensor->fmt, sensor->fps, sensor->mux);
    if (!sensor->cur_mode) {
        LOG_E("Failed to find the mode for %dx%d@%d, mbus %s, mux %d",
              sensor->fmt.width, sensor->fmt.height, NVP_FPS(sensor->fps),
              NVP_MBUS(sensor->fmt.bus_type), NVP_MUX(sensor->mux));
        return -RT_EINVAL;
    }

    if (sensor->cur_mode->is_interlaced) {
        LOG_I("Interlace mode enabled");
        sensor->fmt.flags |= MEDIA_SIGNAL_INTERLACED_MODE;
    }

#ifdef _TEST_PATTERN_ON_
    LOG_I("Test mode enabled");
#endif
#ifndef _SDR_MODE_
    LOG_I("DDR mode enabled");
#endif

    sensor->resetb_pin = camera_rst_pin_get();
    sensor->cur_chan = 0;
    return RT_EOK;
}

static rt_err_t nvp6158_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct nvp_dev *sensor = (struct nvp_dev *)dev;

    if (nvp6158_is_open(sensor))
        return RT_EOK;

    if (nvp6158_probe(sensor))
        return -RT_ERROR;

    LOG_I("NVP6158 inited");
    return RT_EOK;
}

static rt_err_t nvp6158_close(rt_device_t dev)
{
    struct nvp_dev *sensor = (struct nvp_dev *)dev;

    if (!nvp6158_is_open(sensor))
        return -RT_ERROR;

    nvp6158_power_off(sensor);
    LOG_D("NVP6158 Close");
    return RT_EOK;
}

static int nvp6158_get_fmt(struct nvp_dev *sensor, struct mpp_video_fmt *fmt)
{
    if (!fmt) {
        LOG_E("Invalid parameters");
        return -RT_EINVAL;
    }

    fmt->code   = sensor->fmt.code;
    fmt->width  = sensor->fmt.width;
    fmt->height = sensor->fmt.height;
    fmt->flags  = sensor->fmt.flags;
    fmt->bus_type = sensor->fmt.bus_type;
    fmt->mux = NVP_MUX(sensor->mux);
    return RT_EOK;
}

static int nvp6158_set_fmt(struct nvp_dev *sensor, struct mpp_video_fmt *fmt)
{
    struct nvp_mode *new_mode = NULL;

    if (!fmt) {
        LOG_E("Invalid parameters");
        return -RT_EINVAL;
    }

    new_mode = nvp6158_fmt_to_mode(fmt, sensor->fps, sensor->mux);
    if (!new_mode) {
        LOG_E("Failed to find the mode for %dx%d@%d, mbus %s, mux %d",
              fmt->width, fmt->height, NVP_FPS(sensor->fps),
              NVP_MBUS(fmt->bus_type), NVP_MUX(new_mode->mux));
        return -RT_ERROR;
    }

    sensor->fmt.code   = fmt->code;
    sensor->fmt.width  = fmt->width;
    sensor->fmt.height = fmt->height;
    sensor->fmt.flags  = fmt->flags;
    sensor->fmt.bus_type = fmt->bus_type;
    sensor->fps = new_mode->fps;
    sensor->mux = new_mode->mux;
    sensor->cur_mode = new_mode;

    if (nvp6158_is_open(sensor))
        nvp6158_apply_cfg(sensor, new_mode);

    return RT_EOK;
}

static int nvp6158_start(struct nvp_dev *sensor)
{
    return 0;
}

static int nvp6158_stop(struct nvp_dev *sensor)
{
    return 0;
}

static int nvp6158_pause(struct nvp_dev *sensor)
{
    nvp6158_write_reg(sensor->i2c, 0xFF, 1);
    return nvp6158_write_reg(sensor->i2c, NVP6158_VDO_EN_REG, 0);
}

static int nvp6158_resume(struct nvp_dev *sensor)
{
    nvp6158_write_reg(sensor->i2c, 0xFF, 1);
    return nvp6158_write_reg(sensor->i2c, NVP6158_VDO_EN_REG, 0x66);
}

static int nvp6158_set_brightness(struct nvp_dev *sensor, u32 percent)
{
    s8 val = PERCENT_TO_INT((s8)0x80, (s8)0x7F, percent);

    nvp6158_write_reg(sensor->i2c, 0xFF, 0);
    return nvp6158_write_reg(sensor->i2c, NVP6158_BRIGHTNESS_REG(sensor->cur_chan), val);
}

static int nvp6158_set_saturation(struct nvp_dev *sensor, u32 percent)
{
    u8 val = PERCENT_TO_INT(1, 4, percent);

    if (val == 1)
        val = 0;

    if (val == 4)
        val = 0xFF;
    else
        val <<= 6;

    nvp6158_write_reg(sensor->i2c, 0xFF, 0);
    return nvp6158_write_reg(sensor->i2c, NVP6158_SATURATION_REG(sensor->cur_chan), val);
}

static int nvp6158_set_hue(struct nvp_dev *sensor, u32 percent)
{
    u8 val = PERCENT_TO_INT(0, 3, percent);

    if (val == 3)
        val = 0xFF;
    else
        val <<= 6;

    nvp6158_write_reg(sensor->i2c, 0xFF, 0);
    return nvp6158_write_reg(sensor->i2c, NVP6158_HUE_REG(sensor->cur_chan), val);
}

static int nvp6158_set_contrast(struct nvp_dev *sensor, u32 percent)
{
    s8 val = PERCENT_TO_INT((s8)0x80, (s8)0x7F, percent);

    nvp6158_write_reg(sensor->i2c, 0xFF, 0);
    return nvp6158_write_reg(sensor->i2c, NVP6158_CONTRAST_REG(sensor->cur_chan), val);
}

static int nvp6158_set_sharpness(struct nvp_dev *sensor, u32 percent)
{
    u8 val = PERCENT_TO_INT(0, 3, percent);

    if (val == 3)
        val = 0xF;
    else
        val <<= 2;

    nvp6158_write_reg(sensor->i2c, 0xFF, 0);
    return nvp6158_write_reg(sensor->i2c, NVP6158_SHARPNESS_REG(sensor->cur_chan),
                             (val << 4) | val);
}

static int nvp6158_set_auto_gain(struct nvp_dev *sensor, u32 percent)
{
    u8 val = PERCENT_TO_INT(1, 4, percent);

    if (val == 1)
        val = 0;

    if (val == 4)
        val = 0xFF;
    else
        val <<= 6;

    nvp6158_write_reg(sensor->i2c, 0xFF, 0);
    nvp6158_write_reg(sensor->i2c, NVP6158_U_GAIN_REG(sensor->cur_chan), val);
    return nvp6158_write_reg(sensor->i2c, NVP6158_V_GAIN_REG(sensor->cur_chan), val);
}

static int nvp6158_set_channel(struct nvp_dev *sensor, u32 chan)
{
    if (chan >= 4) {
        LOG_E("Invalid channel: %d, must be [0, 3]", chan);
        return -RT_EINVAL;
    }

    sensor->cur_chan = chan;
    LOG_I("Set current channel to: %d", sensor->cur_chan);
    return RT_EOK;
}

static int nvp6158_set_fps(struct nvp_dev *sensor, u32 fps)
{
    struct nvp_mode *new_mode = NULL;
    enum nvp6158_fps new_fps;

    if (fps == 25) {
        new_fps = NVP6158_FPS_25;
    } else if (fps == 30) {
        new_fps = NVP6158_FPS_30;
    } else {
        LOG_E("Unsupported FPS: %d", fps);
        return -RT_ERROR;
    }

    new_mode = nvp6158_fmt_to_mode(&sensor->fmt, new_fps, sensor->mux);
    if (!new_mode) {
        LOG_E("Failed to find the mode for %dx%d@%d, mbus %s, mux %d",
              sensor->fmt.width, sensor->fmt.height, NVP_FPS(new_fps),
              NVP_MBUS(sensor->fmt.bus_type), NVP_MUX(sensor->mux));
        return -RT_ERROR;
    }
    sensor->fps = new_fps;
    sensor->cur_mode = new_mode;

    if (nvp6158_is_open(sensor))
        nvp6158_apply_cfg(sensor, new_mode);
    return RT_EOK;
}

static int nvp6158_set_mux(struct nvp_dev *sensor, u32 mux)
{
    struct nvp_mode *new_mode = NULL;
    enum nvp6158_mux new_mux;

    if (mux == 1) {
        new_mux = NVP6158_MUX_1;
    } else if (mux == 2) {
        new_mux = NVP6158_MUX_2;
    } else if (mux == 4) {
        new_mux = NVP6158_MUX_4;
    } else {
        LOG_E("Unsupported mux: %d", mux);
        return -RT_ERROR;
    }

    new_mode = nvp6158_fmt_to_mode(&sensor->fmt, sensor->fps, new_mux);
    if (!new_mode) {
        LOG_E("Failed to find the mode for %dx%d@%d, mbus %s, mux %d",
              sensor->fmt.width, sensor->fmt.height, NVP_FPS(sensor->fps),
              NVP_MBUS(sensor->fmt.bus_type), NVP_MUX(new_mux));
        return -RT_ERROR;
    }
    sensor->mux = new_mux;
    sensor->cur_mode = new_mode;

    if (nvp6158_is_open(sensor))
        nvp6158_apply_cfg(sensor, new_mode);

    return RT_EOK;
}

static rt_err_t nvp6158_control(rt_device_t dev, int cmd, void *args)
{
    struct nvp_dev *sensor = (struct nvp_dev *)dev;

    switch (cmd) {
    case CAMERA_CMD_START:
        return nvp6158_start(sensor);
    case CAMERA_CMD_STOP:
        return nvp6158_stop(sensor);
    case CAMERA_CMD_PAUSE:
        return nvp6158_pause(sensor);
    case CAMERA_CMD_RESUME:
        return nvp6158_resume(sensor);
    case CAMERA_CMD_GET_FMT:
        return nvp6158_get_fmt(sensor, (struct mpp_video_fmt *)args);
    case CAMERA_CMD_SET_FMT:
        return nvp6158_set_fmt(sensor, (struct mpp_video_fmt *)args);
    case CAMERA_CMD_SET_FPS:
        return nvp6158_set_fps(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_CONTRAST:
        return nvp6158_set_contrast(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_BRIGHTNESS:
        return nvp6158_set_brightness(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_HUE:
        return nvp6158_set_hue(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_SATURATION:
        return nvp6158_set_saturation(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_MUX:
        return nvp6158_set_mux(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_CHANNEL:
        return nvp6158_set_channel(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_SHARPNESS:
        return nvp6158_set_sharpness(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_AUTOGAIN:
        return nvp6158_set_auto_gain(sensor, *(u32 *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops nvp6158_ops =
{
    .init = nvp6158_init,
    .open = nvp6158_open,
    .close = nvp6158_close,
    .control = nvp6158_control,
};
#endif

#ifdef RT_USING_PM
static int nvp6158_pm_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    return RT_EOK;
}

static void nvp6158_pm_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct nvp_dev *sensor = (struct nvp_dev *)device;

    switch (mode) {
    case PM_SLEEP_MODE_LIGHT:
        /* Light sleep: SoC was still powered, no action needed */
        break;
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        /* Deep sleep: SoC was powered off, camera must re-inited GPIO */
        sensor->resetb_pin = camera_rst_pin_get();
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops nvp6158_pm_ops =
{
    SET_DEVICE_PM_OPS(nvp6158_pm_suspend, nvp6158_pm_resume)
    NULL,
};
#endif /* RT_USING_PM */

int rt_hw_nvp6158_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_nvp6158_dev.dev.ops = &nvp6158_ops;
#else
    g_nvp6158_dev.dev.init = nvp6158_init;
    g_nvp6158_dev.dev.open = nvp6158_open;
    g_nvp6158_dev.dev.close = nvp6158_close;
    g_nvp6158_dev.dev.control = nvp6158_control;
#endif
    g_nvp6158_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_nvp6158_dev.dev, CAMERA_DEV_NAME, 0);

#ifdef RT_USING_PM
    rt_pm_device_register(&g_nvp6158_dev.dev, &nvp6158_pm_ops);
#endif

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_nvp6158_init);
