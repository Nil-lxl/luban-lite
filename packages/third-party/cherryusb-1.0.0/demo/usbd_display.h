/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef USBD_DISPLAY_H
#define USBD_DISPLAY_H

#include <stdint.h>
#include "mpp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief enable usb display display.
 * @param none.
 * @return none.
 */
void usb_display_enable(void);

/**
 * @brief disable usb display display.
 * @param none.
 * @return none.
 */
void usb_display_disable(void);

/**
 * @brief set usb display rotate.
 * @param rotate_angle is clockwise rotation angle, 0|90|180|270.
 * @return on success will return 0, and others indicate fail.
 */
int usb_display_set_rotate(unsigned int rotate_angle);

/**
 * @brief get usb display rotate.
 * @return rotate_angle is clockwise rotation angle, 0|90|180|270..
 */
int usb_display_get_rotate();

/**
 * @brief enable usb display display rect.
 * @param enable is enable display rect.
 * @return on success will return 0, and others indicate fail.
 */
int usb_display_rect_enable(bool enable);

/**
 * @brief set usb display display rect.
 * @param rect is display rect info.
 * @return on success will return 0, and others indicate fail.
 */
int usb_display_rect_set(struct mpp_rect *rect);

/**
 * @brief set usb display display rect.
 * @param rect is display rect info.
 * @return on success will return 0, and others indicate fail.
 */
int usb_display_rect_get(struct mpp_rect *rect);
#ifdef __cplusplus
}
#endif

#endif /* USBD_DISPLAY_H */
