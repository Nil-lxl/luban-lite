/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"
#include <aic_hal.h>
#include "disp_gpio.h"

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

    // panel_dsi_generic_send_seq(panel, 0xee, 0x01);
    // panel_dsi_generic_send_seq(panel, 0xea, 0x7A, 0xAA);
    panel_dsi_generic_send_seq(panel, 0xee, 0x01);
    panel_dsi_generic_send_seq(panel, 0xea, 0x07);
    panel_dsi_generic_send_seq(panel, 0xeb, 0x12);
    panel_dsi_generic_send_seq(panel, 0x0a, 0x45);
    panel_dsi_generic_send_seq(panel, 0x17, 0x35);
    panel_dsi_generic_send_seq(panel, 0x28, 0x25);
    panel_dsi_generic_send_seq(panel, 0x29, 0x20);
    panel_dsi_generic_send_seq(panel, 0x2a, 0x62);
    panel_dsi_generic_send_seq(panel, 0x2f, 0xf3);
    panel_dsi_generic_send_seq(panel, 0x45, 0x20);
    panel_dsi_generic_send_seq(panel, 0xee, 0x02);
    panel_dsi_generic_send_seq(panel, 0x39, 0xa0);
    panel_dsi_generic_send_seq(panel, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x01, 0x04);
    panel_dsi_generic_send_seq(panel, 0x02, 0x08);
    panel_dsi_generic_send_seq(panel, 0x03, 0x07);
    panel_dsi_generic_send_seq(panel, 0x04, 0x0d);
    panel_dsi_generic_send_seq(panel, 0x05, 0x27);
    panel_dsi_generic_send_seq(panel, 0x06, 0x0a);
    panel_dsi_generic_send_seq(panel, 0x07, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x08, 0x0d);
    panel_dsi_generic_send_seq(panel, 0x09, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x0A, 0x10);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x44);
    panel_dsi_generic_send_seq(panel, 0x0c, 0x13);
    panel_dsi_generic_send_seq(panel, 0x0d, 0x19);
    panel_dsi_generic_send_seq(panel, 0x0e, 0x28);
    panel_dsi_generic_send_seq(panel, 0x0f, 0x2d);
    panel_dsi_generic_send_seq(panel, 0x10, 0x3F);
    panel_dsi_generic_send_seq(panel, 0x20, 0x00);
    panel_dsi_generic_send_seq(panel, 0x21, 0x04);
    panel_dsi_generic_send_seq(panel, 0x22, 0x08);
    panel_dsi_generic_send_seq(panel, 0x23, 0x07);
    panel_dsi_generic_send_seq(panel, 0x24, 0x0d);
    panel_dsi_generic_send_seq(panel, 0x25, 0x27);
    panel_dsi_generic_send_seq(panel, 0x26, 0x0a);
    panel_dsi_generic_send_seq(panel, 0x27, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x28, 0x0d);
    panel_dsi_generic_send_seq(panel, 0x29, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x2A, 0x10);
    panel_dsi_generic_send_seq(panel, 0x2b, 0x44);
    panel_dsi_generic_send_seq(panel, 0x2c, 0x13);
    panel_dsi_generic_send_seq(panel, 0x2d, 0x19);
    panel_dsi_generic_send_seq(panel, 0x2e, 0x28);
    panel_dsi_generic_send_seq(panel, 0x2f, 0x2d);
    panel_dsi_generic_send_seq(panel, 0x30, 0x3F);

    // panel_dsi_generic_send_seq(panel, 0x00, 0x00);
    // panel_dsi_generic_send_seq(panel, 0x01, 0x02);
    // panel_dsi_generic_send_seq(panel, 0x02, 0x06);
    // panel_dsi_generic_send_seq(panel, 0x03, 0x07);
    // panel_dsi_generic_send_seq(panel, 0x04, 0x0d);
    // panel_dsi_generic_send_seq(panel, 0x05, 0x21);
    // panel_dsi_generic_send_seq(panel, 0x06, 0x0b);
    // panel_dsi_generic_send_seq(panel, 0x07, 0x0d);
    // panel_dsi_generic_send_seq(panel, 0x08, 0x0e);
    // panel_dsi_generic_send_seq(panel, 0x09, 0x0d);
    // panel_dsi_generic_send_seq(panel, 0x0A, 0x10);
    // panel_dsi_generic_send_seq(panel, 0x0b, 0x3f);
    // panel_dsi_generic_send_seq(panel, 0x0c, 0x11);
    // panel_dsi_generic_send_seq(panel, 0x0d, 0x16);
    // panel_dsi_generic_send_seq(panel, 0x0e, 0x27);
    // panel_dsi_generic_send_seq(panel, 0x0f, 0x2b);
    // panel_dsi_generic_send_seq(panel, 0x10, 0x3F);
    // panel_dsi_generic_send_seq(panel, 0x20, 0x00);
    // panel_dsi_generic_send_seq(panel, 0x21, 0x02);
    // panel_dsi_generic_send_seq(panel, 0x22, 0x06);
    // panel_dsi_generic_send_seq(panel, 0x23, 0x07);
    // panel_dsi_generic_send_seq(panel, 0x24, 0x0d);
    // panel_dsi_generic_send_seq(panel, 0x25, 0x21);
    // panel_dsi_generic_send_seq(panel, 0x26, 0x0b);
    // panel_dsi_generic_send_seq(panel, 0x27, 0x0d);
    // panel_dsi_generic_send_seq(panel, 0x28, 0x0e);
    // panel_dsi_generic_send_seq(panel, 0x29, 0x0d);
    // panel_dsi_generic_send_seq(panel, 0x2A, 0x10);
    // panel_dsi_generic_send_seq(panel, 0x2b, 0x3f);
    // panel_dsi_generic_send_seq(panel, 0x2c, 0x11);
    // panel_dsi_generic_send_seq(panel, 0x2d, 0x16);
    // panel_dsi_generic_send_seq(panel, 0x2e, 0x27);
    // panel_dsi_generic_send_seq(panel, 0x2f, 0x2b);
    // panel_dsi_generic_send_seq(panel, 0x30, 0x3F);

    panel_dsi_generic_send_seq(panel, 0xee, 0x03);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x55);
    panel_dsi_generic_send_seq(panel, 0x0f, 0xb9);
    panel_dsi_generic_send_seq(panel, 0xee, 0x04);
    panel_dsi_generic_send_seq(panel, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x01, 0x00);
    panel_dsi_generic_send_seq(panel, 0x02, 0xf0);
    panel_dsi_generic_send_seq(panel, 0x03, 0x03);
    panel_dsi_generic_send_seq(panel, 0x04, 0x20);
    panel_dsi_generic_send_seq(panel, 0x06, 0x06);
    panel_dsi_generic_send_seq(panel, 0x07, 0x05);
    panel_dsi_generic_send_seq(panel, 0x08, 0x12);
    panel_dsi_generic_send_seq(panel, 0x09, 0x20);
    panel_dsi_generic_send_seq(panel, 0x0a, 0x0b);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x00);
    panel_dsi_generic_send_seq(panel, 0x20, 0x00);
    panel_dsi_generic_send_seq(panel, 0x23, 0x38);
    panel_dsi_generic_send_seq(panel, 0x2A, 0x00);
    panel_dsi_generic_send_seq(panel, 0x40, 0x80);
    panel_dsi_generic_send_seq(panel, 0x41, 0x55);
    panel_dsi_generic_send_seq(panel, 0x46, 0x10);
    panel_dsi_generic_send_seq(panel, 0x47, 0x10);
    panel_dsi_generic_send_seq(panel, 0x48, 0x0f);
    panel_dsi_generic_send_seq(panel, 0x49, 0x0f);
    panel_dsi_generic_send_seq(panel, 0xee, 0x05);
    panel_dsi_generic_send_seq(panel, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0x01, 0x06);
    panel_dsi_generic_send_seq(panel, 0x02, 0x25);
    panel_dsi_generic_send_seq(panel, 0x03, 0x05);
    panel_dsi_generic_send_seq(panel, 0x07, 0xcb);
    panel_dsi_generic_send_seq(panel, 0x08, 0xd0);
    panel_dsi_generic_send_seq(panel, 0x09, 0x33);
    panel_dsi_generic_send_seq(panel, 0x10, 0x04);
    panel_dsi_generic_send_seq(panel, 0x11, 0x08);
    panel_dsi_generic_send_seq(panel, 0x12, 0x45);
    panel_dsi_generic_send_seq(panel, 0x13, 0x05);
    panel_dsi_generic_send_seq(panel, 0x19, 0xCd);
    panel_dsi_generic_send_seq(panel, 0x1a, 0x73);
    panel_dsi_generic_send_seq(panel, 0x23, 0x00);
    panel_dsi_generic_send_seq(panel, 0x30, 0x01);
    panel_dsi_generic_send_seq(panel, 0x31, 0x00);
    panel_dsi_generic_send_seq(panel, 0x32, 0x00);
    panel_dsi_generic_send_seq(panel, 0x33, 0x14);
    panel_dsi_generic_send_seq(panel, 0x34, 0x14);
    panel_dsi_generic_send_seq(panel, 0x35, 0x80);
    panel_dsi_generic_send_seq(panel, 0x40, 0x00);
    panel_dsi_generic_send_seq(panel, 0x43, 0x13);
    panel_dsi_generic_send_seq(panel, 0x44, 0x01);
    panel_dsi_generic_send_seq(panel, 0x45, 0x81);
    panel_dsi_generic_send_seq(panel, 0x46, 0x06);
    panel_dsi_generic_send_seq(panel, 0x47, 0x00);
    panel_dsi_generic_send_seq(panel, 0xee, 0x06);
    panel_dsi_generic_send_seq(panel, 0x00, 0x01);
    panel_dsi_generic_send_seq(panel, 0x02, 0x44);
    panel_dsi_generic_send_seq(panel, 0x06, 0xCD);
    panel_dsi_generic_send_seq(panel, 0x08, 0x67);
    panel_dsi_generic_send_seq(panel, 0x09, 0x45);
    panel_dsi_generic_send_seq(panel, 0x0A, 0x23);
    panel_dsi_generic_send_seq(panel, 0x0B, 0x01);
    panel_dsi_generic_send_seq(panel, 0xee, 0x07);
    panel_dsi_generic_send_seq(panel, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x01, 0x04);
    panel_dsi_generic_send_seq(panel, 0x02, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x03, 0x0d);
    panel_dsi_generic_send_seq(panel, 0x04, 0x20);
    panel_dsi_generic_send_seq(panel, 0x05, 0x21);
    panel_dsi_generic_send_seq(panel, 0x06, 0x14);
    panel_dsi_generic_send_seq(panel, 0x07, 0x16);
    panel_dsi_generic_send_seq(panel, 0x08, 0x10);
    panel_dsi_generic_send_seq(panel, 0x09, 0x12);
    panel_dsi_generic_send_seq(panel, 0x0A, 0x3c);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x3c);
    panel_dsi_generic_send_seq(panel, 0x20, 0x01);
    panel_dsi_generic_send_seq(panel, 0x21, 0x04);
    panel_dsi_generic_send_seq(panel, 0x22, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x23, 0x0d);
    panel_dsi_generic_send_seq(panel, 0x24, 0x20);
    panel_dsi_generic_send_seq(panel, 0x25, 0x21);
    panel_dsi_generic_send_seq(panel, 0x26, 0x15);
    panel_dsi_generic_send_seq(panel, 0x27, 0x17);
    panel_dsi_generic_send_seq(panel, 0x28, 0x11);
    panel_dsi_generic_send_seq(panel, 0x29, 0x13);
    panel_dsi_generic_send_seq(panel, 0x2A, 0x3c);
    panel_dsi_generic_send_seq(panel, 0x2b, 0x3c);
    panel_dsi_generic_send_seq(panel, 0xee, 0x08);
    panel_dsi_generic_send_seq(panel, 0x12, 0xda);
    panel_dsi_generic_send_seq(panel, 0x13, 0x1c);
    panel_dsi_generic_send_seq(panel, 0x18, 0x00);
    panel_dsi_generic_send_seq(panel, 0x41, 0x80);
    panel_dsi_generic_send_seq(panel, 0x42, 0x0c);
    panel_dsi_generic_send_seq(panel, 0xee, 0x0F);
    panel_dsi_generic_send_seq(panel, 0x00, 0x01);
    panel_dsi_generic_send_seq(panel, 0x01, 0x10);
    panel_dsi_generic_send_seq(panel, 0xea, 0x00);
    panel_dsi_generic_send_seq(panel, 0xeb, 0x00);
    panel_dsi_generic_send_seq(panel, 0xee, 0x00);
    panel_dsi_generic_send_seq(panel, 0x36, 0x00);
    aic_delay_ms(200);

    // panel_dsi_generic_send_seq(panel, 0x11);
    // aic_delay_ms(150);
    // panel_dsi_generic_send_seq(panel, 0x29);
    // aic_delay_ms(150);

    //////OTP/////////
    panel_dsi_generic_send_seq(panel, 0x28, 0x00);
    panel_dsi_generic_send_seq(panel, 0xee, 0x0a);
    panel_dsi_generic_send_seq(panel, 0xea, 0x07);
    panel_dsi_generic_send_seq(panel, 0xeb, 0x12);
    panel_dsi_generic_send_seq(panel, 0x30, 0x78);
    panel_dsi_generic_send_seq(panel, 0x31, 0x82);
    panel_dsi_generic_send_seq(panel, 0x35, 0x02);
    panel_dsi_generic_send_seq(panel, 0x36, 0x02);
    panel_dsi_generic_send_seq(panel, 0x37, 0x01);
    aic_delay_ms(200);
    panel_dsi_generic_send_seq(panel, 0x00, 0x80);
    panel_dsi_generic_send_seq(panel, 0x05, 0x40);
    panel_dsi_generic_send_seq(panel, 0x06, 0x41);
    panel_dsi_generic_send_seq(panel, 0x07, 0x11);
    panel_dsi_generic_send_seq(panel, 0x08, 0x55);
    panel_dsi_generic_send_seq(panel, 0x09, 0x40);
    aic_delay_ms(200);
    panel_dsi_generic_send_seq(panel, 0xea, 0x78);
    panel_dsi_generic_send_seq(panel, 0xeb, 0x69);
    aic_delay_ms(200);
    panel_dsi_generic_send_seq(panel, 0xee, 0x0a);
    panel_dsi_generic_send_seq(panel, 0xea, 0x07);
    panel_dsi_generic_send_seq(panel, 0xeb, 0x12);
    panel_dsi_generic_send_seq(panel, 0x31, 0x02);
    panel_dsi_generic_send_seq(panel, 0x37, 0x00);
    panel_dsi_generic_send_seq(panel, 0xee, 0x01);

    panel_dsi_generic_send_seq(panel, 0xee, 0x00);
    panel_dsi_generic_send_seq(panel, 0xea, 0x00);
    panel_dsi_generic_send_seq(panel, 0xeb, 0x00);
    panel_dsi_generic_send_seq(panel, 0x29);
    aic_delay_ms(150);

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

static struct display_timing h043a31_timing = {
    .pixelclock = 30 * 1000 * 1000,
    .hactive = 800,
    .hfront_porch = 90,
    .hback_porch = 80,
    .hsync_len = 20,
    .vactive = 480,
    .vfront_porch = 20,
    .vback_porch = 8,
    .vsync_len = 8,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_h043a31 = {
    .name = "panel-h043a31",
    .timings = &h043a31_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

