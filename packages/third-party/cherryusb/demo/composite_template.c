/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  senye Liang <senye.liang@artinchip.com>
 */
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "usbd_msc.h"
#include "usb_osal.h"
#include "composite_template.h"

// #define USBD_VID           0x18D1 // Google
#define USBD_VID           0x33C3 // Display
#define USBD_PID           0x0E01
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

/*!< config descriptor size */


#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

#ifdef CONFIG_USB_HS
#define MSC_MAX_MPS 512
#else
#define MSC_MAX_MPS 64
#endif

#define USB_CONFIG_SIZE 9
#define USB_INTERFACE_SIZE 9
#define USBD_DEV_CLASS_OFFSET (18 + 9)

#define MAX_COMPOSITE_DEV   8
#define MAX_FUNC_EP_NUM     3
#define MAX_FUNC_INTF_NUM   4

int usbd_composite_detection(void);
int usbd_compsite_dev_unload(void);
int usbd_compsite_dev_load(void);
struct function_intf_t {
    bool is_loaded;
    uint8_t dev_class;
    uint8_t intf_num;
    uint8_t intf[MAX_FUNC_INTF_NUM];
    uint8_t ep_num;
    uint8_t ep[MAX_FUNC_EP_NUM];
    uint8_t class_name[10];
    void (*event_handler)(uint8_t event);
    int (*usbd_comp_class_init)(uint8_t *ep_table, void *data);
    void *data;
};

#ifdef LPKG_USING_COMP_MSC
static struct function_intf_t msc_dev = {
    .dev_class = USB_DEVICE_CLASS_MASS_STORAGE,
    .class_name = "msc",
    .data = NULL,
};
#endif

#ifdef LPKG_USING_COMP_ADBD
static struct function_intf_t adb_dev = {
    .dev_class = 0xff,
    .class_name = "adb",
    .data = NULL,
};
#endif

#ifdef LPKG_USING_COMP_DISP
static struct function_intf_t disp_dev = {
    .dev_class = 0xff,
    .class_name = "disp",
    .data = NULL,
};
#endif

#ifdef LPKG_USING_COMP_UAC
static struct function_intf_t uac_dev = {
    .dev_class = USB_DEVICE_CLASS_AUDIO,
    .class_name = "uac",
    .data = NULL,
};
#endif

#ifdef LPKG_USING_COMP_TOUCH
static struct function_intf_t touch_dev = {
    .dev_class = USB_DEVICE_CLASS_HID,
    .class_name = "touch",
    .data = NULL,
};
#endif

static struct function_intf_t *g_func_table[MAX_COMPOSITE_DEV] = {
#ifdef LPKG_USING_COMP_DISP
    &disp_dev,
#endif
#ifdef LPKG_USING_COMP_MSC
    &msc_dev,
#endif
#ifdef LPKG_USING_COMP_ADBD
    &adb_dev,
#endif
#ifdef LPKG_USING_COMP_UAC
    &uac_dev,
#endif
#ifdef LPKG_USING_COMP_TOUCH
    &touch_dev,
#endif
};

struct usbd_comp_desc_t {
    const uint8_t *dev_desc;
    uint8_t *cfg_desc;
    uint8_t *class_desc[MAX_COMPOSITE_DEV];
    const uint8_t **string_desc;
    int16_t class_desc_len[MAX_COMPOSITE_DEV];
    int16_t string_desc_num;
};

struct usbd_comp_dev_t {
    bool is_inited;
    bool is_finished;
    uint8_t intf_index;
    uint8_t ep_index;
    uint8_t dev_num;
    uint8_t intf_num;
    uint8_t max_dev_num;
    uint8_t *comp_desc;
    rt_mutex_t usbd_comp_mutex;
    struct usbd_comp_desc_t desc;
    struct function_intf_t **func_table;
} g_usbdcomp_dev;

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01)
};

static uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER)
};

#ifdef CONFIG_USB_HS
static const uint8_t qualifier_descriptor[] = {
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
    // ///////////////////////////////////////
    // /// Other Speed Configuration Descriptor
    // ///////////////////////////////////////
    0x09,
    USB_DESCRIPTOR_TYPE_OTHER_SPEED,
    WBVAL(USB_CONFIG_SIZE),
    0x01,
    0x01,
    0x00,
    USB_CONFIG_BUS_POWERED,
    USBD_MAX_POWER,
};
#endif
static const uint8_t string0_descriptor[] = {
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
};

