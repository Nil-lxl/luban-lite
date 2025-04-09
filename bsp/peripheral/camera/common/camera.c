/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "drv_camera.h"

#ifdef RT_USING_DEVICE_OPS
#define DEV_IOCTL       dev->ops->control
#else
#define DEV_IOCTL       dev->control
#endif

struct rt_i2c_bus_device *camera_i2c_get(void)
{
    struct rt_i2c_bus_device *i2c = NULL;
    char name[8] = "";

    snprintf(name, 8, "i2c%d", AIC_CAMERA_I2C_CHAN);
    i2c = rt_i2c_bus_device_find(name);
    if (i2c == RT_NULL) {
        LOG_E("Failed to open %s", name);
        return NULL;
    }

    return i2c;
}

void camera_xclk_enable(void)
{
    hal_clk_enable(CLK_OUT1);
}

void camera_xclk_disable(void)
{
    hal_clk_disable(CLK_OUT1);
}

u32 camera_xclk_rate_get(void)
{
#ifdef AIC_CLK_OUT1_FREQ
    return AIC_CLK_OUT1_FREQ;
#else
    return 0;
#endif
}

u32 camera_rst_pin_get(void)
{
    u32 pin = 0;

    pin = rt_pin_get(AIC_CAMERA_RST_PIN);
    if (pin)
        rt_pin_mode(pin, PIN_MODE_OUTPUT);
    else
        LOG_E("Failed to get reset PIN\n");

    return pin;
}

u32 camera_pwdn_pin_get(void)
{
    u32 pin = 0;

    pin = rt_pin_get(AIC_CAMERA_PWDN_PIN);
    if (pin)
        rt_pin_mode(pin, PIN_MODE_OUTPUT);
    else
        LOG_E("Failed to get power down PIN\n");

    return pin;
}

void camera_pin_set_high(u32 pin)
{
    if (!pin)
        return;

    rt_pin_write(pin, PIN_HIGH);
}

void camera_pin_set_low(u32 pin)
{
    if (!pin)
        return;

    rt_pin_write(pin, PIN_LOW);
}

int camera_get_fmt(struct rt_device *dev, void *fmt)
{
    if (!dev || !DEV_IOCTL || !fmt)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_GET_FMT, fmt);
}

int camera_set_fmt(struct rt_device *dev, void *fmt)
{
    if (!dev || !DEV_IOCTL || !fmt)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_FMT, fmt);
}

int camera_set_fps(struct rt_device *dev, u32 fps)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_FPS, &fps);
}

int camera_set_channel(struct rt_device *dev, u32 chan)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_CHANNEL, &chan);
}

int camera_set_brightness(struct rt_device *dev, u32 percent)
{
    if (!dev || !DEV_IOCTL || percent > 100)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_BRIGHTNESS, &percent);
}

int camera_set_contrast(struct rt_device *dev, u32 percent)
{
    if (!dev || !DEV_IOCTL || percent > 100)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_CONTRAST, &percent);
}

int camera_set_saturation(struct rt_device *dev, u32 percent)
{
    if (!dev || !DEV_IOCTL || percent > 100)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_SATURATION, &percent);
}

int camera_set_hue(struct rt_device *dev, u32 percent)
{
    if (!dev || !DEV_IOCTL || percent > 100)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_HUE, &percent);
}

int camera_set_sharpness(struct rt_device *dev, u32 percent)
{
    if (!dev || !DEV_IOCTL || percent > 100)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_SHARPNESS, &percent);
}

int camera_set_denoise(struct rt_device *dev, u32 percent)
{
    if (!dev || !DEV_IOCTL || percent > 100)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_DENOISE, &percent);
}

int camera_set_quality(struct rt_device *dev, u32 percent)
{
    if (!dev || !DEV_IOCTL || percent > 100)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_QUALITY, &percent);
}

int camera_set_aec_val(struct rt_device *dev, u32 percent)
{
    if (!dev || !DEV_IOCTL || percent > 100)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_AEC_VAL, &percent);
}

int camera_set_autogain(struct rt_device *dev, u32 percent)
{
    if (!dev || !DEV_IOCTL || percent > 100)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_AUTOGAIN, &percent);
}

int camera_set_exposure(struct rt_device *dev, u32 percent)
{
    if (!dev || !DEV_IOCTL || percent > 100)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_EXPOSURE, &percent);
}

int camera_set_gain_ctrl(struct rt_device *dev, bool enable)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_GAIN_CTRL, &enable);
}

int camera_set_whitebal(struct rt_device *dev, bool enable)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_WHITEBAL, (void *)enable);
}

int camera_set_awb(struct rt_device *dev, bool enable)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_AWB, &enable);
}

int camera_set_aec2(struct rt_device *dev, bool enable)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_AEC2, &enable);
}

int camera_set_dcw(struct rt_device *dev, bool enable)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_DCW, &enable);
}

int camera_set_bpc(struct rt_device *dev, bool enable)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_BPC, &enable);
}

int camera_set_wpc(struct rt_device *dev, bool enable)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_WPC, &enable);
}

int camera_set_h_flip(struct rt_device *dev, bool enable)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_H_FLIP, &enable);
}

int camera_set_v_flip(struct rt_device *dev, bool enable)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_V_FLIP, &enable);
}

int camera_set_colorbar(struct rt_device *dev, bool enable)
{
    if (!dev || !DEV_IOCTL)
        return -1;

    return DEV_IOCTL(dev, CAMERA_CMD_SET_COLORBAR, &enable);
}
