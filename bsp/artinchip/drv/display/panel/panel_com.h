/*
 * Copyright (C) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PANEL_COM_H_
#define _PANEL_COM_H_

#include "../drv_fb.h"

struct gpio_desc {
    unsigned int g;
    unsigned int p;
};

struct aic_panel_callbacks;

struct aic_panel *aic_find_panel(u32 connector_type);

extern struct aic_panel aic_panel_rgb;
extern struct aic_panel aic_panel_lvds;

/*
 * MIPI-DSI Panel
 */
extern struct aic_panel dsi_simple;
extern struct aic_panel dsi_xm91080;
extern struct aic_panel dsi_st7797;
extern struct aic_panel dsi_st7701s;
extern struct aic_panel dsi_st7703;
extern struct aic_panel dsi_st77922;
extern struct aic_panel dsi_ili9881c;
extern struct aic_panel dsi_hx8394;
extern struct aic_panel dsi_jd9365;
extern struct aic_panel dsi_axs15231b;
extern struct aic_panel dsi_nv3051;
extern struct aic_panel dsi_fl7707;
extern struct aic_panel dsi_ft8201;
extern struct aic_panel dsi_edp_lt9811exb;
extern struct aic_panel dsi_h013a08;
extern struct aic_panel dsi_h016a01;
extern struct aic_panel dsi_h024b12;
extern struct aic_panel dsi_h028b23;
extern struct aic_panel dsi_h028a29;
extern struct aic_panel dsi_h030b07;
extern struct aic_panel dsi_h034a01;
extern struct aic_panel dsi_h035a27;
extern struct aic_panel dsi_h035b16;
extern struct aic_panel dsi_h035b22;
extern struct aic_panel dsi_h043a8;
extern struct aic_panel dsi_h040b24;
extern struct aic_panel dsi_h050a20;
extern struct aic_panel dsi_h055a03;
extern struct aic_panel dsi_h070b13;
extern struct aic_panel dsi_h070a21;
extern struct aic_panel dsi_h078b06;
extern struct aic_panel dsi_h080a11;
extern struct aic_panel dsi_h080d16;
extern struct aic_panel dsi_h103w01;


/*
 * MIPI-DBI Type B I8080 Panel
 */
extern struct aic_panel dbi_ili9488;
extern struct aic_panel dbi_ili9486l;
extern struct aic_panel dbi_st7789;
extern struct aic_panel dbi_h040a12;
extern struct aic_panel dbi_h040a27;

/*
 * MIPI-DBI Type C SPI Panel
 */
extern struct aic_panel dbi_ili9341;
extern struct aic_panel dbi_st77903;
extern struct aic_panel dbi_st7789t3;
extern struct aic_panel dbi_st7789v;
extern struct aic_panel dbi_ili9327;
extern struct aic_panel dbi_co5300;
extern struct aic_panel dbi_st77916;
extern struct aic_panel dbi_gc9108;
extern struct aic_panel dbi_st77912;
extern struct aic_panel spi_general;

/*
 * RGB Panel SPI Init
 */
extern struct aic_panel rgb_st7701s;
extern struct aic_panel rgb_nv3052;
extern struct aic_panel rgb_st7703;
extern struct aic_panel rgb_gc9a01a;
extern struct aic_panel rgb_nt35560;
extern struct aic_panel rgb_st77922;
extern struct aic_panel rgb_st77916;
extern struct aic_panel rgb_jd9855;
extern struct aic_panel rgb_h024a13;
extern struct aic_panel rgb_h028a19;
extern struct aic_panel rgb_h030a8;
extern struct aic_panel rgb_h030a10;
extern struct aic_panel rgb_h034a02;
extern struct aic_panel rgb_h035a17;
extern struct aic_panel rgb_h035a24;
extern struct aic_panel rgb_h040a18;
extern struct aic_panel rgb_h048a02;
extern struct aic_panel rgb_h043a7;
extern struct aic_panel rgb_h043b32;
extern struct aic_panel rgb_h050a12;
extern struct aic_panel rgb_h050a18;
extern struct aic_panel rgb_h052a01;
extern struct aic_panel rgb_h062a01;
extern struct aic_panel rgb_h068a04;
extern struct aic_panel rgb_h068b06;
extern struct aic_panel rgb_h070a18;

/*
 * SRGB Panel
 */
extern struct aic_panel srgb_hx8238;

/*
 * Bridge Panel
 */
extern struct aic_panel bridge_lt8911;
extern struct aic_panel lcos_hx7033;

void panel_di_enable(struct aic_panel *panel, u32 ms);
void panel_di_disable(struct aic_panel *panel, u32 ms);
void panel_de_timing_enable(struct aic_panel *panel, u32 ms);
void panel_de_timing_disable(struct aic_panel *panel, u32 ms);

int panel_default_prepare(void);
int panel_default_unprepare(void);
int panel_default_enable(struct aic_panel *panel);
int panel_default_disable(struct aic_panel *panel);
int panel_register_callback(struct aic_panel *panel,
                struct aic_panel_callbacks *pcallback);

void panel_backlight_enable(struct aic_panel *panel, u32 ms);

void panel_backlight_disable(struct aic_panel *panel, u32 ms);

void panel_send_command(u8 *para_cmd, u32 size, struct aic_panel *panel);

void panel_get_gpio(struct gpio_desc *desc, char *name);

void panel_gpio_set_value(struct gpio_desc *desc, u32 value);

#ifdef AIC_PANEL_SPI_EMULATION
struct panel_spi_device {
    struct gpio_desc cs;
    struct gpio_desc sdi;
    struct gpio_desc scl;
    struct gpio_desc dc;
};

void panel_spi_data_wr(u8 data);
void panel_spi_cmd_wr(u8 cmd);
#ifndef AIC_SPI_EMULATION_WITH_DC
void panel_spi_device_emulation(char *cs, char *sdi, char *scl);
#else
void panel_spi_device_emulation(char *cs, char *sdi, char *scl, char *dc);
#endif

#define panel_spi_send_seq(command, seq...) do {                    \
        static const u8 d[] = { seq };                              \
        int i;                                                      \
        panel_spi_cmd_wr(command);                                  \
        for (i = 0; i < ARRAY_SIZE(d); i++)                         \
            panel_spi_data_wr(d[i]);                                \
    } while (0)
#endif

#endif /* _PANEL_COM_H_ */

