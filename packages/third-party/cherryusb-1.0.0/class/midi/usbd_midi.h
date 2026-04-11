/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xindong.hu@artinchip.com
 */

#ifndef USBD_MIDI_H
#define USBD_MIDI_H

#include "usb_midi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Init audio interface driver */
struct usbd_interface *usbd_midi_init_intf(struct usbd_interface *intf);
void usbd_midi_open(void);

#ifdef __cplusplus
}
#endif

#endif /* USBD_AUDIO_H */