static const uint8_t string1_descriptor[] = {
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'r', 0x00,                  /* wcChar1 */
    't', 0x00,                  /* wcChar2 */
    'I', 0x00,                  /* wcChar3 */
    'n', 0x00,                  /* wcChar4 */
    'C', 0x00,                  /* wcChar5 */
    'h', 0x00,                  /* wcChar6 */
    'i', 0x00,                  /* wcChar7 */
    'p', 0x00,                  /* wcChar8 */
};

static const uint8_t string2_descriptor[] = {
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x2A,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'r', 0x00,                  /* wcChar1 */
    't', 0x00,                  /* wcChar2 */
    'I', 0x00,                  /* wcChar3 */
    'n', 0x00,                  /* wcChar4 */
    'C', 0x00,                  /* wcChar5 */
    'h', 0x00,                  /* wcChar6 */
    'i', 0x00,                  /* wcChar7 */
    'p', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'U', 0x00,                  /* wcChar10 */
    'S', 0x00,                  /* wcChar11 */
    'B', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'e', 0x00,                  /* wcChar15 */
    'v', 0x00,                  /* wcChar16 */
    'i', 0x00,                  /* wcChar17 */
    'c', 0x00,                  /* wcChar18 */
    'e', 0x00,                  /* wcChar19 */
};

static const uint8_t string3_descriptor[] = {
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '4', 0x00,                  /* wcChar3 */
    '1', 0x00,                  /* wcChar4 */
    '2', 0x00,                  /* wcChar5 */
    '3', 0x00,                  /* wcChar6 */
    '4', 0x00,                  /* wcChar7 */
    '5', 0x00,                  /* wcChar8 */
    '6', 0x00,                  /* wcChar9 */
};

static const uint8_t *string_descriptors[4] = { string0_descriptor, string1_descriptor, string2_descriptor, string3_descriptor };

#define STRING_DESC_NUM (sizeof(string_descriptors) / (CACHE_LINE_SIZE / 8U))
#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

struct usbd_comp_dev_t *get_usbdcomp_dev(void)
{
    return &g_usbdcomp_dev;
}

int _create_dev_class(uint8_t dev_class, void *data)
{
    struct function_intf_t *func = NULL;
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();

    func = usb_malloc(sizeof(struct function_intf_t));
    memset(func, 0, sizeof(struct function_intf_t));

    for (int i = 0; i < MAX_COMPOSITE_DEV; i++) {
        if (c->func_table[i] == NULL) {
            func->dev_class = dev_class;
            if (data == NULL)
                snprintf((char *)func->class_name, sizeof(func->class_name), "null-%d", i);
            else
                memcpy((char *)func->class_name, (char *)data, strlen((char *)data));
            func->data = NULL;
            c->func_table[i] = func;
            return i;
        }
    }

    return -1;
}

int _find_dev_class(uint8_t dev_class, void *data)
{
    if (data != NULL) {
        for (int i = 0; i < MAX_COMPOSITE_DEV; i++) {
            if (g_func_table[i] == NULL)
                continue;
            USB_LOG_DBG("%s %s\n", (char *)g_func_table[i]->class_name, (char *)data);
            if (strncmp((char *)g_func_table[i]->class_name,
                        (char *)data, strlen((char *)data)) == 0) {
                return i;
            }
        }
    }

    for (int i = 0; i < MAX_COMPOSITE_DEV; i++) {
        if (g_func_table[i] == NULL)
                continue;

        if (dev_class == g_func_table[i]->dev_class) {
            if (data != NULL) {
                for (int i = 0; i < MAX_COMPOSITE_DEV; i++) {
                    USB_LOG_DBG("%s %s\n", (char *)g_func_table[i]->class_name, (char *)data);
                    if (strncmp((char *)g_func_table[i]->class_name,
                                (char *)data, strlen((char *)data)) == 0) {
                        return i;
                    }
                }
                continue;
            } else {
                return i;
            }
        }
    }

    return -1;
}

const uint8_t *_get_desc(const uint8_t *desc, uint8_t desc_type, uint16_t len)
{
    uint8_t ofs = 0;
    uint8_t desc_cnt = 0;

    if (len == 0)
        desc_cnt = 20;
    for (int i = 0; ((i < desc_cnt) || (ofs <= len)); i++) {
        if (desc[ofs + 1] == desc_type) {
            USB_LOG_DBG("CMOP: desc_type %#x\n", desc[ofs + 1]);
            return &desc[ofs];
        }

        ofs += desc[ofs];
        if (desc[ofs] == 0)
            break;

    }

    USB_LOG_DBG("CMOP: Can not find descriptor for this type: %#x\n", desc_type);

    return NULL;
}

