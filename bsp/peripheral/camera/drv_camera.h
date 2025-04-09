/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef __DRV_CAMERA_H__
#define __DRV_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtdef.h"

#define CAMERA_DEV_NAME     "camera"

/* ioctl command of Camera device */

#define CAMERA_CMD_START                (RT_DEVICE_CTRL_BASE(CAMERA) + 0x01)
#define CAMERA_CMD_STOP                 (RT_DEVICE_CTRL_BASE(CAMERA) + 0x02)
/* Argument type: (struct mpp_video_fmt *) */
#define CAMERA_CMD_GET_FMT              (RT_DEVICE_CTRL_BASE(CAMERA) + 0x03)
#define CAMERA_CMD_SET_FMT              (RT_DEVICE_CTRL_BASE(CAMERA) + 0x05)
/* Argument type: unsigned int */
#define CAMERA_CMD_SET_CHANNEL          (RT_DEVICE_CTRL_BASE(CAMERA) + 0x08)
#define CAMERA_CMD_SET_FPS              (RT_DEVICE_CTRL_BASE(CAMERA) + 0x09)
/* Argument type: unsigned int，percent, [0, 100] */
#define CAMERA_CMD_SET_BRIGHTNESS       (RT_DEVICE_CTRL_BASE(CAMERA) + 0x10)
#define CAMERA_CMD_SET_CONTRAST         (RT_DEVICE_CTRL_BASE(CAMERA) + 0x11)
#define CAMERA_CMD_SET_SATURATION       (RT_DEVICE_CTRL_BASE(CAMERA) + 0x12)
#define CAMERA_CMD_SET_HUE              (RT_DEVICE_CTRL_BASE(CAMERA) + 0x13)
#define CAMERA_CMD_SET_SHARPNESS        (RT_DEVICE_CTRL_BASE(CAMERA) + 0x14)
#define CAMERA_CMD_SET_DENOISE          (RT_DEVICE_CTRL_BASE(CAMERA) + 0x15)
#define CAMERA_CMD_SET_QUALITY          (RT_DEVICE_CTRL_BASE(CAMERA) + 0x16)
#define CAMERA_CMD_SET_AUTOGAIN         (RT_DEVICE_CTRL_BASE(CAMERA) + 0x17)
#define CAMERA_CMD_SET_AEC_VAL          (RT_DEVICE_CTRL_BASE(CAMERA) + 0x18)
#define CAMERA_CMD_SET_EXPOSURE         (RT_DEVICE_CTRL_BASE(CAMERA) + 0x19)
/* Argument type: bool, 0, disable; 1, enable */
#define CAMERA_CMD_SET_GAIN_CTRL        (RT_DEVICE_CTRL_BASE(CAMERA) + 0x20)
#define CAMERA_CMD_SET_WHITEBAL         (RT_DEVICE_CTRL_BASE(CAMERA) + 0x21)
#define CAMERA_CMD_SET_AWB              (RT_DEVICE_CTRL_BASE(CAMERA) + 0x22)
#define CAMERA_CMD_SET_AEC2             (RT_DEVICE_CTRL_BASE(CAMERA) + 0x23)
#define CAMERA_CMD_SET_DCW              (RT_DEVICE_CTRL_BASE(CAMERA) + 0x24)
#define CAMERA_CMD_SET_BPC              (RT_DEVICE_CTRL_BASE(CAMERA) + 0x25)
#define CAMERA_CMD_SET_WPC              (RT_DEVICE_CTRL_BASE(CAMERA) + 0x26)
#define CAMERA_CMD_SET_H_FLIP           (RT_DEVICE_CTRL_BASE(CAMERA) + 0x27)
#define CAMERA_CMD_SET_V_FLIP           (RT_DEVICE_CTRL_BASE(CAMERA) + 0x28)
#define CAMERA_CMD_SET_COLORBAR         (RT_DEVICE_CTRL_BASE(CAMERA) + 0x2F)

int camera_get_fmt(struct rt_device *dev, void *fmt);
int camera_set_fmt(struct rt_device *dev, void *fmt);

int camera_set_channel(struct rt_device *dev, u32 chan);
int camera_set_fps(struct rt_device *dev, u32 fps);

int camera_set_brightness(struct rt_device *dev, u32 percent);
int camera_set_contrast(struct rt_device *dev, u32 percent);
int camera_set_saturation(struct rt_device *dev, u32 percent);
int camera_set_hue(struct rt_device *dev, u32 percent);
int camera_set_sharpness(struct rt_device *dev, u32 percent);
int camera_set_denoise(struct rt_device *dev, u32 percent);
int camera_set_quality(struct rt_device *dev, u32 percent);
int camera_set_autogain(struct rt_device *dev, u32 percent);
int camera_set_aec_val(struct rt_device *dev, u32 percent);
int camera_set_exposure(struct rt_device *dev, u32 percent);

int camera_set_gain_ctrl(struct rt_device *dev, bool enable);
int camera_set_whitebal(struct rt_device *dev, bool enable);
int camera_set_awb(struct rt_device *dev, bool enable);
int camera_set_aec2(struct rt_device *dev, bool enable);
int camera_set_dcw(struct rt_device *dev, bool enable);
int camera_set_bpc(struct rt_device *dev, bool enable);
int camera_set_wpc(struct rt_device *dev, bool enable);
int camera_set_h_flip(struct rt_device *dev, bool enable);
int camera_set_v_flip(struct rt_device *dev, bool enable);
int camera_set_colorbar(struct rt_device *dev, bool enable);

#ifdef __cplusplus
}
#endif

#endif
