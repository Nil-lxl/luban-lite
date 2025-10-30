/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: artinchip
 */

#include <rtconfig.h>
#include "usbh_core.h"
#include "usbh_hid.h"
#include "aic_osal.h"

#define AICUSB_HID_POOL_SIZE    5
#define AICUSB_HID_PROTOCOL(id, pro_name) {.pro_id = id, .name = (pro_name),}

struct hid_protocol {
    rt_list_t list;
    int pro_id;
    char name[16];
    void *data; // todo: hash table for muticast device
};

struct hid_device {
    struct hid_protocol *pro;
    struct rt_device parent;
};

enum hid_register_type {
    HID_REGISTER,
    HID_UNREGISTER,
};

static rt_list_t hid_protocal_list;
static struct hid_device hid_dev_pool[AICUSB_HID_POOL_SIZE];

struct hid_protocol gt_hid_proocol[] = {
    AICUSB_HID_PROTOCOL(0, "keyboard0"),
    AICUSB_HID_PROTOCOL(1, "keyboard1"),
    AICUSB_HID_PROTOCOL(2, "mouse"),
};


void aicusbh_hid_hotplug_irq(char *name, int32_t hotplug);

static struct hid_device *get_hid_dev_from_pool()
{
    int i = 0;
    for (i = 0; i < AICUSB_HID_POOL_SIZE; i++) {
        if (!hid_dev_pool[i].pro)
            return &hid_dev_pool[i];
    }

    return NULL;
}

static void release_hid_dev_to_pool(struct hid_device *hid_dev)
{
    memset(hid_dev, 0, sizeof(struct hid_device));
}

static int aicusbh_hid_protocol_register(struct hid_protocol *pro)
{
    RT_ASSERT(pro != RT_NULL);

    /* insert class driver into driver list */
    rt_list_insert_after(&hid_protocal_list, &(pro->list));

    return RT_EOK;
}

static struct hid_protocol *aicusbh_hid_protocol_find(int pro_id)
{
    struct rt_list_node *node;
    struct hid_protocol *pro;

    /* try to find protocal object */
    for (node = hid_protocal_list.next; node != &hid_protocal_list; node = node->next)
    {
        pro = (struct hid_protocol *)rt_list_entry(node, struct hid_protocol, list);
        if (pro->pro_id == pro_id)
            return pro;
    }

    /* not found */
    return RT_NULL;
}

static rt_err_t aicusbh_hid_open(rt_device_t dev, rt_uint16_t flag)
{
    struct usbh_hid *hid_class = (struct usbh_hid *)dev->user_data;

    usbh_hubport_enumerate_wait(hid_class->hport);

    return RT_EOK;
}

static rt_size_t aicusbh_hid_read(rt_device_t dev, rt_off_t pos, void *buf, rt_size_t len)
{
    struct usbh_hid *hid_class = (struct usbh_hid *)dev->user_data;
    struct hid_protocol *pro;

    pro = aicusbh_hid_protocol_find(hid_class->protocol);
    if (!pro) {
        USB_LOG_ERR("Can't support such HID protocol NO.%d\n", hid_class->protocol);
        return -RT_EEMPTY;
    }

    if (len != hid_class->intin->wMaxPacketSize) {
        USB_LOG_ERR("HID device[%s] read length must be %dbytes\n", pro->name, hid_class->intin->wMaxPacketSize);
        return -RT_EINVAL;
    }

    usbh_int_urb_fill(&hid_class->intin_urb, hid_class->hport, hid_class->intin,
                      buf, len,
                      USB_OSAL_WAITING_FOREVER, NULL, hid_class);
    if (usbh_submit_urb(&hid_class->intin_urb) != 0) {
        return -RT_EIO;
    }

    return len;
}