static uint8_t g_comp_desc[1024];
uint8_t *alloc_comp_desc(struct usbd_comp_desc_t *comp)
{
    return g_comp_desc;
}

void free_comp_desc(void)
{
    memset(g_comp_desc, 0, sizeof(g_comp_desc));
}

static int usbd_comp_init(void)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();
    memset(c, 0, sizeof(struct usbd_comp_dev_t));
    c->desc.dev_desc = device_descriptor;
    c->desc.cfg_desc = config_descriptor;
    c->desc.string_desc = string_descriptors;
    c->func_table = g_func_table;
    c->usbd_comp_mutex = rt_mutex_create("usbd_comp_mutex", RT_IPC_FLAG_PRIO);
    if (c->usbd_comp_mutex == NULL) {
        USB_LOG_ERR("COMP: create dynamic mutex falied.\n");
        return -1;
    }
    return 0;
}

static int _check_comp_device_status()
{
    uint8_t dev_num;
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();

    dev_num = c->dev_num + 1;
    if (c->max_dev_num != 0 && dev_num > c->max_dev_num)
        return -1;

    return 0;
}

void usbd_comp_func_register(const uint8_t *desc,
                                void (*event_handler)(uint8_t event),
                                int (*usbd_comp_class_init)(uint8_t *ep_table, void *data),
                                void *data)
{
    int index;
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();
    struct usb_interface_descriptor *intf_desc = NULL;
    struct usb_configuration_descriptor *cfg_desc = NULL;

    rt_mutex_take(c->usbd_comp_mutex, RT_WAITING_FOREVER);

    if (_check_comp_device_status() < 0)
        return;

    intf_desc = (struct usb_interface_descriptor *)
                    _get_desc(desc, USB_DESCRIPTOR_TYPE_INTERFACE, 0);
    cfg_desc = (struct usb_configuration_descriptor *)
                    _get_desc(desc, USB_DESCRIPTOR_TYPE_CONFIGURATION, 0);

    if (intf_desc == NULL || cfg_desc == NULL) {
        USB_LOG_ERR("COMP: Failed to find the usb device desc!\n");
        return;
    }

    usbd_compsite_dev_unload();

    // 1. find the class
    index = _find_dev_class(intf_desc->bInterfaceClass, data);
    if (index < 0)
        index = _create_dev_class(intf_desc->bInterfaceClass, data);

    if (index < 0) {
        USB_LOG_ERR("COMP: Failed to find the usb device class!(%#x)\n",
                    intf_desc->bInterfaceClass);
        return;
    }

    // 2. bind class private data
    if (c->func_table[index]->is_loaded == true)
        goto _end;
    c->func_table[index]->is_loaded = true;
    c->func_table[index]->intf_num = cfg_desc->bNumInterfaces;
    c->func_table[index]->usbd_comp_class_init = usbd_comp_class_init;
    c->func_table[index]->event_handler = event_handler;

    // 3. bind class descriptor
    c->desc.class_desc[index] = (uint8_t *)&desc[USBD_DEV_CLASS_OFFSET];
    c->desc.class_desc_len[index] += cfg_desc->wTotalLength - 9; // 9:cfg_desc_len

    c->dev_num++;
    c->intf_num += cfg_desc->bNumInterfaces;
    memset(c->func_table[index]->intf, 0, MIN(c->intf_num, MAX_FUNC_INTF_NUM));

    USB_LOG_INFO("COMP: Add device class: %s\n", c->func_table[index]->class_name);
    USB_LOG_DBG("COMP: Class addr: %#lx len: %d\n", (long)c->desc.class_desc[index],
                    c->desc.class_desc_len[index] + 18 + 9);
_end:
    usbd_compsite_dev_load();
    rt_mutex_release(c->usbd_comp_mutex);

}

