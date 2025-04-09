/*
 * Copyright (c) 2022-2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usbd_cdc.h"

/*!< endpoint address */
#define CDC_IN_EP(x)  (0x81+(x) * 2)
#define CDC_OUT_EP(x) (0x1+(x) * 2)
#define CDC_INT_EP(x) (0x82+(x) * 2)

#define USBD_VID           0x33C3
#define USBD_PID           0xFFFF
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033


#define ACM_TEST_NUM       7
/*!< config descriptor size */
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN * ACM_TEST_NUM)

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

uint8_t test[] = {
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP(1), CDC_OUT_EP(1), CDC_IN_EP(1), CDC_MAX_MPS, 0x02)
};
/*!< global descriptor */
static const uint8_t cdc_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, ACM_TEST_NUM * 2, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP(0), CDC_OUT_EP(0), CDC_IN_EP(0), CDC_MAX_MPS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x02, CDC_INT_EP(1), CDC_OUT_EP(1), CDC_IN_EP(1), CDC_MAX_MPS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x04, CDC_INT_EP(2), CDC_OUT_EP(2), CDC_IN_EP(2), CDC_MAX_MPS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x06, CDC_INT_EP(3), CDC_OUT_EP(3), CDC_IN_EP(3), CDC_MAX_MPS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x08, CDC_INT_EP(4), CDC_OUT_EP(4), CDC_IN_EP(4), CDC_MAX_MPS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x0a, CDC_INT_EP(5), CDC_OUT_EP(5), CDC_IN_EP(5), CDC_MAX_MPS, 0x02),
    CDC_ACM_DESCRIPTOR_INIT(0x0c, CDC_INT_EP(6), CDC_OUT_EP(6), CDC_IN_EP(6), CDC_MAX_MPS, 0x02),
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
    0x26,                       /* bLength */
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
    'C', 0x00,                  /* wcChar10 */
    'D', 0x00,                  /* wcChar11 */
    'C', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
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

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[ACM_TEST_NUM][2048];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[ACM_TEST_NUM][2048];

volatile bool ep_tx_busy_flag = false;

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

void usbd_event_handler(uint8_t event)
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
            for (int i = 1; i <= ACM_TEST_NUM; i++) {
                usbd_ep_start_read(i, read_buffer[i - 1], 2048);
            }
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void usbd_cdc_acm_bulk_out(uint8_t ep, uint32_t nbytes)
{
    memcpy(write_buffer[(ep & 0x0f) - 1], read_buffer[ep - 1], nbytes);
    usbd_ep_start_write(ep | 0x80, write_buffer[(ep & 0x0f) - 1], nbytes);

    /* setup next out ep read transfer */
    usbd_ep_start_read(ep, read_buffer[ep - 1], 2048);
}

void usbd_cdc_acm_bulk_in(uint8_t ep, uint32_t nbytes)
{

}

struct usbd_endpoint cdc_out_ep[ACM_TEST_NUM];
struct usbd_endpoint cdc_in_ep[ACM_TEST_NUM];
struct usbd_interface intf0[ACM_TEST_NUM];
struct usbd_interface intf1[ACM_TEST_NUM];

int cdc_acm_multi_init(void)
{
    usbd_desc_register(cdc_descriptor);
    for (int i = 0; i < ACM_TEST_NUM; i++) {

        cdc_out_ep[i].ep_addr = 0x1 + i * 2;
        cdc_out_ep[i].ep_cb = usbd_cdc_acm_bulk_out;

        cdc_in_ep[i].ep_addr = 0x81 + i * 2;
        cdc_in_ep[i].ep_cb = usbd_cdc_acm_bulk_in;

        usbd_add_interface(usbd_cdc_acm_init_intf(&intf0[i]));
        usbd_add_interface(usbd_cdc_acm_init_intf(&intf1[i]));
        usbd_add_endpoint(&cdc_out_ep[i]);
        usbd_add_endpoint(&cdc_in_ep[i]);
    }

    usbd_initialize();
    return 0;
}

INIT_APP_EXPORT(cdc_acm_multi_init);
