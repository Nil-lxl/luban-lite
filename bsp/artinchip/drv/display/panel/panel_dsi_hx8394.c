/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"

#define HX8394_RESET_PIN    "PE.5"

static struct gpio_desc reset_gpio;

/* Manufacturer specific commands sent via DSI, listed in HX8394-F datasheet */
#define HX8394_CMD_SETSEQUENCE    0xb0
#define HX8394_CMD_SETPOWER       0xb1
#define HX8394_CMD_SETDISP        0xb2
#define HX8394_CMD_SETCYC         0xb4
#define HX8394_CMD_SETVCOM        0xb6
#define HX8394_CMD_SETTE          0xb7
#define HX8394_CMD_SETSENSOR      0xb8
#define HX8394_CMD_SETEXTC        0xb9
#define HX8394_CMD_SETMIPI        0xba
#define HX8394_CMD_SETOTP         0xbb
#define HX8394_CMD_SETREGBANK     0xbd
#define HX8394_CMD_SETSTBA        0xc0
#define HX8394_CMD_SETDGCLUT      0xc1
#define HX8394_CMD_SETID          0xc3
#define HX8394_CMD_SETDDB         0xc4
#define HX8394_CMD_SETECO         0xc6
#define HX8394_CMD_SETCABC        0xc9
#define HX8394_CMD_SETCABCGAIN    0xca
#define HX8394_CMD_SETPANEL       0xcc
#define HX8394_CMD_SETOFFSET      0xd2
#define HX8394_CMD_SETGIP0        0xd3
#define HX8394_CMD_SETIOOPT       0xd4
#define HX8394_CMD_SETGIP1        0xd5
#define HX8394_CMD_SETGIP2        0xd6
#define HX8394_CMD_SETGIP3        0xd8
#define HX8394_CMD_SETGPO         0xd6
#define HX8394_CMD_SETSCALING     0xdd
#define HX8394_CMD_SETIDLE        0xdf
#define HX8394_CMD_SETGAMMA       0xe0
#define HX8394_CMD_SETCHEMODE_DYN 0xe4
#define HX8394_CMD_SETCHE         0xe5
#define HX8394_CMD_SETCESEL       0xe6
#define HX8394_CMD_SET_SP_CMD     0xe9
#define HX8394_CMD_SETREADINDEX   0xfe
#define HX8394_CMD_GETSPIREAD     0xff

