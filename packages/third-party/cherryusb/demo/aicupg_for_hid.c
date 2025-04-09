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
#include <data_trans_layer.h>
#include "usbd_core.h"
#include "usbd_hid.h"

#define USBD_VID 0x33C3
#define USBD_PID 0x8899

/*!< endpoint address */
#define HID_IN_EP       0x83
#define HID_IN_EP_SIZE  1024
#define HID_OUT_EP      0x04
#define HID_OUT_EP_SIZE 1024

/*!< config descriptor size */
#define USB_CONFIG_SIZE      (9 + 9 + 9 + 7 + 7)
#define HID_REPORT_DESC_SIZE 34

#ifdef CONFIG_USB_HS
#define HID_MAX_MPS 512
#else
#define HID_MAX_MPS 64
#endif

#define HID_1_1 0x0111

static const uint8_t hid_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0, 0, 0, USBD_VID, USBD_PID, 0x101, 1),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 1, 1, USB_CONFIG_BUS_POWERED, 0xFA),
    USB_INTERFACE_DESCRIPTOR_INIT(0, 0, 2, 0x03, 0x00, 0x00, 0),
    USB_HID_DESCRIPTOR_INIT(HID_1_1, 0, 1, 0x22, HID_REPORT_DESC_SIZE),
    USB_ENDPOINT_DESCRIPTOR_INIT(HID_IN_EP, 0x03, HID_IN_EP_SIZE, 0),
    USB_ENDPOINT_DESCRIPTOR_INIT(HID_OUT_EP, 0x03, HID_OUT_EP_SIZE, 0),

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
    0x2c,                       /* bLength */
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
    'H', 0x00,                  /* wcChar10 */
    'I', 0x00,                  /* wcChar11 */
    'D', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'e', 0x00,                  /* wcChar15 */
    'v', 0x00,                  /* wcChar16 */
    'i', 0x00,                  /* wcChar17 */
    'c', 0x00,                  /* wcChar18 */
    'e', 0x00,                  /* wcChar19 */
    0x00, 0x00,                 /* wcChar20 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
    ///////////////////////////////////////
    /// Other Speed Configuration Descriptor
    ///////////////////////////////////////
    0x09,
    USB_DESCRIPTOR_TYPE_OTHER_SPEED,
    WBVAL(USB_CONFIG_SIZE),
    0x01,
    0x01,
    0x00,
    USB_CONFIG_BUS_POWERED,
    USB_CONFIG_POWER_MA(0xFA),
    0x00
#endif
};

/*!< custom hid report descriptor */
static const uint8_t hid_report_desc[HID_REPORT_DESC_SIZE] = {
#ifdef CONFIG_USB_HS
    /* USER CODE BEGIN 0 */
    0x06, 0x00, 0xff, /* USAGE_PAGE (Vendor Defined Page 1) */
    0x09, 0x01,       /* USAGE (Vendor Usage 1) */
    0xa1, 0x01,       /* COLLECTION (Application) */

    0x09, 0x02,       /*   USAGE (Vendor Usage 1) */
    0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
    0x25, 0xff,       /*   LOGICAL_MAXIMUM (255) */
    0x75, 0x08,       /*   REPORT_SIZE (8) */
    0x96, 0x00, 0x04, /*   REPORT_COUNT (1024) */
    0x81, 0x02,       /*   INPUT (Data,Var,Abs) */
    ///* <___________________________________________________> */
    0x09, 0x03,       /*   USAGE (Vendor Usage 1) */
    0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
    0x25, 0xff,       /*   LOGICAL_MAXIMUM (255) */
    0x75, 0x08,       /*   REPORT_SIZE (8) */
    0x96, 0x00, 0x04, /*   REPORT_COUNT (1024) */
    0x91, 0x02,       /*   OUTPUT (Data,Var,Abs) */
    /* USER CODE END 0 */
    0xC0 /*     END_COLLECTION	             */
#else
    /* USER CODE BEGIN 0 */
    0x06, 0x00, 0xff, /* USAGE_PAGE (Vendor Defined Page 1) */
    0x09, 0x01,       /* USAGE (Vendor Usage 1) */
    0xa1, 0x01,       /* COLLECTION (Application) */

    0x09, 0x01,       /*   USAGE (Vendor Usage 1) */
    0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
    0x26, 0xff, 0x00, /*   LOGICAL_MAXIMUM (255) */
    0x95, 0x40,       /*   REPORT_COUNT (64) */
    0x75, 0x08,       /*   REPORT_SIZE (8) */
    0x81, 0x02,       /*   INPUT (Data,Var,Abs) */
    /* <___________________________________________________> */
    0x09, 0x01,       /*   USAGE (Vendor Usage 1) */
    0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
    0x26, 0xff, 0x00, /*   LOGICAL_MAXIMUM (255) */
    0x95, 0x40,   /*   REPORT_COUNT (64) */
    0x75, 0x08,       /*   REPORT_SIZE (8) */
    0x91, 0x02,       /*   OUTPUT (Data,Var,Abs) */
    /* USER CODE END 0 */
    0xC0 /*     END_COLLECTION	             */
#endif
};

