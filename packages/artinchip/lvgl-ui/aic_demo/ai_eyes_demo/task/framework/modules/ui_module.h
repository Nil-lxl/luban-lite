/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __UI_MODULE_H__
#define __UI_MODULE_H__

#include "../core/module.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get UI module instance
 */
module_t *ui_module_get(void);

#ifdef __cplusplus
}
#endif

#endif /* __UI_MODULE_H__ */

