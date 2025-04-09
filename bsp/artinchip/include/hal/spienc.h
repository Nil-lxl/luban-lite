/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Hao Xiong <hao.xiong@artinchip.com>
 */

#ifndef _AIC_SPIENC_H_
#define _AIC_SPIENC_H_

#include <hal_spienc.h>

static inline int spienc_init(void)
{
    return hal_spienc_init();
}

static inline void spienc_set_cfg(u32 spi_bus, u32 addr, u32 cpos, u32 clen)
{
    hal_spienc_set_cfg(spi_bus, addr, cpos, clen);
}

static inline void spienc_set_bypass(int status)
{
    hal_spienc_set_bypass(status);
}

static inline void spienc_select_tweak(int select)
{
    hal_spienc_select_tweak(select);
}

static inline void spienc_start(void)
{
    hal_spienc_start();
}

static inline void spienc_stop(void)
{
    hal_spienc_stop();
}

static inline int spienc_check_empty(void)
{
    return hal_spienc_check_empty();
}

#endif
