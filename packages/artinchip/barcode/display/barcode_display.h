/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#ifndef BARCODE_DISPLAY_H
#define BARCODE_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config/barcode_config.h"

#ifdef BARCODE_ENABLE_DISPLAY

#include "mpp_fb.h"
#include "artinchip_fb.h"
#ifdef BARCODE_DISPLAY_ROTATION
#include "mpp_ge.h"
#endif

typedef struct {
    struct mpp_fb *fb;
    struct aicfb_screeninfo fb_info;
#ifdef BARCODE_DISPLAY_ROTATION
    struct mpp_ge *ge_dev;
    unsigned char *ge_out_buffer;
    int rotation;
#endif
    bool initialized;
} barcode_display_t;

/**
 * @brief Initialize display module
 * @param disp Display handle
 * @return 0 on success, negative value on failure
 */
int barcode_display_init(barcode_display_t *disp);

/**
 * @brief Deinitialize display module
 * @param disp Display handle
 */
void barcode_display_deinit(barcode_display_t *disp);

/**
 * @brief Update video display layer
 * @param disp Display handle
 * @param width Video width
 * @param height Video height
 * @param phy_addr_y Y component physical address
 * @param phy_addr_uv UV component physical address
 * @return 0 on success, negative value on failure
 */
int barcode_display_update_video(barcode_display_t *disp, 
                                  int width, int height, 
                                  unsigned long phy_addr_y, 
                                  unsigned long phy_addr_uv);

/**
 * @brief Display text on LCD
 * @param disp Display handle
 * @param text Text content
 * @param x X coordinate
 * @param y Y coordinate
 * @return 0 on success, negative value on failure
 */
int barcode_display_show_text(barcode_display_t *disp, 
                              const char *text, int x, int y);

/**
 * @brief Set UI layer transparency
 * @param disp Display handle
 * @param alpha Transparency value (0-255)
 * @return 0 on success, negative value on failure
 */
int barcode_display_set_ui_alpha(barcode_display_t *disp, int alpha);

#else

// Empty implementation when display is disabled
typedef struct { 
    int dummy; 
} barcode_display_t;

static inline int barcode_display_init(barcode_display_t *disp) { 
    (void)disp;
    return 0; 
}

static inline void barcode_display_deinit(barcode_display_t *disp) {
    (void)disp;
}

static inline int barcode_display_update_video(barcode_display_t *disp, 
                                                int width, int height, 
                                                unsigned long phy_addr_y, 
                                                unsigned long phy_addr_uv) {
    (void)disp;
    (void)width;
    (void)height;
    (void)phy_addr_y;
    (void)phy_addr_uv;
    return 0;
}

static inline int barcode_display_show_text(barcode_display_t *disp, 
                                            const char *text, int x, int y) {
    (void)disp;
    (void)text;
    (void)x;
    (void)y;
    return 0;
}

static inline int barcode_display_set_ui_alpha(barcode_display_t *disp, int alpha) {
    (void)disp;
    (void)alpha;
    return 0;
}

#endif /* BARCODE_ENABLE_DISPLAY */

#ifdef __cplusplus
}
#endif

#endif /* BARCODE_DISPLAY_H */
