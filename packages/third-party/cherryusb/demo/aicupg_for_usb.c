/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <aic_core.h>
#include <aic_utils.h>
#include <aicupg.h>
#include <usbd_core.h>
#include "usbd_aicupg.h"

#define USBD_VID           0x33C3
#define USBD_PID           0x6677

/*!< endpoint address */
#define BULK_OUT_EP  0x02
#define BULK_IN_EP   0x81

/*!< config descriptor size */
#define USB_CONFIG_SIZE (9 + 9 + 7 + 7)

#ifdef CONFIG_USB_HS
#define BULK_MAX_MPS 512
#else
#define BULK_MAX_MPS 64
#endif

/*!< global descriptor */
static const uint8_t usb_upg_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0, 0, 0, USBD_VID, USBD_PID, 0x101, 1),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 1, 1, USB_CONFIG_BUS_POWERED, 0xFA),
    USB_INTERFACE_DESCRIPTOR_INIT(0, 0, 2, 0xFF, 0xFF, 0xFF, 0),
    USB_ENDPOINT_DESCRIPTOR_INIT(BULK_IN_EP, USB_ENDPOINT_TYPE_BULK, BULK_MAX_MPS, 0),
    USB_ENDPOINT_DESCRIPTOR_INIT(BULK_OUT_EP, USB_ENDPOINT_TYPE_BULK, BULK_MAX_MPS, 0),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(0x0409),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'r', 0x00,                  /* wcChar1 */
    't', 0x00,                  /* wcChar2 */
    'i', 0x00,                  /* wcChar3 */
    'n', 0x00,                  /* wcChar4 */
    'c', 0x00,                  /* wcChar5 */
    'h', 0x00,                  /* wcChar6 */
    'i', 0x00,                  /* wcChar7 */
    'p', 0x00,                  /* wcChar8 */
    0x00, 0x00,                 /* wcChar9 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x24,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'r', 0x00,                  /* wcChar1 */
    't', 0x00,                  /* wcChar2 */
    'i', 0x00,                  /* wcChar3 */
    'n', 0x00,                  /* wcChar4 */
    'c', 0x00,                  /* wcChar5 */
    'h', 0x00,                  /* wcChar6 */
    'i', 0x00,                  /* wcChar7 */
    'p', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'D', 0x00,                  /* wcChar10 */
    'e', 0x00,                  /* wcChar11 */
    'v', 0x00,                  /* wcChar12 */
    'i', 0x00,                  /* wcChar13 */
    'c', 0x00,                  /* wcChar14 */
    'e', 0x00,                  /* wcChar15 */
    0x00, 0x00,                 /* wcChar16 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0xFF,
    0xFF,
    0xFF,
    0x40,
    0x01,
    0x00,
#endif
};

static struct usbd_interface intf0;
static struct usbd_interface intf1;

int usb_init(void)
{
    usbd_desc_register(usb_upg_descriptor);
    usbd_add_interface(usbd_aicupg_init_intf(&intf0, BULK_OUT_EP, BULK_IN_EP));
    usbd_add_interface(usbd_aicupg_init_intf(&intf1, BULK_OUT_EP, BULK_IN_EP));
    usbd_initialize();
    return 0;
}