static void panel_gpio_init(struct aic_panel *panel)
{
    panel_get_gpio(&reset_gpio, HX8394_RESET_PIN);

    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(100);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(120);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel)
{
    int ret;

    panel_gpio_init(panel);

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    /* 5.19.8 SETEXTC: Set extension command (B9h) */
    panel_dsi_generic_send_seq(panel, 0xb9,
                   0xff, 0x83, 0x94);

    /* 5.19.9 SETMIPI: Set MIPI control (BAh) */
    panel_dsi_generic_send_seq(panel, 0xba,
                   0x63, 0x03, 0x68, 0x7b, 0xb2, 0xc0);

    /* 5.19.2 SETPOWER: Set power (B1h) */
    panel_dsi_generic_send_seq(panel, 0xb1,
                   0x48, 0x0f, 0x6f, 0x09, 0x32, 0x54, 0x71, 0x51, 0x57, 0x43);

    /* 5.19.3 SETOFFSET: Set VP_REFS and VN_REF voltage (D2h) */
    panel_dsi_generic_send_seq(panel, 0xd2,
                   0x00, 0x80, 0x78, 0x0c, 0x0d, 0x22);

    /* 5.19.3 SETDISP: Set display related register (B2h) */
    panel_dsi_generic_send_seq(panel, 0xb2,
                   0x00, 0x80, 0x78, 0x0c, 0x0d, 0x22, 0x00, 0x00, 0x00, 0x00,
                   0xc8);

    /* 5.19.4 SETCYC: Set display waveform cycles (B4h) */
    panel_dsi_generic_send_seq(panel, 0xb4,
                   0x69, 0x6a, 0x69, 0x6a, 0x69, 0x6a, 0x01, 0x0c, 0x7c, 0x55,
                   0x00, 0x3f, 0x69, 0x6a, 0x69, 0x6a, 0x69, 0x6a, 0x01, 0x0c,
                   0x7c);

    /* 5.19.5 SETVCOM: Set VCOM voltage (B6h) */
    panel_dsi_generic_send_seq(panel, 0xb6,
                   0x6e, 0x6e);

    /* 5.19.19 SETGIP0: Set GIP Option0 (D3h) */
    panel_dsi_generic_send_seq(panel, 0xd3,
                   0xd3, 0x00, 0x00, 0x07, 0x07, 0x40, 0x07, 0x0c, 0x00, 0x08,
                   0x10, 0x08, 0x54, 0x15, 0xaa, 0x05, 0xaa, 0x02, 0x15, 0x16,
                   0x06, 0x05, 0x06, 0x47, 0x44, 0x0a, 0x0a, 0x4b, 0x10, 0x07,
                   0x07, 0x0c, 0x40);

    /* 5.19.20 Set GIP Option1 (D5h) */
    panel_dsi_generic_send_seq(panel, 0xd5,
                   0x20, 0x18, 0x18, 0x21, 0x26, 0x18, 0x24, 0x27, 0x1c, 0x25,
                   0x1d, 0x1c, 0x00, 0x1d, 0x02, 0x01, 0x04, 0x03, 0x18, 0x18,
                   0x06, 0x05, 0x18, 0x18, 0x08, 0x07, 0x18, 0x18, 0x18, 0x18,
                   0x18, 0x18, 0x18, 0x18, 0x18, 0x09, 0x18, 0x18,
                   0x0a, 0x0b, 0x18, 0x18, 0x18, 0x18);

    /* 5.19.21 Set GIP Option2 (D6h) */
    panel_dsi_generic_send_seq(panel, 0xd6,
                   0x25, 0x18, 0x18, 0x24, 0x27, 0x18, 0x21, 0x26, 0x1c, 0x20,
                   0x1D, 0x1c, 0x0b, 0x1d, 0x09, 0x0a, 0x07, 0x08, 0x18, 0x18,
                   0x05, 0x06, 0x18, 0x18, 0x03, 0x04, 0x18, 0x18, 0x18, 0x18,
                   0x18, 0x18, 0x18, 0x18, 0x18, 0x02, 0x18, 0x18, 0x01, 0x00,
                   0x18, 0x18, 0x18, 0x18);

    /* 5.19.25 SETGAMMA: Set gamma curve related setting (E0h) */
    panel_dsi_generic_send_seq(panel, 0xe0,
                   0x00, 0x22, 0x2c, 0x32, 0x34, 0x38, 0x3b, 0x39, 0x71, 0x7f,
                   0x8d, 0x8a, 0x91, 0x9e, 0xa2, 0xa4, 0xad, 0xae, 0xa6, 0xb2,
                   0xbf, 0x5c, 0x57, 0x5f, 0x5d, 0x5e, 0x5a, 0x54, 0x5c, 0x00,
                   0x22, 0x2c, 0x32, 0x34, 0x38, 0x3b, 0x39, 0x71, 0x7f, 0x8d,
                   0x8a, 0x90, 0x9f, 0xa0, 0xa3, 0xaf, 0xaf, 0xa7, 0xb2, 0xbf,
                   0x5c, 0x59, 0x5f, 0x5e, 0x5e, 0x5c, 0x58, 0x5c);

    /* 5.19.12 SETIOOPT (C0h), Set Source Option */
    panel_dsi_generic_send_seq(panel, 0xc0,
                   0x1f, 0x31);

    /* 5.19.17 SETPANEL (CCh) */
    panel_dsi_generic_send_seq(panel, 0xcc,
                   0x03);

    /* 5.19.23 SETIOPT (D4h) */
    panel_dsi_generic_send_seq(panel, 0xd4,
                   0x02);

    /* 5.19.11 Set register bank (BDh) */
    panel_dsi_generic_send_seq(panel, 0xbd,
                   0x02);

    /* 5.19.26 SETGIP3 (D8h) */
    panel_dsi_generic_send_seq(panel, 0xd8,
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF);

    /* 5.19.11 Set register bank (BDh) */
    panel_dsi_generic_send_seq(panel, 0xbd,
                   0x00);

    /* 5.19.11 Set register bank (BDh) */
    panel_dsi_generic_send_seq(panel, 0xbd,
                   0x01);

    /* 5.19.2 SETPOWER: Set power (B1h) */
    panel_dsi_generic_send_seq(panel, 0xb1,
                   0x00);

    /* 5.19.11 Set register bank (BDh) */
    panel_dsi_generic_send_seq(panel, 0xbd,
                   0x00);

    /* 5.19.16 SETECO (C6h) */
    panel_dsi_generic_send_seq(panel, 0xc6,
                   0xed);

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
    aic_delay_ms(5);

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

static struct display_timing hx8394_timing = {
    .pixelclock = 63000000,
    .hactive = 720,
    .hfront_porch = 100,
    .hback_porch = 20,
    .hsync_len = 20,
    .vactive = 1280,
    .vfront_porch = 15,
    .vback_porch = 12,
    .vsync_len = 2,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_hx8394 = {
    .name = "panel-hx8394",
    .timings = &hx8394_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
