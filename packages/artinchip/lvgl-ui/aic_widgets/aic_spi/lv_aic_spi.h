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

typedef void (*ui_init_cb)(void);

typedef struct {
    struct mpp_fb *fb;
    struct aicfb_screeninfo info;

    void *buf1;
    void *buf2;

    lv_display_t *disp;
} spi_disp_t;

struct lv_spi_dev {
    const char * bus_name;
    unsigned int mode;
    unsigned int max_hz;

    unsigned int spi_type;
    unsigned int use_spi_hardware;

    void *dev;
    unsigned int rs_pin;
    unsigned int bl_pin;
    unsigned char *data;
    unsigned int len;

    unsigned char *tx_buf;
    unsigned int tx_len;
    struct mpp_ge *ge2d_dev;
    lv_display_t *disp;

    aicos_sem_t display_sem;
    aicos_wqueue_t te_queue;
    unsigned int has_send_data;
    unsigned int frame_count;

    struct aicfb_screeninfo info;
};

void lv_spi_flush(u8 *data, unsigned int len);

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
void lv_spi_screen_enable(lv_display_t *disp);
