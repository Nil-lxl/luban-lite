/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "player_backend_ops.h"
#include "aic_ui.h"

#if LV_USE_AIC_SIMULATOR == 0
#include <stdlib.h>
#include <string.h>

extern const player_backend_ops_t *png_backend_get_template(void);
extern const player_backend_ops_t *aic_backend_get_template(void);

static bool is_suffix(const char *str, const char *suffix)
{
    size_t len_str = strlen(str);
    size_t len_suffix = strlen(suffix);

    if (len_suffix > len_str) {
        return false;
    }
    return strncmp(str + len_str - len_suffix, suffix, len_suffix) == 0;
}

const player_backend_ops_t * player_backend_match_file_suffix(const char *src)
{
    if (!src) return NULL;

    if (is_suffix(src, ".apng") || is_suffix(src, ".png")) {
        return png_backend_get_template();
    }

    return aic_backend_get_template();
}
#endif