void usbd_comp_func_release(const uint8_t *desc, void *data)
{
    int index;
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();
    struct usb_interface_descriptor *intf_desc = NULL;
    struct usb_configuration_descriptor *cfg_desc = NULL;

    rt_mutex_take(c->usbd_comp_mutex, RT_WAITING_FOREVER);

    intf_desc = (struct usb_interface_descriptor *)
                    _get_desc(desc, USB_DESCRIPTOR_TYPE_INTERFACE, 0);
    cfg_desc = (struct usb_configuration_descriptor *)
                    _get_desc(desc, USB_DESCRIPTOR_TYPE_CONFIGURATION, 0);

    if (intf_desc == NULL || cfg_desc == NULL) {
        USB_LOG_ERR("COMP: Failed to find the usb device desc!\n");
        return;
    }

    index = _find_dev_class(intf_desc->bInterfaceClass, data);
    if (index < 0) {
        USB_LOG_ERR("COMP: Failed to find the usb device class!(%#x)\n",
                    intf_desc->bInterfaceClass);
        return;
    }

    if (c->func_table[index]->is_loaded == false)
        goto _end;
    c->func_table[index]->is_loaded = false;
    c->desc.class_desc[index] = NULL;
    c->desc.class_desc_len[index] = 0;

    c->dev_num--;
    c->intf_num -= cfg_desc->bNumInterfaces;

    USB_LOG_INFO("COMP: Release device class: %s\n", c->func_table[index]->class_name);

    usbd_compsite_dev_unload();
    usbd_compsite_dev_load();
_end:
    rt_mutex_release(c->usbd_comp_mutex);

}

int make_comp_dev_desc(uint8_t *dest, uint8_t *src, int len)
{
    struct usb_device_descriptor *dev = NULL;
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();

    memcpy(dest, src, len);
    dev = (struct usb_device_descriptor *)dest;
    dev->idVendor = USBD_VID;
    dev->idProduct = USBD_PID;
    if (c->dev_num > 1 && USBD_PID < 0xFFFF)
        dev->idProduct += 1;

    return len;
}

int make_comp_cfg_desc(uint8_t *dest, uint8_t *src, int len)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();
    struct usb_configuration_descriptor *cfg = NULL;

    memcpy(dest, src, len);
    cfg = (struct usb_configuration_descriptor *)dest;
    cfg->wTotalLength = USB_CONFIG_SIZE;

    for (int i = 0; i < MAX_COMPOSITE_DEV; i++) {
        if (c->desc.class_desc_len[i] == 0)
            continue;
        cfg->wTotalLength += c->desc.class_desc_len[i];
    }
    cfg->bNumInterfaces = c->intf_num;

    return len;
}

int _make_uvc_intf_cs_desc(uint8_t *src)
{
    struct usb_interface_descriptor *intf_desc = NULL;
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();
    uint8_t *desc = NULL;

    intf_desc = (struct usb_interface_descriptor *)src;

    if (intf_desc->bInterfaceSubClass == 0x01U) {   /* VIDEO_SC_VIDEOCONTROL */
        desc = (uint8_t *)_get_desc(src, 0x24U, 0); /* VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE */
        if (desc[2] == 0x01U) /* VIDEO_VC_HEADER_DESCRIPTOR_SUBTYPE */ {
            desc[desc[0] - 1] = c->intf_index;
        }
    }

    return 0;
}

int make_comp_intf_cs_desc(uint8_t *src)
{
    struct usb_interface_descriptor *intf_desc = NULL;

    intf_desc = (struct usb_interface_descriptor *)src;

    switch(intf_desc->bInterfaceClass) {
        case USB_DEVICE_CLASS_VIDEO :
            _make_uvc_intf_cs_desc(src);
        break;
    }

    return 0;
}

