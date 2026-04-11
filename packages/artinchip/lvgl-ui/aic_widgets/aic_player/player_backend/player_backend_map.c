/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"
#if LVGL_VERSION_MAJOR == 8
void *lv_malloc_zeroed(size_t size)
{
    void *buffer = lv_mem_alloc(size);
    if (!buffer)
        return NULL;
    memset(buffer, 0, size);
    return buffer;
}
#endif
