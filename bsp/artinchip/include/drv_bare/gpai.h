/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao Li <siyao.li@artinchip.com>
 */

#ifndef __BL_GPAI_H_
#define __BL_GPAI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_gpai.h"

void drv_gpai_init(void);
int drv_gpai_chan_init(int ch);
int drv_gpai_get_data_mode(int ch);
int drv_gpai_get_mode(int ch);
int drv_gpai_irq_callback(struct aic_gpai_irq_info *irq_info);
int drv_gpai_enabled(int ch, bool enabled);
int drv_gpai_get_ch_info(struct aic_gpai_ch_info *chan_info);
int drv_gpai_config_dma(struct aic_dma_transfer_info *dma_info);
int drv_gpai_get_dma_data(int ch);
int drv_gpai_stop_dma(int ch);

#ifdef __cplusplus
}
#endif

#endif /* __BL_GPAI_H_ */