int make_comp_intf_ep_desc(uint8_t *dest, uint8_t *src,
                            int len, int index)
{
    uint8_t *intf_ptr, *ep_ptr;
    uint8_t last_intf_index = 0, last_addr = 0, intf_num = 0;
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();
    struct usb_interface_descriptor *intf_desc = NULL;
    struct usb_interface_association_descriptor *iad_desc = NULL;
    struct usb_endpoint_descriptor *ep = NULL;

    memcpy(dest, src, len);

    intf_ptr = dest;
    ep_ptr = dest;

    do {
        /* check if the first descriptor is IAD ? */
        iad_desc = (struct usb_interface_association_descriptor *)
                    _get_desc(intf_ptr, USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION, intf_ptr[0]);
        if (iad_desc != NULL)
            iad_desc->bFirstInterface = c->intf_index;

        /* 1. make interface descriptor. */
        intf_ptr = (uint8_t *)_get_desc(intf_ptr, USB_DESCRIPTOR_TYPE_INTERFACE, 0);
        if (intf_ptr == NULL)
            goto _finish;

        intf_desc = (struct usb_interface_descriptor *)intf_ptr;

        c->func_table[index]->intf[intf_num] = c->intf_index;
        intf_num++;

        if (intf_desc->bAlternateSetting == 0) {
            intf_desc->bInterfaceNumber = c->intf_index;
            last_intf_index = c->intf_index;
            c->intf_index++;
        } else {
            intf_desc->bInterfaceNumber = last_intf_index;
        }

        // By default, all interfaces use a product character descriptor(string0).
        intf_desc->iInterface = 0;

        /* 1.1 make class-specific interface descriptor*/
        make_comp_intf_cs_desc(intf_ptr);

        /* point to the next interface descriptor. */
        intf_ptr += intf_ptr[0];

        /* 2. make endpoint descriptor. */
        if (intf_desc->bNumEndpoints == 0)
            continue;
        ep_ptr = (uint8_t *)_get_desc((uint8_t *)intf_desc, USB_DESCRIPTOR_TYPE_ENDPOINT, 0);
        ep = (struct usb_endpoint_descriptor *)ep_ptr;
        if (ep == NULL) {
            USB_LOG_ERR("COMP: Failed to find the usb device ep_desc!\n");
            return 0;
        }

        for (int i = 0; i < intf_desc->bNumEndpoints; i++, ep++) {
            c->func_table[index]->ep_num = intf_desc->bNumEndpoints;
            if (((ep->bEndpointAddress) & 0x0F) != last_addr)
                c->ep_index++;
            last_addr = ep->bEndpointAddress & 0x0F;
            ep->bEndpointAddress &= 0xF0;
            ep->bEndpointAddress |= c->ep_index;
            c->func_table[index]->ep[i] = ep->bEndpointAddress;
        }
    } while(intf_desc != NULL);

_finish:
    return len;
}

uint8_t *make_comp_desc(uint8_t *dest, struct usbd_comp_desc_t *src)
{
    int i, ofs = 0;

    /* 1. make device desc */
    ofs += make_comp_dev_desc(dest, (uint8_t *)src->dev_desc,
                                src->dev_desc[0]);

    /* 2. make config desc */
    ofs += make_comp_cfg_desc(dest + ofs, (uint8_t *)src->cfg_desc,
                                src->cfg_desc[0]);

    /* 3. make intf & ep desc */
    for (i = 0; i < MAX_COMPOSITE_DEV; i++) {
        if (src->class_desc[i] == NULL || src->class_desc_len[i] == 0)
            continue;
        ofs += make_comp_intf_ep_desc(dest + ofs, (uint8_t *)src->class_desc[i],
                                        src->class_desc_len[i], i);
    }

    /* 4. make string desc */
    for (i = 0; i < STRING_DESC_NUM; i++) {
        memcpy(dest + ofs, (uint8_t *)src->string_desc[i], src->string_desc[i][0]);
        ofs += src->string_desc[i][0];
    }

    #ifdef CONFIG_USB_HS
    memcpy(dest + ofs, qualifier_descriptor, sizeof(qualifier_descriptor));
    #endif

    return dest;
}

void usbd_event_handler(uint8_t event)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();
    if (usbd_compsite_is_inited() == false)
        return;

    for (int i = 0; i < MAX_COMPOSITE_DEV; i++) {
        if (c->func_table[i] == NULL ||
            c->func_table[i]->is_loaded != true ||
            c->func_table[i]->event_handler == NULL)
            continue;

        c->func_table[i]->event_handler(event);
    }
}

static void usbd_composite_add_intf(void)
{
    int ret = 0;
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();

    for (int i = 0; i < MAX_COMPOSITE_DEV; i++) {
        if (c->func_table[i] == NULL ||
            c->func_table[i]->is_loaded != true)
            continue;

        if (c->func_table[i]->usbd_comp_class_init != NULL) {
            USB_LOG_INFO("COMP: %s device loaded.\n", c->func_table[i]->class_name);
            ret = c->func_table[i]->usbd_comp_class_init(c->func_table[i]->ep, NULL);
            if(ret < 0)
                USB_LOG_WRN("COMP: Class %#x initialization falied.\n",
                            c->func_table[i]->dev_class);
        }
    }
}

bool usbd_compsite_is_inited(void)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();
    return c->is_finished;
}

uint8_t usbd_compsite_set_dev_num(uint8_t num)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();

    rt_mutex_take(c->usbd_comp_mutex, RT_WAITING_FOREVER);
    c->max_dev_num = num;
    rt_mutex_release(c->usbd_comp_mutex);

    return c->max_dev_num;
}

