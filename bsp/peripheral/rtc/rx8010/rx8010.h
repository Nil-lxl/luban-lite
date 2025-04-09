/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-12-11        the first version
 */
#ifndef __RX8010_H__
#define __RX8010_H__

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <time.h>
#include <string.h>

#define RX8010_ADDR         0x32

#define RX8010_SEC          0x10
#define RX8010_MIN          0x11
#define RX8010_HOUR         0x12
#define RX8010_WDAY         0x13
#define RX8010_MDAY         0x14
#define RX8010_MONTH        0x15
#define RX8010_YEAR         0x16
#define RX8010_RESV17       0x17
#define RX8010_ALMIN        0x18
#define RX8010_ALHOUR       0x19
#define RX8010_ALWDAY       0x1A
#define RX8010_TCOUNT0      0x1B
#define RX8010_TCOUNT1      0x1C
#define RX8010_EXT          0x1D
#define RX8010_FLAG         0x1E
#define RX8010_CTRL         0x1F
/* 0x20 to 0x2F are user registers */
#define RX8010_RESV30       0x30
#define RX8010_RESV31       0x31
#define RX8010_IRQ          0x32

#define RX8010_EXT_WADA     (1 << 3)
#define RX8010_FLAG_VLF     (1 << 1)
#define RX8010_FLAG_AF      (1 << 3)
#define RX8010_FLAG_TF      (1 << 4)
#define RX8010_FLAG_UF      (1 << 5)
#define RX8010_CTRL_AIE     (1 << 3)
#define RX8010_CTRL_UIE     (1 << 5)
#define RX8010_CTRL_STOP    (1 << 6)
#define RX8010_CTRL_TEST    (1 << 7)
#define RX8010_ALARM_AE     (1 << 7)

extern int rt_hw_rx8010_init(void);

#endif
