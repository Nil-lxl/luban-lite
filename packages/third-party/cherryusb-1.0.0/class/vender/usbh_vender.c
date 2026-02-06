/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "usbh_core.h"
#include "usbh_vender.h"

#define DEV_FORMAT "/dev/vender"

#define CONFIG_USBHOST_MAX_CUSTOM_CLASS 1
static struct usbh_vender g_vender_class[CONFIG_USBHOST_MAX_CUSTOM_CLASS];
static uint32_t g_devinuse = 0;

static struct usbh_vender *usbh_vender_class_alloc(void)
{
    int devno;

    for (devno = 0; devno < CONFIG_USBHOST_MAX_CUSTOM_CLASS; devno++) {
        if ((g_devinuse & (1 << devno)) == 0) {
            g_devinuse |= (1 << devno);
            memset(&g_vender_class[devno], 0, sizeof(struct usbh_vender));
            g_vender_class[devno].minor = devno;
            return &g_vender_class[devno];
        }
    }
    return NULL;
}

static void usbh_vender_class_free(struct usbh_vender *vender_class)
{
    int devno = vender_class->minor;

    if (devno >= 0 && devno < 32) {
        g_devinuse &= ~(1 << devno);
    }
    memset(vender_class, 0, sizeof(struct usbh_vender));
}

static int usbh_vender_connect(struct usbh_hubport *hport, uint8_t intf)
{
    struct usb_endpoint_descriptor *ep_desc;
    int ret = 0;

    struct usbh_vender *vender_class = usbh_vender_class_alloc();
    if (vender_class == NULL) {
        USB_LOG_ERR("Fail to alloc vender_class\r\n");
        return -USB_ERR_NOMEM;
    }

    return ret;
}


static int usbh_vender_disconnect(struct usbh_hubport *hport, uint8_t intf)
{
    int ret = 0;

    struct usbh_vender *vender_class = (struct usbh_vender *)hport->config.intf[intf].priv;

    if (vender_class) {
        if (vender_class->venderin) {
            usbh_kill_urb(&vender_class->venderin_urb);
        }

        if (vender_class->venderout) {
            usbh_kill_urb(&vender_class->venderout_urb);
        }

        if (hport->config.intf[intf].devname[0] != '\0') {
            USB_LOG_INFO("Unregister vender Class:%s\r\n", hport->config.intf[intf].devname);
            usbh_vender_stop(vender_class);
        }

        usbh_vender_class_free(vender_class);
    }

    return ret;
}

__WEAK void usbh_vender_run(struct usbh_vender *vender_class)
{
}

__WEAK void usbh_vender_stop(struct usbh_vender *vender_class)
{
}

static const struct usbh_class_driver vender_class_driver = {
    .driver_name = "vender",
    .connect = usbh_vender_connect,
    .disconnect = usbh_vender_disconnect
};

CLASS_INFO_DEFINE const struct usbh_class_info vender_class_info = {
    .match_flags = USB_CLASS_MATCH_INTF_CLASS | USB_CLASS_MATCH_INTF_SUBCLASS | USB_CLASS_MATCH_INTF_PROTOCOL,
    .class = 0,
    .subclass = 0,
    .protocol = 0,
    .vid = 0x00,
    .pid = 0x00,
    .class_driver = &vender_class_driver
};
