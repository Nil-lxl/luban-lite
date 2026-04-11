/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef USBD_DISPLAY_H
#define USBD_DISPLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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


#ifdef __cplusplus
}
#endif

#endif /* USBD_DISPLAY_H */
