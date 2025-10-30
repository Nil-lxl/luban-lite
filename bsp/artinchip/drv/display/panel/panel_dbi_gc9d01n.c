/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dbi.h"

static int panel_dbi_enable(struct aic_panel *panel)
{
    return 0;
}

static struct aic_panel_funcs gc9d01n_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_dbi_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing gc9d01n_timing = {
    .pixelclock   = 600000,

    .hactive      = 160,
    .hback_porch  = 2,
    .hfront_porch = 3,
    .hsync_len    = 1,

    .vactive      = 160,
    .vback_porch  = 3,
    .vfront_porch = 2,
    .vsync_len    = 1,
};

static struct panel_dbi dbi = {
    .type = SPI,
    .format = SPI_4LINE_RGB565,
};

struct aic_panel dbi_gc9d01n = {
    .name = "panel-gc9d01n",
    .timings = &gc9d01n_timing,
    .funcs = &gc9d01n_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
