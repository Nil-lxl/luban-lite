/*
 * Copyright (C) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui <huahui.mai@artinchip.com>
 */

#include <string.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <drv_qspi.h>

#include "lvgl.h"
#include <mpp_fb.h>
#include <mpp_ge.h>

#include "aic_spi_lcd.h"

#if defined(LV_SPI_ST77916)
#define  LV_LCD_WIDTH    360
#define  LV_LCD_HEIGHT   360
#elif defined(LV_SPI_ST7789)
#define  LV_LCD_WIDTH    320
#define  LV_LCD_HEIGHT   320
#elif defined(LV_SPI_ST77912)
#define  LV_LCD_WIDTH    240
#define  LV_LCD_HEIGHT   240
#elif defined(LV_SPI_GC9D01N)
#define  LV_LCD_WIDTH    160
#define  LV_LCD_HEIGHT   160
#else
#define  LV_LCD_WIDTH    0
#define  LV_LCD_HEIGHT   0
#endif

struct lv_spi_config {
    const aic_spi_lcd_cfg_t spi_cfg;
    const char *bl_pin;
    const char *te_pin;
    bool bl_low_active;
    unsigned int width;
    unsigned int height;
};

struct lv_spi_dev {
    const struct lv_spi_config *config;
    aic_spi_lcd_dev_t *dev;

    /* LVGL Draw Buffer */
    unsigned char *data;
    unsigned int fb_size;

    /* SPI Transfer Buffer */
    unsigned char *tx_buf;
    unsigned int tx_len;
    struct mpp_ge *ge2d_dev;

    int te_pin;
    aicos_wqueue_t te_queue;
    aicos_sem_t display_sem;
    aicos_sem_t sync_ready;
    unsigned int frame_count;

    bool bl_en;
    bool power_on;

    lv_display_t *disp;
    unsigned int id;
};

void lv_spi_flush(lv_display_t *disp, u8 *data);

void lv_spi_write_buffer(struct lv_spi_dev *spi_dev,
                         unsigned int cmd, unsigned int len, const u8 *data);

int lv_spi_display_init(int use_frame_buffer);

/*
 * defined in the spi tft controller dirver lv_xxx.c
 */
void lv_spi_panel_enable(struct lv_spi_dev *dev);

#define lv_spi_write_seq(dev, cmd, seq...)                  \
    do {                                                    \
        static const u8 d[] = { seq };                      \
        lv_spi_write_buffer(dev, cmd, ARRAY_SIZE(d), d);    \
    } while(0);

/*
 * Init ArtInChip SoC SPI Controller and enable
 * lcd peripheral by calling lv_spi_panel_enable()
 */
void lv_spi_screen_enable(struct lv_spi_dev *spi_dev);

void lv_qspi_disp_mode_init(struct lv_spi_dev *spi_dev);
