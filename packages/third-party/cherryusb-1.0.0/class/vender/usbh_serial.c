/*
 * Copyright (c) 2025, ArtInChip
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbh_core.h"
// #include "usbh_serial.h"

#define DEV_FORMAT "/dev/ttyUSB%d"

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_serial_buf[USB_ALIGN_UP(64, CONFIG_USB_ALIGN_SIZE)];

#define CONFIG_USBHOST_MAX_SERIAL_CLASS 5

struct usbh_serial {
    struct usbh_hubport *hport;
    struct usb_endpoint_descriptor *bulkin;  /* Bulk IN endpoint */
    struct usb_endpoint_descriptor *bulkout; /* Bulk OUT endpoint */
    struct usbh_urb bulkout_urb;
    struct usbh_urb bulkin_urb;

    uint8_t intf;
    uint8_t minor;

    void *user_data;
};

static struct usbh_serial g_serial_class[CONFIG_USBHOST_MAX_SERIAL_CLASS];
static uint32_t g_devinuse = 0;

__WEAK void usbh_serial_run(struct usbh_serial *serial_class)
{
    (void)serial_class;
}

__WEAK void usbh_serial_stop(struct usbh_serial *serial_class)
{
    (void)serial_class;
}

static struct usbh_serial *usbh_serial_class_alloc(void)
{
    uint8_t devno;

    for (devno = 0; devno < CONFIG_USBHOST_MAX_SERIAL_CLASS; devno++) {
        if ((g_devinuse & (1U << devno)) == 0) {
            g_devinuse |= (1U << devno);
            memset(&g_serial_class[devno], 0, sizeof(struct usbh_serial));
            g_serial_class[devno].minor = devno;
            return &g_serial_class[devno];
        }
    }
    return NULL;
}

static void usbh_serial_class_free(struct usbh_serial *serial_class)
{
    uint8_t devno = serial_class->minor;

    if (devno < 32) {
        g_devinuse &= ~(1U << devno);
    }
    memset(serial_class, 0, sizeof(struct usbh_serial));
}


static int usbh_serial_connect(struct usbh_hubport *hport, uint8_t intf)
{
    struct usb_endpoint_descriptor *ep_desc;
    int ret = 0;

    struct usbh_serial *serial_class = usbh_serial_class_alloc();
    if (serial_class == NULL) {
        USB_LOG_ERR("Fail to alloc serial_class\r\n");
        return -USB_ERR_NOMEM;
    }

    serial_class->hport = hport;
    serial_class->intf = intf;

    hport->config.intf[intf].priv = serial_class;

    for (uint8_t i = 0; i < hport->config.intf[intf].altsetting[0].intf_desc.bNumEndpoints; i++) {
        ep_desc = &hport->config.intf[intf].altsetting[0].ep[i].ep_desc;

        if (ep_desc->bEndpointAddress & 0x80) {
            USBH_EP_INIT(serial_class->bulkin, ep_desc);
        } else {
            USBH_EP_INIT(serial_class->bulkout, ep_desc);
        }
    }

    snprintf(hport->config.intf[intf].devname, CONFIG_USBHOST_DEV_NAMELEN, DEV_FORMAT, serial_class->minor);

    USB_LOG_INFO("Register Serial Class:%s\r\n", hport->config.intf[intf].devname);

    usbh_serial_run(serial_class);
    return ret;
}

static int usbh_serial_disconnect(struct usbh_hubport *hport, uint8_t intf)
{
    int ret = 0;

    struct usbh_serial *serial_class = (struct usbh_serial *)hport->config.intf[intf].priv;

    if (serial_class) {
        if (serial_class->bulkin) {
            usbh_kill_urb(&serial_class->bulkin_urb);
        }

        if (serial_class->bulkout) {
            usbh_kill_urb(&serial_class->bulkout_urb);
        }

        if (hport->config.intf[intf].devname[0] != '\0') {
            usb_osal_thread_schedule_other();
            USB_LOG_INFO("Unregister Serial Class:%s\r\n", hport->config.intf[intf].devname);
            usbh_serial_stop(serial_class);
        }

        usbh_serial_class_free(serial_class);
    }

    return ret;
}

int usbh_serial_bulk_in_transfer(struct usbh_serial *serial_class, uint8_t *buffer, uint32_t buflen, uint32_t timeout)
{
    int ret;
    struct usbh_urb *urb = &serial_class->bulkin_urb;

    usbh_bulk_urb_fill(urb, serial_class->hport, serial_class->bulkin, buffer, buflen, timeout, NULL, NULL);
    ret = usbh_submit_urb(urb);
    if (ret == 0) {
        ret = urb->actual_length;
    }
    return ret;
}

int usbh_serial_bulk_out_transfer(struct usbh_serial *serial_class, uint8_t *buffer, uint32_t buflen, uint32_t timeout)
{
    int ret;
    struct usbh_urb *urb = &serial_class->bulkout_urb;

    usbh_bulk_urb_fill(urb, serial_class->hport, serial_class->bulkout, buffer, buflen, timeout, NULL, NULL);
    ret = usbh_submit_urb(urb);
    if (ret == 0) {
        ret = urb->actual_length;
    }
    return ret;
}

//ttyusb 0 AT
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX char serial_buffer[512];
int cmd_ttyusb_debug(int argc, char *argv[])
{
    int ret;
    struct usbh_serial *serial_class;
    int len = 0;

    char dev_name[20] = {0};
    if (argc != 3) {
        printf("ttyusb [NO] [AT]\n");
        return -1;
    }

    snprintf(dev_name, 20, "/dev/ttyUSB%s", argv[1]);

    while ((serial_class = (struct usbh_serial *)usbh_find_class_instance(dev_name)) == NULL) {
        printf("Can't find name %s\n", dev_name);
        return -1;
    }

    memset(serial_buffer, 0, 512);
    strncpy(serial_buffer, argv[2], 511);
    len = strlen(serial_buffer);
    serial_buffer[len++] = '\r';
    serial_buffer[len++] = '\n';

    printf("%s send: %s\n", dev_name, serial_buffer);
    ret = usbh_serial_bulk_out_transfer(serial_class, (uint8_t *)serial_buffer, len, 100);
    if (ret < 0) {
        USB_LOG_RAW("bulk out error,ret:%d\r\n", ret);
        return -1;
    } else {
        USB_LOG_RAW("send over:%d\r\n", serial_class->bulkout_urb.actual_length);
    }

    ret = usbh_serial_bulk_in_transfer(serial_class, (uint8_t *)serial_buffer, 512, 100);
    if (ret < 0) {
        USB_LOG_RAW("bulk in error,ret:%d\r\n", ret);
        return -1;
    } else {
    }

    printf("%s receive: %s\n", dev_name, serial_buffer);

    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_ttyusb_debug, ttyusb, ttycmd);

const struct usbh_class_driver serial_class_driver = {
    .driver_name = "serial",
    .connect = usbh_serial_connect,
    .disconnect = usbh_serial_disconnect
};

CLASS_INFO_DEFINE const struct usbh_class_info serial_class_info = {
    .match_flags = USB_CLASS_MATCH_INTF_CLASS,//USB_CLASS_MATCH_VID_PID | USB_CLASS_MATCH_INTF_CLASS,
    .class = 0xff,
    .subclass = 0,
    .protocol = 0,
    .vid = 0,
    .pid = 0,
    .class_driver = &serial_class_driver
};
