/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __APP_FRAMEWORK_H__
#define __APP_FRAMEWORK_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the application framework
 * Initialize message bus and register all modules
 * @return 0 on success, <0 on failure
 */
int app_framework_init(void);

/**
 * Deinitialize the application framework
 * Unregister all registered modules
 * @return 0 on success, <0 on failure
 */
int app_framework_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_FRAMEWORK_H__ */