static rt_err_t aicusbh_hid_control(rt_device_t dev, int cmd, void *args)
{
    struct usbh_hid *hid_class = (struct usbh_hid *)dev->user_data;
    int ret = RT_EOK;
    switch (cmd) {
        case 0:
            *((uint16_t *)args) = hid_class->intin->wMaxPacketSize;
            break;
        case 1:
            *((uint8_t *)args) = hid_class->bInterval;
            break;
        default:
            ret = -RT_EINVAL;
            break;
    }

    return ret;
}

#ifdef RT_USING_DEVICE_OPS
static struct rt_device_ops aic_usbh_hid_ops = {
    .open = aicusbh_hid_open,
    .read = aicusbh_hid_read,
    .control = aicusbh_hid_control,
};
#endif

static int aicusbh_hid_device_register(struct hid_protocol *pro, struct usbh_hid *hid_class)
{
    int result;
    rt_device_t device;
    struct hid_device *hid_dev;

    hid_dev = get_hid_dev_from_pool();
    if (!hid_dev) {
        USB_LOG_ERR("Can't get hid deveice from hid device pool\n");
        return -RT_ENOMEM;
    }
    hid_dev->pro = pro;
    pro->data = hid_dev;

    device = &hid_dev->parent;
#ifdef RT_USING_DEVICE_OPS
    device->ops         = &aic_usbh_hid_ops;
#else
    device->init        = RT_NULL;
    device->open        = aicusbh_hid_open;
    device->close       = RT_NULL;
    device->read        = aicusbh_hid_read;
    device->write       = RT_NULL;
    device->control     = aicusbh_hid_control;
#endif

    device->type        = RT_Device_Class_Char;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;
    device->user_data   = hid_class;

    result = rt_device_register(device, pro->name, RT_DEVICE_FLAG_RDONLY);
    if (result != RT_EOK)
    {
        USB_LOG_ERR("HID device register err code: %d\n", result);
        return result;
    }

    USB_LOG_INFO("HID device[%s] register success\n", pro->name);

    return RT_EOK;
}

static int aicusbh_hid_device_unregister(struct hid_protocol *pro)
{
    struct hid_device *hid_dev = (struct hid_device *)pro->data;
    rt_device_t device = &hid_dev->parent;

    pro->data = NULL;
    rt_device_unregister(device);
    release_hid_dev_to_pool(hid_dev);

    USB_LOG_INFO("HID device[%s] unregister success\n", pro->name);

    return RT_EOK;
}

static void do_hid_register(struct usbh_hid *hid_class)
{
    struct hid_protocol *pro;

    pro = aicusbh_hid_protocol_find(hid_class->protocol);
    if (!pro) {
        USB_LOG_ERR("Can't support such HID protocol NO.%d\n", hid_class->protocol);
        return;
    }

    /* Don't support muticast device */
    if (pro->data) {
        USB_LOG_WRN("%s has been used, ignoring latest device\n", pro->name);
        return;
    }

    aicusbh_hid_device_register(pro, hid_class);
    aicusbh_hid_hotplug_irq(pro->name, true);
}

static void do_hid_unregister(struct usbh_hid *hid_class)
{
    struct hid_protocol *pro;
    pro = aicusbh_hid_protocol_find(hid_class->protocol);
    if (!pro) {
        USB_LOG_ERR("Can't find HID protocol NO.%d\n", hid_class->protocol);
        return;
    }

    aicusbh_hid_device_unregister(pro);
    aicusbh_hid_hotplug_irq(pro->name, false);
}

__WEAK void aicusbh_hid_hotplug_irq(char *name, int32_t hotplug)
{
}

void usbh_hid_run(struct usbh_hid *hid_class)
{
    do_hid_register(hid_class);
}

void usbh_hid_stop(struct usbh_hid *hid_class)
{
    do_hid_unregister(hid_class);
}

static int aicusb_hid_driver_init()
{
    int i;

    rt_list_init(&hid_protocal_list);

    for (i = 0; i < (sizeof(gt_hid_proocol) / sizeof(gt_hid_proocol[0])); i++)
        aicusbh_hid_protocol_register(&gt_hid_proocol[i]);

    return 0;
}

INIT_ENV_EXPORT(aicusb_hid_driver_init);
