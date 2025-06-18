/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"

#ifndef LVDS_PANEL_MATCH_ID
#define LVDS_PANEL_MATCH_ID 0
#endif

#ifndef RGB_PANEL_MATCH_ID
#define RGB_PANEL_MATCH_ID 0
#endif

#ifndef PANEL_PIXELCLOCK
#define PANEL_PIXELCLOCK  52
#endif

#ifndef PANEL_HACTIVE
#define PANEL_HACTIVE   1024
#endif

#ifndef PANEL_HBP
#define PANEL_HBP       160
#endif

#ifndef PANEL_HFP
#define PANEL_HFP       160
#endif

#ifndef PANEL_HSW
#define PANEL_HSW       20
#endif

#ifndef PANEL_VACTIVE
#define PANEL_VACTIVE   600
#endif

#ifndef PANEL_VBP
#define PANEL_VBP       12
#endif

#ifndef PANEL_VFP
#define PANEL_VFP       20
#endif

#ifndef PANEL_VSW
#define PANEL_VSW       2
#endif

static struct aic_panel_funcs simple_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing timing_custom = {
    .pixelclock   = PANEL_PIXELCLOCK * 1000000,

    .hactive      = PANEL_HACTIVE,
    .hback_porch  = PANEL_HBP,
    .hfront_porch = PANEL_HFP,
    .hsync_len    = PANEL_HSW,

    .vactive      = PANEL_VACTIVE,
    .vback_porch  = PANEL_VBP,
    .vfront_porch = PANEL_VFP,
    .vsync_len    = PANEL_VSW,

    .flags        = AIC_DISPLAY_FLAGS,
};

#ifdef AIC_DISP_RGB
static struct display_timing timing_480_272 = {
    .pixelclock   = 10 * 1000000,

    .hactive      = 480,
    .hback_porch  = 43,
    .hfront_porch = 8,
    .hsync_len    = 1,

    .vactive      = 272,
    .vback_porch  = 12,
    .vfront_porch = 4,
    .vsync_len    = 10,

    .flags        = AIC_DISPLAY_FLAGS,
};

static struct display_timing timing_800_480 = {
    .pixelclock   = 28 * 1000000,

    .hactive      = 800,
    .hback_porch  = 10,
    .hfront_porch = 5,
    .hsync_len    = 40,

    .vactive      = 480,
    .vback_porch  = 32,
    .vfront_porch = 12,
    .vsync_len    = 3,

    .flags        = AIC_DISPLAY_FLAGS,
};
#endif

#if defined(AIC_DISP_RGB) || defined(AIC_DISP_LVDS)
static struct display_timing timing_1024_600 = {
    .pixelclock   = 52 * 1000000,

    .hactive      = 1024,
    .hback_porch  = 160,
    .hfront_porch = 160,
    .hsync_len    = 20,

    .vactive      = 600,
    .vback_porch  = 12,
    .vfront_porch = 20,
    .vsync_len    = 2,

    .flags        = AIC_DISPLAY_FLAGS,
};
#endif

#ifdef AIC_DISP_LVDS
static struct display_timing timing_1920_1080 = {
    .pixelclock   = 142 * 1000000,

    .hactive      = 1920,
    .hback_porch  = 80,
    .hfront_porch = 120,
    .hsync_len    = 20,

    .vactive      = 1080,
    .vback_porch  = 20,
    .vfront_porch = 20,
    .vsync_len    = 10,

    .flags        = AIC_DISPLAY_FLAGS,
};
#endif

#ifdef AIC_DISP_RGB
static struct panel_rgb rgb = {
    .mode = AIC_RGB_MODE,
    .format = AIC_RGB_FORMAT,
    .clock_phase = AIC_RGB_CLK_CTL,
    .data_order = AIC_RGB_DATA_ORDER,
    .data_mirror = AIC_RGB_DATA_MIRROR,
};

static struct panel_desc rgb_desc[] = {
    [0] = {
        .name = "1024x600",
        .rgb = &rgb,
        .timings = &timing_1024_600,
        .funcs = &simple_funcs,
    },
    [1] = {
        .name = "800x480",
        .rgb = &rgb,
        .timings = &timing_800_480,
        .funcs = &simple_funcs,
    },
    [2] = {
        .name = "480x272",
        .rgb = &rgb,
        .timings = &timing_480_272,
        .funcs = &simple_funcs,
    },
    [3] = {
        .name = "custom",
        .rgb = &rgb,
        .timings = &timing_custom,
        .funcs = &simple_funcs,
    },
};

struct aic_panel aic_panel_rgb = {
    .name = "panel-rgb",
    .desc = rgb_desc,
    .match_num = ARRAY_SIZE(rgb_desc),
    .match_id = RGB_PANEL_MATCH_ID,
    .connector_type = AIC_RGB_COM,
};
#endif

#ifdef AIC_DISP_LVDS
static struct panel_lvds lvds = {
    .mode      = AIC_LVDS_MODE,
    .link_mode = AIC_LVDS_LINK_MODE,
    .link_swap = AIC_LVDS_LINK_SWAP_EN,
    .lanes     = { AIC_LVDS_LINK0_LANES, AIC_LVDS_LINK1_LANES },
    .pols      = { AIC_LVDS_LINK0_POL, AIC_LVDS_LINK1_POL },
};

static struct panel_desc lvds_desc[] = {
    [0] = {
        .name = "1024x600",
        .lvds = &lvds,
        .timings = &timing_1024_600,
        .funcs = &simple_funcs,
    },
    [1] = {
        .name = "1920x1080",
        .lvds = &lvds,
        .timings = &timing_1920_1080,
        .funcs = &simple_funcs,
    },
    [2] = {
        .name = "custom",
        .lvds = &lvds,
        .timings = &timing_custom,
        .funcs = &simple_funcs,
    },
};

struct aic_panel aic_panel_lvds = {
    .name = "panel-lvds",
    .desc = lvds_desc,
    .match_num = ARRAY_SIZE(lvds_desc),
    .match_id = LVDS_PANEL_MATCH_ID,
    .connector_type = AIC_LVDS_COM,
};
#endif

