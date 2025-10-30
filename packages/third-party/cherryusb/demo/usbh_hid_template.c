/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Senye Liang <senye.liang@artinchip.com>
 */

#include <rtconfig.h>
#include "usbh_core.h"
#include "usbh_hid.h"

struct usbh_hid_func_t {
    uint8_t *buf;

    usb_osal_thread_t  hid_tid;
    struct usbh_hid *hid_class;

    uint8_t interval;
    uint8_t recv_flag;
}g_usbh_hid_func[CONFIG_USBHOST_MAX_HID_CLASS];

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t hid_buffer[CONFIG_USBHOST_MAX_HID_CLASS][128];

static struct usbh_hid_func_t *get_hid_func(struct usbh_hid *hid_class)
{
    if (hid_class->intf > CONFIG_USBHOST_MAX_HID_CLASS) {
        USB_LOG_ERR("USB Host does not support so many interface functions. ");
        return NULL;
    }

    g_usbh_hid_func[hid_class->intf].hid_class = hid_class;
    g_usbh_hid_func[hid_class->intf].buf = hid_buffer[hid_class->intf];

    return &g_usbh_hid_func[hid_class->intf];
}

void usbh_hid_callback(void *arg, int nbytes)
{
    struct usbh_hid *hid_class = (struct usbh_hid *)arg;
    struct usbh_hid_func_t *hid_func = get_hid_func(hid_class);

    if (hid_func == NULL)
        return;

    if (nbytes > 0) {
        for (int i = 0; i < nbytes; i++) {
            USB_LOG_RAW("0x%02x ", hid_func->buf[i]);
        }
        USB_LOG_RAW("nbytes:%d %#x\r\n", nbytes, hid_class->intin_urb.ep->bEndpointAddress);
    }

    hid_func->recv_flag = 1;
}

static void usbh_hid_thread(void *argument)
{
    struct usbh_hid *hid_class = (struct usbh_hid *)argument;
    struct usbh_hid_func_t *hid_func = get_hid_func(hid_class);

    usbh_hubport_enumerate_wait(hid_class->hport);

    if (hid_func == NULL)
        return;

    usbh_int_urb_fill(&hid_class->intin_urb, hid_class->hport, hid_class->intin, hid_func->buf, hid_class->intin->wMaxPacketSize, 0, usbh_hid_callback, hid_class);
    usbh_submit_urb(&hid_class->intin_urb);

    /* Suggest you to use timer for int transfer and use ep interval */
    while(1) {
        if (hid_func->recv_flag == 1) {
            usbh_int_urb_fill(&hid_class->intin_urb, hid_class->hport, hid_class->intin, hid_func->buf, hid_class->intin->wMaxPacketSize, 0, usbh_hid_callback, hid_class);
            usbh_submit_urb(&hid_class->intin_urb);
            hid_func->recv_flag = 0;
        }

        rt_thread_mdelay(hid_class->bInterval);
    }
}

void usbh_hid_run(struct usbh_hid *hid_class)
{
    struct usbh_hid_func_t *hid_func = get_hid_func(hid_class);

    if (hid_func == NULL) {
        USB_LOG_WRN("The interface function cannot be created.\n");
        return;
    }

    hid_func->hid_tid = usb_osal_thread_create(hid_class->hport->config.intf[hid_class->intf].devname,
                                                2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_hid_thread, hid_class);

}

void usbh_hid_stop(struct usbh_hid *hid_class)
{
    struct usbh_hid_func_t *hid_func = get_hid_func(hid_class);

    if (hid_func == NULL) {
        USB_LOG_WRN("The interface function cannot be delete.\n");
        return;
    }

    if (hid_func->hid_tid)
        usb_osal_thread_delete(hid_func->hid_tid);

    memset(hid_func, 0, sizeof(struct usbh_hid_func_t));
}

