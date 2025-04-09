/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"
#include <aic_hal.h>

static int panel_enable(struct aic_panel *panel)
{
    int ret;

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_dcs_send_seq(panel, 0xE0, 0x00);

    panel_dsi_dcs_send_seq(panel, 0xE1, 0x93);
    panel_dsi_dcs_send_seq(panel, 0xE2, 0x65);
    panel_dsi_dcs_send_seq(panel, 0xE3, 0xF8);
    panel_dsi_dcs_send_seq(panel, 0x80, 0x03);

    panel_dsi_dcs_send_seq(panel, 0xE0, 0x01);

    panel_dsi_dcs_send_seq(panel, 0x00, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x01, 0x41);
    panel_dsi_dcs_send_seq(panel, 0x03, 0x10);
    panel_dsi_dcs_send_seq(panel, 0x04, 0x44);
    panel_dsi_dcs_send_seq(panel, 0x17, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x18, 0xD0);
    panel_dsi_dcs_send_seq(panel, 0x19, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x1A, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x1B, 0xD0);
    panel_dsi_dcs_send_seq(panel, 0x1C, 0x00);

    panel_dsi_dcs_send_seq(panel, 0x24, 0xFE);
    panel_dsi_dcs_send_seq(panel, 0x35, 0x26);
    panel_dsi_dcs_send_seq(panel, 0x37, 0x09);

    panel_dsi_dcs_send_seq(panel, 0x38, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x39, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x3A, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0x3C, 0x78);
    panel_dsi_dcs_send_seq(panel, 0x3D, 0xFF);
    panel_dsi_dcs_send_seq(panel, 0x3E, 0xFF);
    panel_dsi_dcs_send_seq(panel, 0x3F, 0xFF);

    panel_dsi_dcs_send_seq(panel, 0x40, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x41, 0x64);
    panel_dsi_dcs_send_seq(panel, 0x42, 0xC7);
    panel_dsi_dcs_send_seq(panel, 0x43, 0x18);
    panel_dsi_dcs_send_seq(panel, 0x44, 0x0B);
    panel_dsi_dcs_send_seq(panel, 0x45, 0x14);

    panel_dsi_dcs_send_seq(panel, 0x55, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x57, 0x49);
    panel_dsi_dcs_send_seq(panel, 0x59, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0x5A, 0x1B);
    panel_dsi_dcs_send_seq(panel, 0x5B, 0x19);

    panel_dsi_dcs_send_seq(panel, 0x5D, 0x7F);
    panel_dsi_dcs_send_seq(panel, 0x5E, 0x56);
    panel_dsi_dcs_send_seq(panel, 0x5F, 0x43);
    panel_dsi_dcs_send_seq(panel, 0x60, 0x37);
    panel_dsi_dcs_send_seq(panel, 0x61, 0x33);
    panel_dsi_dcs_send_seq(panel, 0x62, 0x25);
    panel_dsi_dcs_send_seq(panel, 0x63, 0x2A);
    panel_dsi_dcs_send_seq(panel, 0x64, 0x16);
    panel_dsi_dcs_send_seq(panel, 0x65, 0x30);
    panel_dsi_dcs_send_seq(panel, 0x66, 0x2F);
    panel_dsi_dcs_send_seq(panel, 0x67, 0x32);
    panel_dsi_dcs_send_seq(panel, 0x68, 0x53);
    panel_dsi_dcs_send_seq(panel, 0x69, 0x43);
    panel_dsi_dcs_send_seq(panel, 0x6A, 0x4C);
    panel_dsi_dcs_send_seq(panel, 0x6B, 0x40);
    panel_dsi_dcs_send_seq(panel, 0x6C, 0x3D);
    panel_dsi_dcs_send_seq(panel, 0x6D, 0x31);
    panel_dsi_dcs_send_seq(panel, 0x6E, 0x20);
    panel_dsi_dcs_send_seq(panel, 0x6F, 0x0F);

    panel_dsi_dcs_send_seq(panel, 0x70, 0x7F);
    panel_dsi_dcs_send_seq(panel, 0x71, 0x56);
    panel_dsi_dcs_send_seq(panel, 0x72, 0x43);
    panel_dsi_dcs_send_seq(panel, 0x73, 0x37);
    panel_dsi_dcs_send_seq(panel, 0x74, 0x33);
    panel_dsi_dcs_send_seq(panel, 0x75, 0x25);
    panel_dsi_dcs_send_seq(panel, 0x76, 0x2A);
    panel_dsi_dcs_send_seq(panel, 0x77, 0x16);
    panel_dsi_dcs_send_seq(panel, 0x78, 0x30);
    panel_dsi_dcs_send_seq(panel, 0x79, 0x2F);
    panel_dsi_dcs_send_seq(panel, 0x7A, 0x32);
    panel_dsi_dcs_send_seq(panel, 0x7B, 0x53);
    panel_dsi_dcs_send_seq(panel, 0x7C, 0x43);
    panel_dsi_dcs_send_seq(panel, 0x7D, 0x4C);
    panel_dsi_dcs_send_seq(panel, 0x7E, 0x40);
    panel_dsi_dcs_send_seq(panel, 0x7F, 0x3D);
    panel_dsi_dcs_send_seq(panel, 0x80, 0x31);
    panel_dsi_dcs_send_seq(panel, 0x81, 0x20);
    panel_dsi_dcs_send_seq(panel, 0x82, 0x0F);

    panel_dsi_dcs_send_seq(panel, 0xE0, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x00, 0x5F);
    panel_dsi_dcs_send_seq(panel, 0x01, 0x5F);
    panel_dsi_dcs_send_seq(panel, 0x02, 0x5E);
    panel_dsi_dcs_send_seq(panel, 0x03, 0x5E);
    panel_dsi_dcs_send_seq(panel, 0x04, 0x50);
    panel_dsi_dcs_send_seq(panel, 0x05, 0x48);
    panel_dsi_dcs_send_seq(panel, 0x06, 0x48);
    panel_dsi_dcs_send_seq(panel, 0x07, 0x4A);
    panel_dsi_dcs_send_seq(panel, 0x08, 0x4A);
    panel_dsi_dcs_send_seq(panel, 0x09, 0x44);
    panel_dsi_dcs_send_seq(panel, 0x0A, 0x44);
    panel_dsi_dcs_send_seq(panel, 0x0B, 0x46);
    panel_dsi_dcs_send_seq(panel, 0x0C, 0x46);
    panel_dsi_dcs_send_seq(panel, 0x0D, 0x5F);
    panel_dsi_dcs_send_seq(panel, 0x0E, 0x5F);
    panel_dsi_dcs_send_seq(panel, 0x0F, 0x57);
    panel_dsi_dcs_send_seq(panel, 0x10, 0x57);
    panel_dsi_dcs_send_seq(panel, 0x11, 0x77);
    panel_dsi_dcs_send_seq(panel, 0x12, 0x77);
    panel_dsi_dcs_send_seq(panel, 0x13, 0x40);
    panel_dsi_dcs_send_seq(panel, 0x14, 0x42);
    panel_dsi_dcs_send_seq(panel, 0x15, 0x5F);

    panel_dsi_dcs_send_seq(panel, 0x16, 0x5F);
    panel_dsi_dcs_send_seq(panel, 0x17, 0x5F);
    panel_dsi_dcs_send_seq(panel, 0x18, 0x5E);
    panel_dsi_dcs_send_seq(panel, 0x19, 0x5E);
    panel_dsi_dcs_send_seq(panel, 0x1A, 0x50);
    panel_dsi_dcs_send_seq(panel, 0x1B, 0x49);
    panel_dsi_dcs_send_seq(panel, 0x1C, 0x49);
    panel_dsi_dcs_send_seq(panel, 0x1D, 0x4B);
    panel_dsi_dcs_send_seq(panel, 0x1E, 0x4B);
    panel_dsi_dcs_send_seq(panel, 0x1F, 0x45);
    panel_dsi_dcs_send_seq(panel, 0x20, 0x45);
    panel_dsi_dcs_send_seq(panel, 0x21, 0x47);
    panel_dsi_dcs_send_seq(panel, 0x22, 0x47);
    panel_dsi_dcs_send_seq(panel, 0x23, 0x5F);
    panel_dsi_dcs_send_seq(panel, 0x24, 0x5F);
    panel_dsi_dcs_send_seq(panel, 0x25, 0x57);
    panel_dsi_dcs_send_seq(panel, 0x26, 0x57);
    panel_dsi_dcs_send_seq(panel, 0x27, 0x77);
    panel_dsi_dcs_send_seq(panel, 0x28, 0x77);
    panel_dsi_dcs_send_seq(panel, 0x29, 0x41);
    panel_dsi_dcs_send_seq(panel, 0x2A, 0x43);
    panel_dsi_dcs_send_seq(panel, 0x2B, 0x5F);

    panel_dsi_dcs_send_seq(panel, 0x2C, 0x1E);
    panel_dsi_dcs_send_seq(panel, 0x2D, 0x1E);
    panel_dsi_dcs_send_seq(panel, 0x2E, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0x2F, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0x30, 0x10);
    panel_dsi_dcs_send_seq(panel, 0x31, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x32, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x33, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x34, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x35, 0x0B);
    panel_dsi_dcs_send_seq(panel, 0x36, 0x0B);
    panel_dsi_dcs_send_seq(panel, 0x37, 0x09);
    panel_dsi_dcs_send_seq(panel, 0x38, 0x09);
    panel_dsi_dcs_send_seq(panel, 0x39, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0x3A, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0x3B, 0x17);
    panel_dsi_dcs_send_seq(panel, 0x3C, 0x17);
    panel_dsi_dcs_send_seq(panel, 0x3D, 0x17);
    panel_dsi_dcs_send_seq(panel, 0x3E, 0x17);
    panel_dsi_dcs_send_seq(panel, 0x3F, 0x03);
    panel_dsi_dcs_send_seq(panel, 0x40, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x41, 0x1F);

    panel_dsi_dcs_send_seq(panel, 0x42, 0x1E);
    panel_dsi_dcs_send_seq(panel, 0x43, 0x1E);
    panel_dsi_dcs_send_seq(panel, 0x44, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0x45, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0x46, 0x10);
    panel_dsi_dcs_send_seq(panel, 0x47, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x48, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x49, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x4A, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x4B, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0x4C, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0x4D, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x4E, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x4F, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0x50, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0x51, 0x17);
    panel_dsi_dcs_send_seq(panel, 0x52, 0x17);
    panel_dsi_dcs_send_seq(panel, 0x53, 0x17);
    panel_dsi_dcs_send_seq(panel, 0x54, 0x17);
    panel_dsi_dcs_send_seq(panel, 0x55, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x56, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x57, 0x1F);

    panel_dsi_dcs_send_seq(panel, 0xE0, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x58, 0x40);
    panel_dsi_dcs_send_seq(panel, 0x59, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x5A, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x5B, 0x30);
    panel_dsi_dcs_send_seq(panel, 0x5C, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x5D, 0x30);
    panel_dsi_dcs_send_seq(panel, 0x5E, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x5F, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x60, 0x30);
    panel_dsi_dcs_send_seq(panel, 0x61, 0x03);
    panel_dsi_dcs_send_seq(panel, 0x62, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x63, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x64, 0xA6);
    panel_dsi_dcs_send_seq(panel, 0x65, 0x43);
    panel_dsi_dcs_send_seq(panel, 0x66, 0x30);
    panel_dsi_dcs_send_seq(panel, 0x67, 0x73);
    panel_dsi_dcs_send_seq(panel, 0x68, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x69, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x6A, 0x7F);
    panel_dsi_dcs_send_seq(panel, 0x6B, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x6C, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x6D, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x6E, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x6F, 0x88);

    panel_dsi_dcs_send_seq(panel, 0x75, 0xD9);
    panel_dsi_dcs_send_seq(panel, 0x76, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x77, 0x33);
    panel_dsi_dcs_send_seq(panel, 0x78, 0x43);

    panel_dsi_dcs_send_seq(panel, 0xE0, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x35, 0x00);

    ret = panel_dsi_dcs_exit_sleep_mode(panel);
    if (ret < 0) {
        printf("Failed to exit sleep mode: %d\n", ret);
        return ret;
    }
    aic_delay_ms(120);

    ret = panel_dsi_dcs_set_display_on(panel);
    if (ret < 0) {
        printf("Failed to set display on: %d\n", ret);
        return ret;
    }
    aic_delay_ms(20);

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

static struct display_timing jd9365_timing = {
    .pixelclock = 60000000,
    .hactive = 800,
    .hfront_porch = 60,
    .hback_porch = 20,
    .hsync_len = 20,
    .vactive = 800,
    .vfront_porch = 24,
    .vback_porch = 12,
    .vsync_len = 4,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST | DSI_CLOCK_NON_CONTINUOUS,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_jd9365 = {
    .name = "panel-jd9365",
    .timings = &jd9365_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
