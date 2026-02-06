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

static struct aic_panel_funcs spi_general_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_dbi_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing spi_general_timing = {
    .pixelclock   = 600000,

    .hactive      = SPI_GENERAL_WIDTH,
    .vactive      = SPI_GENERAL_HEIGHT,
};

static struct panel_dbi dbi = {
    .type = SPI,
    .format = SPI_4LINE_RGB565,
};

struct aic_panel spi_general = {
    .name = "panel-spi-general",
    .timings = &spi_general_timing,
    .funcs = &spi_general_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
