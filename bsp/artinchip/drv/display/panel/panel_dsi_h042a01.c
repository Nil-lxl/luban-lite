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

    panel_dsi_generic_send_seq(panel, 0xee, 0x01);
    panel_dsi_generic_send_seq(panel, 0xea, 0x07);
    panel_dsi_generic_send_seq(panel, 0xeb, 0x12);
    panel_dsi_generic_send_seq(panel, 0x05, 0x19);
    panel_dsi_generic_send_seq(panel, 0x0a, 0x8a); //vcom  8b
    panel_dsi_generic_send_seq(panel, 0x14, 0x58);
    panel_dsi_generic_send_seq(panel, 0x15, 0x58);  //
    panel_dsi_generic_send_seq(panel, 0x17, 0x32);//VDDD
    panel_dsi_generic_send_seq(panel, 0x21, 0x01);
    panel_dsi_generic_send_seq(panel, 0x22, 0x01);
    panel_dsi_generic_send_seq(panel, 0x23, 0x11);
    panel_dsi_generic_send_seq(panel, 0x28, 0x1F);// vgh  14  
    panel_dsi_generic_send_seq(panel, 0x29, 0x29); // vgl -14
    panel_dsi_generic_send_seq(panel, 0x2a, 0x63);
    panel_dsi_generic_send_seq(panel, 0x2f, 0xf3);  //


    //////////////////////////  page2 ////////////////////////////
    panel_dsi_generic_send_seq(panel, 0xee, 0x02);
    panel_dsi_generic_send_seq(panel, 0x39, 0x70);//b0  70=4.4V

    //G2.2  2025.9.15 
    panel_dsi_generic_send_seq(panel, 0x00, 0x00);//0
    panel_dsi_generic_send_seq(panel, 0x01, 0x0D);//4
    panel_dsi_generic_send_seq(panel, 0x02, 0x14);//8
    panel_dsi_generic_send_seq(panel, 0x03, 0x09);//12 
    panel_dsi_generic_send_seq(panel, 0x04, 0x10);//28
    panel_dsi_generic_send_seq(panel, 0x05, 0x4C);//52
    panel_dsi_generic_send_seq(panel, 0x06, 0x0B);//76
    panel_dsi_generic_send_seq(panel, 0x07, 0x11);//100
    panel_dsi_generic_send_seq(panel, 0x08, 0x0E);//131
    panel_dsi_generic_send_seq(panel, 0x09, 0x0D);//155 
    panel_dsi_generic_send_seq(panel, 0x0A, 0x11);//179 
    panel_dsi_generic_send_seq(panel, 0x0b, 0x5F);//203  
    panel_dsi_generic_send_seq(panel, 0x0c, 0x14);//227
    panel_dsi_generic_send_seq(panel, 0x0d, 0x18);//243
    panel_dsi_generic_send_seq(panel, 0x0e, 0x3E);//247
    panel_dsi_generic_send_seq(panel, 0x0f, 0x3D);//251 
    panel_dsi_generic_send_seq(panel, 0x10, 0x3F);//255

    panel_dsi_generic_send_seq(panel, 0x20, 0x00);//0
    panel_dsi_generic_send_seq(panel, 0x21, 0x0B);//4
    panel_dsi_generic_send_seq(panel, 0x22, 0x12);//8
    panel_dsi_generic_send_seq(panel, 0x23, 0x03);//12
    panel_dsi_generic_send_seq(panel, 0x24, 0x08);//28
    panel_dsi_generic_send_seq(panel, 0x25, 0x40);//52  
    panel_dsi_generic_send_seq(panel, 0x26, 0x0B);//76  
    panel_dsi_generic_send_seq(panel, 0x27, 0x09);//100
    panel_dsi_generic_send_seq(panel, 0x28, 0x0E);//131  
    panel_dsi_generic_send_seq(panel, 0x29, 0x0D);//155
    panel_dsi_generic_send_seq(panel, 0x2A, 0x11);//179
    panel_dsi_generic_send_seq(panel, 0x2b, 0x5F);//203 
    panel_dsi_generic_send_seq(panel, 0x2c, 0x14);//227
    panel_dsi_generic_send_seq(panel, 0x2d, 0x18);//243
    panel_dsi_generic_send_seq(panel, 0x2e, 0x3E);//247
    panel_dsi_generic_send_seq(panel, 0x2f, 0x3D);//251
    panel_dsi_generic_send_seq(panel, 0x30, 0x3F);//255



    //////////////////////////  page4 ////////////////////////////
    panel_dsi_generic_send_seq(panel, 0xee, 0x04);
    panel_dsi_generic_send_seq(panel, 0x00, 0x01);
    panel_dsi_generic_send_seq(panel, 0x01, 0x01); //960
    panel_dsi_generic_send_seq(panel, 0x02, 0xe0);
    panel_dsi_generic_send_seq(panel, 0x03, 0x05);  //1280
    panel_dsi_generic_send_seq(panel, 0x04, 0x00);
    panel_dsi_generic_send_seq(panel, 0x06, 0x14);
    panel_dsi_generic_send_seq(panel, 0x07, 0x05); //sstp
    panel_dsi_generic_send_seq(panel, 0x08, 0x12);
    panel_dsi_generic_send_seq(panel, 0x09, 0x20);
    panel_dsi_generic_send_seq(panel, 0x0a, 0x0f);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x04);  //0: 1dot 
    panel_dsi_generic_send_seq(panel, 0x20, 0x40);
    panel_dsi_generic_send_seq(panel, 0x2a, 0x00);
    panel_dsi_generic_send_seq(panel, 0x40, 0x80);
    panel_dsi_generic_send_seq(panel, 0x41, 0x60);


    //////////////////////////  page5 ////////////////////////////
    panel_dsi_generic_send_seq(panel, 0xee, 0x05);
    panel_dsi_generic_send_seq(panel, 0x00, 0x05);
    panel_dsi_generic_send_seq(panel, 0x01, 0x09);
    panel_dsi_generic_send_seq(panel, 0x02, 0x05);
    panel_dsi_generic_send_seq(panel, 0x03, 0x05);


    panel_dsi_generic_send_seq(panel, 0x07, 0x01); //stv1//
    panel_dsi_generic_send_seq(panel, 0x08, 0x05);
    panel_dsi_generic_send_seq(panel, 0x09, 0x00);


    panel_dsi_generic_send_seq(panel, 0x10, 0x08);
    panel_dsi_generic_send_seq(panel, 0x11, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x12, 0x05);
    panel_dsi_generic_send_seq(panel, 0x13, 0x05);
    panel_dsi_generic_send_seq(panel, 0x19, 0x90); //1936
    panel_dsi_generic_send_seq(panel, 0x1a, 0x77);
    panel_dsi_generic_send_seq(panel, 0x23, 0x00);//

    panel_dsi_generic_send_seq(panel, 0x30, 0x01);
    panel_dsi_generic_send_seq(panel, 0x31, 0x01);
    panel_dsi_generic_send_seq(panel, 0x32, 0x00);
    panel_dsi_generic_send_seq(panel, 0x33, 0x14);
    panel_dsi_generic_send_seq(panel, 0x34, 0x14);
    panel_dsi_generic_send_seq(panel, 0x35, 0xb4);

    panel_dsi_generic_send_seq(panel, 0x36, 0x01);
    panel_dsi_generic_send_seq(panel, 0x37, 0x01);
    panel_dsi_generic_send_seq(panel, 0x38, 0x00);
    panel_dsi_generic_send_seq(panel, 0x39, 0x14);
    panel_dsi_generic_send_seq(panel, 0x3a, 0x14);
    panel_dsi_generic_send_seq(panel, 0x40, 0x00);
    panel_dsi_generic_send_seq(panel, 0x41, 0x00);
    panel_dsi_generic_send_seq(panel, 0x43, 0x11);
    panel_dsi_generic_send_seq(panel, 0x44, 0x01);
    panel_dsi_generic_send_seq(panel, 0x45, 0x81);
    panel_dsi_generic_send_seq(panel, 0x46, 0x06);
    panel_dsi_generic_send_seq(panel, 0x47, 0x03);


    //////////////////////////  page6 ////////////////////////////
    panel_dsi_generic_send_seq(panel, 0xee, 0x06);     //backward
    panel_dsi_generic_send_seq(panel, 0x00, 0x23);
    panel_dsi_generic_send_seq(panel, 0x01, 0x01);
    panel_dsi_generic_send_seq(panel, 0x02, 0x04);
    panel_dsi_generic_send_seq(panel, 0x06, 0xcd);
    panel_dsi_generic_send_seq(panel, 0x08, 0x67);
    panel_dsi_generic_send_seq(panel, 0x09, 0x45);
    panel_dsi_generic_send_seq(panel, 0x0a, 0x23);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x01);


    //////////////////////////  page7 ////////////////////////////
    panel_dsi_generic_send_seq(panel, 0xee, 0x07); //PAGE7
    //GIP LEFT 1-22  
    panel_dsi_generic_send_seq(panel, 0x00, 0x14);
    panel_dsi_generic_send_seq(panel, 0x01, 0x14);
    panel_dsi_generic_send_seq(panel, 0x02, 0x16);
    panel_dsi_generic_send_seq(panel, 0x03, 0x16);
    panel_dsi_generic_send_seq(panel, 0x04, 0x10);
    panel_dsi_generic_send_seq(panel, 0x05, 0x10);
    panel_dsi_generic_send_seq(panel, 0x06, 0x12);
    panel_dsi_generic_send_seq(panel, 0x07, 0x12);
    panel_dsi_generic_send_seq(panel, 0x08, 0x0d);
    panel_dsi_generic_send_seq(panel, 0x09, 0x0d);
    panel_dsi_generic_send_seq(panel, 0x0A, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0B, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0C, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x0D, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x0E, 0x04);
    panel_dsi_generic_send_seq(panel, 0x0F, 0x04);
    panel_dsi_generic_send_seq(panel, 0x10, 0x3c);
    panel_dsi_generic_send_seq(panel, 0x11, 0x3c);
    panel_dsi_generic_send_seq(panel, 0x12, 0x20);
    panel_dsi_generic_send_seq(panel, 0x13, 0x20);
    panel_dsi_generic_send_seq(panel, 0x14, 0x21);
    panel_dsi_generic_send_seq(panel, 0x15, 0x21);

    //GIP RIGHT 1-22 
    panel_dsi_generic_send_seq(panel, 0x20, 0x15);
    panel_dsi_generic_send_seq(panel, 0x21, 0x15);
    panel_dsi_generic_send_seq(panel, 0x22, 0x17);
    panel_dsi_generic_send_seq(panel, 0x23, 0x17);
    panel_dsi_generic_send_seq(panel, 0x24, 0x11);
    panel_dsi_generic_send_seq(panel, 0x25, 0x11);
    panel_dsi_generic_send_seq(panel, 0x26, 0x13);
    panel_dsi_generic_send_seq(panel, 0x27, 0x13);
    panel_dsi_generic_send_seq(panel, 0x28, 0x0d);
    panel_dsi_generic_send_seq(panel, 0x29, 0x0d);
    panel_dsi_generic_send_seq(panel, 0x2A, 0x01);
    panel_dsi_generic_send_seq(panel, 0x2b, 0x01);
    panel_dsi_generic_send_seq(panel, 0x2c, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x2d, 0x0c);
    panel_dsi_generic_send_seq(panel, 0x2e, 0x04);
    panel_dsi_generic_send_seq(panel, 0x2F, 0x04);
    panel_dsi_generic_send_seq(panel, 0x30, 0x3c);
    panel_dsi_generic_send_seq(panel, 0x31, 0x3c);
    panel_dsi_generic_send_seq(panel, 0x32, 0x20);
    panel_dsi_generic_send_seq(panel, 0x33, 0x20);
    panel_dsi_generic_send_seq(panel, 0x34, 0x21);
    panel_dsi_generic_send_seq(panel, 0x35, 0x21);

    //////////////////////////  page8 ////////////////////////////
    panel_dsi_generic_send_seq(panel, 0xee, 0x08); //PAGE8
    panel_dsi_generic_send_seq(panel, 0x12, 0xdA); //  vddl vddh  df
    panel_dsi_generic_send_seq(panel, 0x13, 0x9b);  // eq RT   fb
    panel_dsi_generic_send_seq(panel, 0x18, 0x00); // 00
    panel_dsi_generic_send_seq(panel, 0x20, 0x00);
    panel_dsi_generic_send_seq(panel, 0x2c, 0x20);
    panel_dsi_generic_send_seq(panel, 0x4b, 0xa0);
    panel_dsi_generic_send_seq(panel, 0x61, 0x20);

    //////////////////////////  pagef ////////////////////////////
    panel_dsi_generic_send_seq(panel, 0xee, 0x0f);
    panel_dsi_generic_send_seq(panel, 0x00, 0x01);

    //////////////////////////  page0 ////////////////////////////
    panel_dsi_generic_send_seq(panel, 0xee, 0x00);
    panel_dsi_generic_send_seq(panel, 0xea, 0x00);
    panel_dsi_generic_send_seq(panel, 0xeb, 0x00);
    panel_dsi_generic_send_seq(panel, 0x36, 0x00);

    panel_dsi_generic_send_seq(panel, 0x11, 0x00);
    aic_delay_ms(60);
    panel_dsi_generic_send_seq(panel, 0x29, 0x00);
    aic_delay_ms(50);

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

static struct display_timing h042a01_timing = {
    .pixelclock = 84 * 1000 * 1000,
    .hactive = 1280,
    .hfront_porch = 60,
    .hback_porch = 30,
    .hsync_len = 30,
    .vactive = 960,
    .vfront_porch = 16,
    .vback_porch = 8,
    .vsync_len = 8,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_h042a01 = {
    .name = "panel-h042a01",
    .timings = &h042a01_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

