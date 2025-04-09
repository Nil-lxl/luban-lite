/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-11-11        the first version
 */

#ifndef __FT6336_H__
#define __FT6336_H__

#include "touch.h"

#define FT6336_MAX_TOUCH            5
#define FT6336_POINT_LEN            6

/* FT6336 Have Two Address */
// #define FT6336_SALVE_ADDR           0x38
#define FT6336_SALVE_ADDR           0x48

/* FT6336 REG */
#define FT6336_DEVICE_MODE          0x00
#define FT6336_GEST_ID              0x01
#define FT6336_TD_STATUS            0x02
#define FT6336_TOUCH1_XH            0x03
#define FT6336_TOUCH1_XL            0x04
#define FT6336_TOUCH1_YH            0x05
#define FT6336_TOUCH1_YL            0x06
#define FT6336_TOUCH1_WEIGHT        0x07
#define FT6336_TOUCH1_MISC          0x08
#define FT6336_G_THGROUP            0x80
#define FT6336_G_PERIODACTIVE       0x88

#endif
