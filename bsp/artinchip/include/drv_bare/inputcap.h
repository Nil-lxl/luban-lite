/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#ifndef _INPUTCAP_H_
#define _INPUTCAP_H_

#include "aic_common.h"

#define INPUTCAP_WATER_MARK              64

struct inputcap_fifo_buf {
    uint32_t *event0_buf;
    uint32_t event0_buflen;
    uint32_t *event1_buf;
    uint32_t event1_buflen;
};

struct inputcap_cb {
    void (*func_event0)(void *para);
    void (*func_event1)(void *para);
};

#ifdef AIC_DMA_DRV
int aic_inputcap_setbuf(u32 ch, struct inputcap_fifo_buf *buf);
#endif
int aic_inputcap_open(u32 ch, struct inputcap_cb cb);
int aic_inputcap_close(u32 ch);
int aic_inputcap_int_dis(u32 ch, u8 event);
u32* aic_get_inputcap_data(u32 ch, u8 event);
int drv_inputcap_init(void);

#endif // end of _INPUTCAP_H_
