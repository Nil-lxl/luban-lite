/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Huahui Mai <huahui.mai@artinchip.com>
 */

#include "panel_com.h"
#include "panel_dsi.h"

#define JD9366TC_RST  "PF.1"

static struct gpio_desc reset_gpio;

static void panel_gpio_init(void)
{
    panel_get_gpio(&reset_gpio, JD9366TC_RST);

    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(200);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel)
{
    int ret;

    panel_gpio_init();

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_dcs_send_seq(panel, 0x30,0x01);
    panel_dsi_dcs_send_seq(panel, 0x78,0x49,0x61,0x02,0x00);
    panel_dsi_dcs_send_seq(panel, 0x30,0x02);
    panel_dsi_dcs_send_seq(panel, 0x31,0x12);
    panel_dsi_dcs_send_seq(panel, 0x32,0x08);
    panel_dsi_dcs_send_seq(panel, 0x33,0x3f);
    panel_dsi_dcs_send_seq(panel, 0x3c,0x04);
    panel_dsi_dcs_send_seq(panel, 0x3d,0x78);
    panel_dsi_dcs_send_seq(panel, 0x3e,0x43);
    panel_dsi_dcs_send_seq(panel, 0x3f,0x30);
    panel_dsi_dcs_send_seq(panel, 0x42,0xa2);
    panel_dsi_dcs_send_seq(panel, 0x43,0xf0);
    panel_dsi_dcs_send_seq(panel, 0x44,0x01);
    panel_dsi_dcs_send_seq(panel, 0x46,0x17);
    panel_dsi_dcs_send_seq(panel, 0x49,0xc0);
    panel_dsi_dcs_send_seq(panel, 0x6d,0x30);
    panel_dsi_dcs_send_seq(panel, 0x6e,0x21);
    panel_dsi_dcs_send_seq(panel, 0x41,0x5b,0x5b,0x03,0x03,0x5b,0x5b,0x02,0x02,0x03,0x03,0x03,0x03);
    panel_dsi_dcs_send_seq(panel, 0x5a,0x00,0x00,0x34,0x34,0x31,0x31,0x23,0x23,0x24,0x24,0x23);
    panel_dsi_dcs_send_seq(panel, 0x5b,0x23,0x0b,0x0b,0x09,0x09,0x0f,0x0f,0x0d,0x0d,0x06,0x06);
    panel_dsi_dcs_send_seq(panel, 0x5c,0x00,0x00,0x34,0x34,0x31,0x31,0x23,0x23,0x24,0x24,0x23);
    panel_dsi_dcs_send_seq(panel, 0x5d,0x23,0x0a,0x0a,0x08,0x08,0x0e,0x0e,0x0c,0x0c,0x05,0x05);
    panel_dsi_dcs_send_seq(panel, 0x5e,0x00,0x00,0x31,0x31,0x34,0x34,0x23,0x23,0x24,0x24,0x23);
    panel_dsi_dcs_send_seq(panel, 0x5f,0x23,0x0c,0x0c,0x0e,0x0e,0x08,0x08,0x0a,0x0a,0x05,0x05);
    panel_dsi_dcs_send_seq(panel, 0x60,0x00,0x00,0x31,0x31,0x34,0x34,0x23,0x23,0x24,0x24,0x23);
    panel_dsi_dcs_send_seq(panel, 0x61,0x23,0x0d,0x0d,0x0f,0x0f,0x09,0x09,0x0b,0x0b,0x06,0x06);
    panel_dsi_dcs_send_seq(panel, 0x64,0xff,0xff,0x3f);
    panel_dsi_dcs_send_seq(panel, 0x65,0xff,0xff,0x3f);
    panel_dsi_dcs_send_seq(panel, 0x6f,0x03,0x00,0x00);
    panel_dsi_dcs_send_seq(panel, 0x70,0x03,0x00,0x00);
    panel_dsi_dcs_send_seq(panel, 0x71,0x00,0x00,0x80);
    panel_dsi_dcs_send_seq(panel, 0x72,0x00,0x00,0x00);
    panel_dsi_dcs_send_seq(panel, 0x4c,0x22,0x22);
    panel_dsi_dcs_send_seq(panel, 0x73,0x2a);
    panel_dsi_dcs_send_seq(panel, 0x4e,0x00,0x00);
    panel_dsi_dcs_send_seq(panel, 0x50,0x00,0x00);
    panel_dsi_dcs_send_seq(panel, 0x55,0xff,0xff);
    panel_dsi_dcs_send_seq(panel, 0x56,0xff,0xff);
    panel_dsi_dcs_send_seq(panel, 0x57,0x00,0x00);
    panel_dsi_dcs_send_seq(panel, 0x58,0xff,0xff);
    panel_dsi_dcs_send_seq(panel, 0x66,0xff,0xff);
    panel_dsi_dcs_send_seq(panel, 0x67,0xff,0xff);
    panel_dsi_dcs_send_seq(panel, 0x4a,0x3f);
    panel_dsi_dcs_send_seq(panel, 0x30,0x08);
    panel_dsi_dcs_send_seq(panel, 0x31,0x65);
    panel_dsi_dcs_send_seq(panel, 0x33,0x05);
    panel_dsi_dcs_send_seq(panel, 0x40,0x50);
    panel_dsi_dcs_send_seq(panel, 0x41,0x80);
    panel_dsi_dcs_send_seq(panel, 0x42,0x1a);
    panel_dsi_dcs_send_seq(panel, 0x47,0x0a);
    panel_dsi_dcs_send_seq(panel, 0x48,0x0d);
    panel_dsi_dcs_send_seq(panel, 0x50,0x17);
    panel_dsi_dcs_send_seq(panel, 0x5a,0x20);
    panel_dsi_dcs_send_seq(panel, 0x5b,0x00);
    panel_dsi_dcs_send_seq(panel, 0x5c,0x53);
    panel_dsi_dcs_send_seq(panel, 0x62,0x04);
    panel_dsi_dcs_send_seq(panel, 0x65,0x5f);
    panel_dsi_dcs_send_seq(panel, 0x73,0x01);
    panel_dsi_dcs_send_seq(panel, 0x30,0x0a);
    panel_dsi_dcs_send_seq(panel, 0x32,0xff);
    panel_dsi_dcs_send_seq(panel, 0x33,0x28);
    panel_dsi_dcs_send_seq(panel, 0x3f,0x53);
    panel_dsi_dcs_send_seq(panel, 0x40,0x15);
    panel_dsi_dcs_send_seq(panel, 0x47,0x20);
    panel_dsi_dcs_send_seq(panel, 0x48,0x80);
    panel_dsi_dcs_send_seq(panel, 0x49,0x03);
    panel_dsi_dcs_send_seq(panel, 0x30,0x0b);
    panel_dsi_dcs_send_seq(panel, 0x33,0x00,0x42);
    panel_dsi_dcs_send_seq(panel, 0x3c,0x00,0xbf);
    panel_dsi_dcs_send_seq(panel, 0x43,0xb1);
    panel_dsi_dcs_send_seq(panel, 0x44,0x31);
    panel_dsi_dcs_send_seq(panel, 0x3e,0x00,0x10,0x22,0x28,0x32);
    panel_dsi_dcs_send_seq(panel, 0x3f,0x56,0x72,0x73,0x7c,0x77,0x91,0x90,0x9d,0xac,0xa9,0xae,0xb5,0xc5,0xb8);
    panel_dsi_dcs_send_seq(panel, 0x40,0x55,0x5D,0x62,0x73,0x00,0x10,0x22,0x28,0x32);
    panel_dsi_dcs_send_seq(panel, 0x41,0x56,0x72,0x73,0x7c,0x77,0x91,0x90,0x9d,0xac,0xa9,0xae,0xb5,0xc5,0xb8);
    panel_dsi_dcs_send_seq(panel, 0x42,0x55,0x5D,0x62,0x73);
    panel_dsi_dcs_send_seq(panel, 0x45,0x70);
    panel_dsi_dcs_send_seq(panel, 0x46,0x3b);
    panel_dsi_dcs_send_seq(panel, 0x48,0x7c);
    panel_dsi_dcs_send_seq(panel, 0x49,0x1e);
    panel_dsi_dcs_send_seq(panel, 0x4a,0x3a);
    panel_dsi_dcs_send_seq(panel, 0x30,0x0c);
    panel_dsi_dcs_send_seq(panel, 0x32,0x62);
    panel_dsi_dcs_send_seq(panel, 0x71,0x77);
    panel_dsi_dcs_send_seq(panel, 0x30,0x0d);
    panel_dsi_dcs_send_seq(panel, 0x4c,0x74);
    panel_dsi_dcs_send_seq(panel, 0x30,0x00);
    panel_dsi_dcs_send_seq(panel, 0x35,0x00);

    ret = panel_dsi_dcs_exit_sleep_mode(panel);
    if (ret < 0) {
        pr_err("Failed to exit sleep mode: %d\n", ret);
        return ret;
    }

    aic_delay_ms(120);

    ret = panel_dsi_dcs_set_display_on(panel);
    if (ret < 0) {
        pr_err("Failed to set display on: %d\n", ret);
        return ret;
    }

    aic_delay_ms(10);

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

static struct display_timing jd9366tc_timing = {
    .pixelclock = 130000000,
    .hactive = 800,
    .hfront_porch = 280,
    .hback_porch = 280,
    .hsync_len = 8,
    .vactive = 1280,
    .vfront_porch = 280,
    .vback_porch = 20,
    .vsync_len = 8,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_jd9366tc = {
    .name = "panel-jd9366tc",
    .timings = &jd9366tc_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
