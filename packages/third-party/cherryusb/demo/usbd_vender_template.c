/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "usbd_core.h"
#include "usbd_vender.h"

/*!< endpoint address */
#define IN_EP  0x81
#define OUT_EP 0x01

#define USBD_VID           0x33C3
#define USBD_PID           0x7788
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

/*!< config descriptor size */
#define USB_CONFIG_SIZE (9 + 9 + 7 + 7)

#ifdef CONFIG_USB_HS
#define MAX_MPS 512
#else
#define MAX_MPS 64
#endif

/*!< global descriptor */
static const uint8_t vender_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_1_1, 0xFF, 0x00, 0x00, USBD_VID, USBD_PID, 0x0001, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x1, 0x01, 0xC0, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, 0x00, 0x00, 0x01),
    USB_ENDPOINT_DESCRIPTOR_INIT(OUT_EP, USB_ENDPOINT_TYPE_BULK, MAX_MPS, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(IN_EP, USB_ENDPOINT_TYPE_BULK, MAX_MPS, 0x00),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x2C,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'V', 0x00,                  /* wcChar10 */
    'E', 0x00,                  /* wcChar11 */
    'N', 0x00,                  /* wcChar12 */
    'D', 0x00,                  /* wcChar13 */
    'E', 0x00,                  /* wcChar14 */
    'R', 0x00,                  /* wcChar15 */
    ' ', 0x00,                  /* wcChar16 */
    'D', 0x00,                  /* wcChar17 */
    'E', 0x00,                  /* wcChar18 */
    'M', 0x00,                  /* wcChar19 */
    'O', 0x00,                  /* wcChar20 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '2', 0x00,                  /* wcChar3 */
    '1', 0x00,                  /* wcChar4 */
    '2', 0x00,                  /* wcChar5 */
    '3', 0x00,                  /* wcChar6 */
    '4', 0x00,                  /* wcChar7 */
    '5', 0x00,                  /* wcChar8 */
    '6', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x02,
    0x02,
    0x01,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[2048];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[2048];

volatile bool ep_tx_busy_flag = false;

#ifdef CONFIG_USB_HS
#define MAX_MPS 512
#else
#define MAX_MPS 64
#endif

void usbd_vender_out(uint8_t ep, uint32_t nbytes);
void usbd_vender_in(uint8_t ep, uint32_t nbytes);
/*!< endpoint call back */
struct usbd_endpoint vender_out_ep = {
    .ep_addr = OUT_EP,
    .ep_cb = usbd_vender_out
};

struct usbd_endpoint vender_in_ep = {
    .ep_addr = IN_EP,
    .ep_cb = usbd_vender_in
};

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
void usbd_comp_vender_event_handler(uint8_t event)
#else
void usbd_event_handler(uint8_t event)
#endif
{
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            /* setup first out ep read transfer */
            usbd_ep_start_read(vender_out_ep.ep_addr , read_buffer, 2048);
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void usbd_vender_out(uint8_t ep, uint32_t nbytes)
{
    USB_LOG_INFO("vender out data %#x %d\n", ep, nbytes);

    /* setup next out ep read transfer */
    usbd_ep_start_read(vender_out_ep.ep_addr , read_buffer, 2048);
}

void usbd_vender_in(uint8_t ep, uint32_t nbytes)
{
    USB_LOG_INFO("vender in data %#x %d\n", ep, nbytes);
}

static struct usbd_interface intf0;

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
#include "composite_template.h"
int usbd_comp_vender_init(uint8_t *ep_table, void *data)
{
    vender_out_ep.ep_addr = ep_table[0];
    vender_in_ep.ep_addr = ep_table[1];
    usbd_add_interface(usbd_vender_init_intf(&intf0));
    usbd_add_endpoint(&vender_out_ep);
    usbd_add_endpoint(&vender_in_ep);
    return 0;
}
#endif

int vender_device_init(void)
{
    const uint8_t data[10] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30 };

    memcpy(&write_buffer[0], data, 10);
    memset(&write_buffer[10], 'a', 2038);

#ifndef LPKG_CHERRYUSB_DEVICE_COMPOSITE
    usbd_desc_register(vender_descriptor);
    usbd_add_interface(usbd_vender_init_intf(&intf0));
    usbd_add_endpoint(&vender_out_ep);
    usbd_add_endpoint(&vender_in_ep);
    usbd_initialize();
#else
    usbd_comp_func_register(vender_descriptor,
                            usbd_comp_vender_event_handler,
                            usbd_comp_vender_init, "vender");
#endif
    return 0;
}
INIT_DEVICE_EXPORT(vender_device_init);

static int cmd_test_vender(int argc, char **argv)
{
    memset(write_buffer, 'a', 2038);
    usbd_ep_start_write(vender_in_ep.ep_addr , write_buffer, 192);
    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_vender, test_vender_usb, Test CMU CLK);
