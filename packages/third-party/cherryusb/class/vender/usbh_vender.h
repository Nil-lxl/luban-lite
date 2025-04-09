/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _USBH_VENDER_H
#define _USBH_VENDER_H

#include "usb_vender.h"

struct usbh_vender {
    struct usbh_hubport *hport;
    struct usb_endpoint_descriptor *vender_in;
    struct usb_endpoint_descriptor *vender_out;
    struct usbh_urb vender_in_urb;
    struct usbh_urb vender_out_urb;

    uint8_t intf; /* interface number */
    uint8_t minor;
};

void usbh_vender_run(struct usbh_vender *vender_class);
void usbh_vender_stop(struct usbh_vender *vender_class);

#endif
