/*
 * Copyright (c) 2026-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xyg <yiguan.xu@artinchip.com>
 */

#ifndef __DRV_ALC5616_H__
#define __DRV_ALC5616_H__

#include "codec.h"
#include "aic_common.h"

/* ALC5616 Register Map */
#define ALC5616_SW_RESET        0x00
#define ALC5616_LINE_OUT_CTRL1  0x03
#define ALC5616_LINE_OUT_CTRL2  0x05
#define ALC5616_IN1_IN2         0x0D
#define ALC5616_IN1_VOL_CTL     0x0F
#define ALC5616_DAC_DIG_VOL     0x19
#define ALC5616_ADC_DIG_VOL     0x1C
#define ALC5616_ADC_DIG_MIXER   0x27
#define ALC5616_DAC_DIG_MIXER   0x2A
#define ALC5616_RECMIXL_CTRL2   0x3C
#define ALC5616_RECMIXR_CTRL2   0x3E
#define ALC5616_HPOMIX_CTRL     0x45
#define ALC5616_OUTMIXL_CTRL3   0x4F
#define ALC5616_OUTMIXR_CTRL3   0x52
#define ALC5616_LOUTMIX_CTRL    0x53
#define ALC5616_PWR_MGMT1       0x61
#define ALC5616_PWR_MGMT2       0x62
#define ALC5616_PWR_MGMT3       0x63
#define ALC5616_PWR_MGMT4       0x64
#define ALC5616_PWR_MGMT5       0x65
#define ALC5616_PWR_MGMT6       0x66
#define ALC5616_PR_INDEX        0x6A
#define ALC5616_PR_DATA         0x6C
#define ALC5616_I2S1_CTRL       0x70
#define ALC5616_ADC_DAC_CLK1    0x73
#define ALC5616_ADC_DAC_CLK2    0x74
#define ALC5616_GLOBAL_CLK_CTRL 0x80
#define ALC5616_HP_AMP_CTRL1    0x8E
#define ALC5616_HP_AMP_CTRL2    0x8F
#define ALC5616_SOFT_VOL_ZCD    0xD9
#define ALC5616_I2S1_GNL_CTL    0xFA
#define ALC5616_VENDOR_ID       0xFE
#define ALC5616_REG_MAX         0xFF

#endif /* __DRV_ALC5616_H__ */