#define TRANS_DATA_BUFF_MAX_SIZE (64 * 1024)

static struct aicupg_trans_info trans_info;

static uint32_t usbd_hid_ep_start_read(uint8_t *data, uint32_t data_len)
{
    trans_info.transfer_len = data_len;
    usbd_ep_start_read(HID_OUT_EP, data, HID_OUT_EP_SIZE);
    return HID_OUT_EP_SIZE;
}

static uint32_t usbd_hid_ep_start_write(uint8_t *data, uint32_t data_len)
{
    trans_info.transfer_len = data_len;
    usbd_ep_start_write(HID_IN_EP, data, HID_IN_EP_SIZE);
    return HID_IN_EP_SIZE;
}

static void usbd_hid_notify_handler(uint8_t event, void *arg)
{
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_CONFIGURED:
            /* setup first out ep read transfer */
            USB_LOG_DBG("Start reading aic cbw\r\n");
            if (!trans_info.trans_pkt_buf) {
                trans_info.trans_pkt_siz = TRANS_DATA_BUFF_MAX_SIZE;
                trans_info.trans_pkt_buf = aicupg_malloc_align(trans_info.trans_pkt_siz, CACHE_LINE_SIZE);
                if (!trans_info.trans_pkt_buf) {
                    pr_err("malloc trans pkt buf(%u) failed.\n", (u32)trans_info.trans_pkt_siz);
                    return;
                }
            }
            trans_info.stage = AICUPG_READ_CBW;
            trans_info.recv((uint8_t *)trans_info.trans_pkt_buf, USB_SIZEOF_AIC_CBW);
            break;

        default:
            break;
    }
}

static void usbd_hid_in_callback(uint8_t ep, uint32_t nbytes)
{
    USB_LOG_DBG("actual in len:%d\r\n", nbytes);
    data_trans_layer_in_proc(&trans_info);
}

static void usbd_hid_out_callback(uint8_t ep, uint32_t nbytes)
{
    USB_LOG_DBG("actual out len:%d\r\n", nbytes);
    data_trans_layer_out_proc(&trans_info);
}

static struct usbd_endpoint hid_in_ep = {
    .ep_cb = usbd_hid_in_callback,
    .ep_addr = HID_IN_EP
};

static struct usbd_endpoint hid_out_ep = {
    .ep_cb = usbd_hid_out_callback,
    .ep_addr = HID_OUT_EP
};

static struct usbd_interface intf0;

int hid_init(void)
{
    usbd_desc_register(hid_descriptor);
    usbd_add_interface(usbd_hid_init_intf(&intf0, hid_report_desc, HID_REPORT_DESC_SIZE));
    intf0.notify_handler = usbd_hid_notify_handler;

    usbd_add_endpoint(&hid_in_ep);
    usbd_add_endpoint(&hid_out_ep);

    memset((uint8_t *)&trans_info, 0, sizeof(struct aicupg_trans_info));

    trans_info.send = usbd_hid_ep_start_write;
    trans_info.recv = usbd_hid_ep_start_read;

    usbd_initialize();
    return 0;
}
