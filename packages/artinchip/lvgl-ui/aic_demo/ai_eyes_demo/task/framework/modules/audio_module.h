/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __AUDIO_MODULE_H__
#define __AUDIO_MODULE_H__

#include "../core/module.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get Audio module instance
 */
module_t *audio_module_get(void);

/**
 * Set audio output volume
 * @param volume Volume value (0-100)
 * @return RT_EOK on success, otherwise failure code
 */
int audio_module_set_volume(int volume);

/**
 * Get current audio output volume
 * @return Current volume (0-100); returns cached value if device is not open or read fails
 */
int audio_module_get_volume(void);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_MODULE_H__ */

