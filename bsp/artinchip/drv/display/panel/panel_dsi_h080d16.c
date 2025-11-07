/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"
#include <aic_hal.h>

#if defined AIC_USING_D213ECV_EzUIX1_DEMO_V1
#define RESET_PIN  "PB.6"
#elif defined AIC_USING_D213ECV_EzUIX1_DEMO_V0
#define RESET_PIN  "PA.3"
#endif

static struct gpio_desc reset_gpio;

static void panel_gpio_init(struct aic_panel *panel) {
    panel_get_gpio(&reset_gpio, RESET_PIN);

    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel) {
    int ret;

    panel_gpio_init(panel);

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_generic_send_seq(panel, 0x27, 0xAA);
    panel_dsi_generic_send_seq(panel, 0x48, 0x02);
    panel_dsi_generic_send_seq(panel, 0xB6, 0x20);
    panel_dsi_generic_send_seq(panel, 0x01, 0x80);
    panel_dsi_generic_send_seq(panel, 0x02, 0xB0);
    panel_dsi_generic_send_seq(panel, 0x03, 0x47);

    panel_dsi_generic_send_seq(panel, 0x04, 0x64);  //hfp
    panel_dsi_generic_send_seq(panel, 0x05, 0x02);  //hsync
    panel_dsi_generic_send_seq(panel, 0x06, 0x64);  //hbp

    panel_dsi_generic_send_seq(panel, 0x07, 0x00);

    panel_dsi_generic_send_seq(panel, 0x08, 0x14);  //vfp
    panel_dsi_generic_send_seq(panel, 0x09, 0x02);  //vsync
    panel_dsi_generic_send_seq(panel, 0x0A, 0x14);  //vbp

    panel_dsi_generic_send_seq(panel, 0x0B, 0x82);
    panel_dsi_generic_send_seq(panel, 0x0C, 0x13);

    // panel_dsi_generic_send_seq(panel, 0x0B, 0x02);
    // panel_dsi_generic_send_seq(panel, 0x0C, 0x51);

    panel_dsi_generic_send_seq(panel, 0x0D, 0x01);
    panel_dsi_generic_send_seq(panel, 0x0E, 0x80);
    panel_dsi_generic_send_seq(panel, 0x0F, 0x20);
    panel_dsi_generic_send_seq(panel, 0x10, 0x20);
    panel_dsi_generic_send_seq(panel, 0x11, 0x03);
    panel_dsi_generic_send_seq(panel, 0x12, 0x1B);
    panel_dsi_generic_send_seq(panel, 0x13, 0x53);
    panel_dsi_generic_send_seq(panel, 0x14, 0x01);
    panel_dsi_generic_send_seq(panel, 0x15, 0x43);
    panel_dsi_generic_send_seq(panel, 0x16, 0x20);
    panel_dsi_generic_send_seq(panel, 0x17, 0x1F);
    panel_dsi_generic_send_seq(panel, 0x18, 0x01);
    panel_dsi_generic_send_seq(panel, 0x19, 0x43);
    panel_dsi_generic_send_seq(panel, 0x1A, 0x20);
    panel_dsi_generic_send_seq(panel, 0x1B, 0x1F);
    panel_dsi_generic_send_seq(panel, 0x1E, 0x46);
    panel_dsi_generic_send_seq(panel, 0x51, 0x30);
    panel_dsi_generic_send_seq(panel, 0x88, 0x40);
    panel_dsi_generic_send_seq(panel, 0x1F, 0x10);
    panel_dsi_generic_send_seq(panel, 0x2A, 0x01);


    panel_dsi_setup_realmode(panel);

    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs panel_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h080d16_timing = {
    .pixelclock = 157 * 1000 * 1000,
    .hactive = 1920,
    .hfront_porch = 100,
    .hback_porch = 100,
    .hsync_len = 2,
    .vactive = 1200,
    .vfront_porch = 20,
    .vback_porch = 20,
    .vsync_len = 2,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_PULSE,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_h080d16 = {
    .name = "panel-h080d16",
    .timings = &h080d16_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

