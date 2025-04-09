/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usbd_midi.h"

static int midi_class_interface_request_handler(struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    USB_LOG_INFO("midi_class_interface_request_handler\r\n");
    return 0;
}

static void midi_notify_handler(uint8_t event, void *arg)
{
    switch (event) {
    case USBD_EVENT_RESET:
        break;

    case USBD_EVENT_CONFIGURED:
        usbd_midi_open();
        break;
    default:
        break;
    }
}

struct usbd_interface *usbd_midi_init_intf(struct usbd_interface *intf)
{

    intf->class_interface_handler = midi_class_interface_request_handler;
    intf->class_endpoint_handler = NULL;
    intf->vendor_handler = NULL;
    intf->notify_handler = midi_notify_handler;

    return intf;
}
