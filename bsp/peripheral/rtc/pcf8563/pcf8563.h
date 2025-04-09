/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-11-15        the first version
 */

#ifndef __PCF8563_H__
#define __PCF8563_H__

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <time.h>
#include <string.h>

/* slave address */
#define PCF8563_ARRD            0x51

/* register */
#define REG_PCF8563_STATE1      0x00
#define REG_PCF8563_STATE2      0x01
#define REG_PCF8563_SEC         0x02
#define REG_PCF8563_MIN         0x03
#define REG_PCF8563_HOUR        0x04
#define REG_PCF8563_DAY         0x05
#define REG_PCF8563_WEEK        0x06
#define REG_PCF8563_MON         0x07
#define REG_PCF8563_YEAR        0x08
#define REG_PCF8563_CLKOUT      0x0d

/* offset */
#define SHIELD_PCF8563_STATE1   (unsigned char)0xa8
#define SHIELD_PCF8563_STATE2   (unsigned char)0x1f
#define SHIELD_PCF8563_SEC      (unsigned char)0x7f
#define SHIELD_PCF8563_MIN      (unsigned char)0x7f
#define SHIELD_PCF8563_HOUR     (unsigned char)0x3f
#define SHIELD_PCF8563_DAY      (unsigned char)0x3f
#define SHIELD_PCF8563_WEEK     (unsigned char)0x07
#define SHIELD_PCF8563_MON      (unsigned char)0x1f
#define SHIELD_PCF8563_YEAR     (unsigned char)0xff

extern int rt_hw_pcf8563_init(void);

#endif