uint8_t usbd_compsite_get_dev_num(void)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();

    return c->max_dev_num;
}

int usbd_compsite_dev_load(void)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();

    if (c->comp_desc != NULL)
        free_comp_desc();
    c->ep_index = 0;
    c->intf_index = 0;
    c->comp_desc = alloc_comp_desc(&c->desc);
    make_comp_desc(c->comp_desc, &c->desc);
    usbd_desc_register(c->comp_desc);
    usbd_composite_add_intf();

    if (c->max_dev_num == 0 || c->dev_num == c->max_dev_num) {
        usbd_initialize();
        c->is_finished = true;
        USB_LOG_INFO("COMP: Composite device loaded.(%d - %d)\n", c->dev_num, c->max_dev_num);
    }

    return 0;
}

int usbd_compsite_dev_unload(void)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();

    if (c->is_finished == true) {
        usbd_deinitialize();
        c->is_finished = false;
        USB_LOG_INFO("COMP: Composite device unloaded.(%d)\n", c->dev_num);
    }

    if (c->comp_desc != NULL)
        free_comp_desc();

    c->ep_index = 0;
    c->intf_index = 0;

    return 0;
}

#if defined(KERNEL_RTTHREAD)
rt_thread_t usbd_composite_tid;

static void usbd_comp_detection_thread(void *parameter)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();
_retry:
    rt_thread_mdelay(400);
    // Has the composite device been loaded
    if (c->is_finished == true)
        goto _retry;

    rt_mutex_take(c->usbd_comp_mutex, RT_WAITING_FOREVER);

    usbd_compsite_dev_load();

    rt_mutex_release(c->usbd_comp_mutex);

}

int usbd_composite_detection(void)
{
    usbd_composite_tid = rt_thread_create("usbd_comp_detection", usbd_comp_detection_thread, RT_NULL,
                                    1024 * 5, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (usbd_composite_tid != RT_NULL)
        rt_thread_startup(usbd_composite_tid);
    else
        printf("create usbd_comp_detection thread err!\n");

    return RT_EOK;
}
INIT_PREV_EXPORT(usbd_comp_init);

#include <finsh.h>
#include <getopt.h>

static void cmd_comp_usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -l, \tlist all usb composite device.\n");
    printf("\t -h ,\tusage\n");
}

static int _list_comp_device_status(void)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();

    printf("---------------------------------\n");
    printf("composite device status -> %s \n", c->is_finished ? "ok" : "no ready");
    printf("functions number -> %d / %d \n", c->dev_num, c->max_dev_num);
    return 0;
}

static int _list_comp_device(void)
{
    struct usbd_comp_dev_t *c = get_usbdcomp_dev();

    printf("----------------------------------------");
    printf("----------------------------------------\n");

    printf("|     Name\t |\tClass\t|\tActive\t|\tIntf\t|\tEp\t|\n");

    for (int i = 0; i < MAX_COMPOSITE_DEV; i++) {
        if (c->func_table[i] == NULL)
            continue;

        if ((char *)c->func_table[i]->class_name != NULL)
            printf("      %-7s\t", (char *)c->func_table[i]->class_name);
        else
            printf("\tno name\t");

        printf("\t%#x\t", c->func_table[i]->dev_class);
        printf("\t %d\t", c->func_table[i]->is_loaded);

        if (!c->func_table[i]->is_loaded) {
            printf("\n");
            continue;
        }

        printf("\t");
        for (int j = 0; j < c->func_table[i]->intf_num; j++)
            printf("%d ", c->func_table[i]->intf[j]);

        printf("\t\t");
        for (int j = 0; j < c->func_table[i]->ep_num; j++)
            printf("%#x ", c->func_table[i]->ep[j]);
        printf("\n");
    }

    printf("----------------------------------------");
    printf("----------------------------------------\n");
    return 0;
}

static void cmd_test_usb_comp(int argc, char **argv)
{
    int opt;
    if (argc < 2) {
        _list_comp_device_status();
        _list_comp_device();
        return;
    }
    optind = 0;
    while ((opt = getopt(argc, argv, "lhu:")) != -1) {
        switch (opt) {
        case 'l':
            _list_comp_device_status();
            _list_comp_device();
            break;
        case 'h':
        default:
            cmd_comp_usage(argv[0]);
            break;
        }
    }
}
MSH_CMD_EXPORT_ALIAS(cmd_test_usb_comp, test_usb_comp, usb composite device test);
#endif
