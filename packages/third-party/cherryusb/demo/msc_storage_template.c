/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usbd_msc.h"

#define MSC_IN_EP  0x81
#define MSC_OUT_EP 0x02

#define USBD_VID           0x33C3
#define USBD_PID           0xFFFF
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define USB_CONFIG_SIZE (9 + MSC_DESCRIPTOR_LEN)

#ifdef CONFIG_USB_HS
#define MSC_MAX_MPS 512
#else
#define MSC_MAX_MPS 64
#endif


const uint8_t msc_storage_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_1_1, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MSC_DESCRIPTOR_INIT(0x00, MSC_OUT_EP, MSC_IN_EP, MSC_MAX_MPS, 0x02),
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
    'M', 0x00,                  /* wcChar10 */
    'S', 0x00,                  /* wcChar11 */
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
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

#include <string.h>
#include <string.h>
#include "ff.h"			/* Declarations of FatFs API */
/* ELM FatFs provide a DIR struct */
#define HAVE_DIR_STRUCTURE
#include <dfs_fs.h>
#include <dfs_file.h>
#include <dfs_elm.h>
#include "diskio.h"		/* Declarations of device I/O functions */

struct usbd_storage_p {
    struct dfs_filesystem *fs;
    rt_device_t dev;
    char dev_name[10];
    uint32_t block_num;
    uint16_t block_size;
    unsigned char pdrv;
    char fs_path[10];
    char fs_type[5];
    bool storage_exist;
    uint8_t is_inited;
    uint8_t is_forbidden;
    uint8_t is_configured;
};

char msc_storage_path[] = MSC_STORAGE_PATH;
rt_thread_t usbd_msc_tid;
struct usbd_interface msc_intf0;
struct usbd_storage_p usbd_storage;
static void usbd_msc_suspend(void);
static void usbd_msc_configured(void);
int usbd_msc_detection(void);

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
void usbd_comp_msc_event_handler(uint8_t event)
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
            usbd_msc_suspend();
            break;
        case USBD_EVENT_CONFIGURED:
#if !defined(KERNEL_RTTHREAD)
            usbd_msc_configured();
#endif
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

struct usbd_storage_p *get_usbd_storage(void)
{
    return &usbd_storage;
}

static void usbd_msc_suspend(void)
{
    int ret = 0;
    long device_type = 0;

    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    if (usbd_storage == NULL || usbd_storage->fs == NULL)
        return;

    if (usbd_storage->is_configured == false)
        return;

    if ((FATFS *)usbd_storage->fs->data == NULL) {
#if defined(KERNEL_RTTHREAD)
        usbd_storage->dev = rt_device_find(usbd_storage->dev_name);
        if (usbd_storage->dev == NULL) {
            usbd_storage->storage_exist = false;
            return;
        }
        rt_device_close(usbd_storage->dev);
#else
        disk_ioctl(usbd_storage->pdrv, GET_DEVICE_TYPE, &device_type);
#endif
        ret = dfs_mount(usbd_storage->dev_name, usbd_storage->fs_path, usbd_storage->fs_type, 0, (void *)device_type);
        if (ret < 0)
            USB_LOG_ERR("Failed to mount %s to %s\n", usbd_storage->dev_name, usbd_storage->fs_path);
        else
            USB_LOG_INFO("Mount %s to %s\n", usbd_storage->dev_name, usbd_storage->fs_path);
    }

    usbd_storage->storage_exist = true;
}

static void usbd_msc_configured(void)
{
    int ret;
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    if (usbd_storage == NULL || usbd_storage->fs == NULL)
        return;

    if ((FATFS *)usbd_storage->fs->data == NULL)
        return;

    ret = dfs_unmount(usbd_storage->fs_path);
    if (ret < 0)
        USB_LOG_ERR("Failed to unmount %s !\n", usbd_storage->fs_path);
    else
        USB_LOG_INFO("Unmount %s \n", usbd_storage->fs_path);

#if defined(KERNEL_RTTHREAD)
    ret = rt_device_open(usbd_storage->dev, RT_DEVICE_FLAG_RDWR);
    if (ret != RT_EOK) {
        USB_LOG_ERR("Open device failed !\n");
        return;
    }
#endif
    usbd_storage->storage_exist = true;
    usbd_storage->is_configured = true;
}

void usbd_msc_get_cap(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    if (usbd_storage == NULL || usbd_storage->storage_exist == false)
        return;

    *block_num = (uint32_t)usbd_storage->block_num;
    *block_size = (uint16_t)usbd_storage->block_size;

    USB_LOG_DBG("block_num:%ld block_size:%d\n", *block_num, *block_size);
}

int usbd_msc_sector_read(uint32_t sector, uint8_t *buffer, uint32_t length)
{
    int ret = -1;
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    if (usbd_storage->storage_exist == false || usbd_storage == NULL)
        return 0;

#if defined(KERNEL_RTTHREAD)
    ret = rt_device_read(usbd_storage->dev,
                         sector, buffer,
                         length / usbd_storage->block_size);
    if (ret == length / usbd_storage->block_size)
        return 0;
#else
    ret = disk_read(usbd_storage->pdrv, buffer, sector,
                    length / usbd_storage->block_size);
#endif
    return ret;
}

int usbd_msc_sector_write(uint32_t sector, uint8_t *buffer, uint32_t length)
{
    int ret = -1;
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    if (usbd_storage->storage_exist == false || usbd_storage == NULL)
        return 0;

#if defined(KERNEL_RTTHREAD)
    ret = rt_device_write(usbd_storage->dev,
                          sector, buffer,
                          length / usbd_storage->block_size);
    if (ret == length / usbd_storage->block_size)
        return 0;
#else
    ret = disk_write(usbd_storage->pdrv, buffer, sector,
                     length / usbd_storage->block_size);
#endif
    return ret;
}

extern struct dfs_filesystem filesystem_table[];
int _fs_is_exist(char *path)
{
    struct dfs_filesystem *iter;
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    for (iter = &filesystem_table[0];
            iter < &filesystem_table[DFS_FILESYSTEMS_MAX]; iter++) {
        if ((iter != NULL) && (iter->path != NULL)) {
            if (strcmp(iter->path, path) == 0) {
                strcpy(usbd_storage->fs_path, iter->path);
                strcpy(usbd_storage->fs_type, iter->ops->name);
                return 0;
            }
        }
    }
    return -1;
}

int usbd_storage_init(char *path)
{
    FATFS *f; // using elmfat
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    if (_fs_is_exist(path) < 0)
        return -1;

    usbd_storage->fs = dfs_filesystem_lookup(path);
    f = (FATFS *)usbd_storage->fs->data;
    usbd_storage->pdrv = f->pdrv;
    usbd_storage->dev = usbd_storage->fs->dev_id;
    strcpy(usbd_storage->dev_name, (char *)usbd_storage->dev);
#if defined(KERNEL_RTTHREAD)
    int ret;
    struct rt_device_blk_geometry geometry;

    rt_device_close(usbd_storage->dev);
    ret = rt_device_open(usbd_storage->dev, RT_DEVICE_FLAG_RDWR);
    if (ret != RT_EOK) {
        USB_LOG_ERR("Open device failed !\n");
        return -1;
    }

    rt_memset(&geometry, 0, sizeof(geometry));
    ret = rt_device_control(usbd_storage->dev, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry);
    if (ret != RT_EOK) {
        USB_LOG_ERR("Get geometry failed!\n");
        return -1;
    }

    usbd_storage->block_num = geometry.sector_count;
    usbd_storage->block_size = geometry.block_size;
#else
    disk_ioctl(usbd_storage->pdrv, GET_SECTOR_COUNT, &usbd_storage->block_num);
    disk_ioctl(usbd_storage->pdrv, GET_SECTOR_SIZE, &usbd_storage->block_size);
#endif
    return 0;
}

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
#include "composite_template.h"
int usbd_comp_msc_init(uint8_t *ep_table, void *data)
{
    usbd_add_interface(usbd_msc_init_intf(&msc_intf0, ep_table[0], ep_table[1]));
    return 0;
}
#endif

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
#include "composite_template.h"
#endif

int msc_storage_init(char *path)
{
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    if (usbd_storage->is_forbidden == true)
        return -1;

    if (usbd_storage->is_inited == true) {
        USB_LOG_DBG("Msc storage already initialized ! (%s)\n", usbd_storage->fs_path);
        return 0;
    }

#ifndef LPKG_CHERRYUSB_DEVICE_COMPOSITE
    usbd_desc_register(msc_storage_descriptor);
    usbd_add_interface(usbd_msc_init_intf(&msc_intf0, MSC_OUT_EP, MSC_IN_EP));
    usbd_initialize();
#else
    usbd_comp_func_register(msc_storage_descriptor,
                            usbd_comp_msc_event_handler,
                            usbd_comp_msc_init, NULL);
#endif
    usbd_storage->is_inited = true;
    USB_LOG_INFO("msc_storage_init.\n");
    return 0;
}

int msc_storage_deinit()
{
    struct usbd_storage_p *usbd_storage = get_usbd_storage();
    memset(usbd_storage, 0, sizeof(struct usbd_storage_p));
    usbd_deinitialize();

    return 0;
}

int msc_storage_forbid(void)
{
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    if (usbd_storage->is_forbidden == true)
        return -1;

    usbd_msc_thread_deinit();
    usbd_msc_set_popup();
    usbd_storage->is_forbidden = true;
    usbd_msc_check_storage();
    if (usbd_msc_tid != NULL)
        rt_thread_delete(usbd_msc_tid);
    usbd_msc_suspend();

    return 0;
}

int msc_usbd_forbid(void)
{
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    if (usbd_storage->is_forbidden == true)
        return -1;

    usbd_storage->is_forbidden = true;
    usbd_msc_check_storage();
    usbd_msc_thread_deinit();
    if (usbd_msc_tid != NULL)
        rt_thread_delete(usbd_msc_tid);
    usbd_msc_suspend();

    #ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
    usbd_storage->is_inited = false;
    usbd_comp_func_release(msc_storage_descriptor, NULL);
    #else
    msc_storage_deinit();
    #endif

    return 0;
}

int msc_storage_allow(void)
{
    struct usbd_storage_p *usbd_storage = get_usbd_storage();
    usbd_storage->is_forbidden = false;
    usbd_msc_detection();

    return 0;
}

#if defined(AIC_CONSOLE_BARE_DRV)
#include <console.h>
static int cmd_msc_storage_init(int argc, char *argv[])
{
    msc_storage_init(argv[1]);
    return 0;
}
CONSOLE_CMD(msc_init, cmd_msc_storage_init, "Mount usb massstorage");
#endif

#if defined(KERNEL_RTTHREAD)
bool usbd_msc_check_storage(void)
{
    rt_device_t device = NULL;
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    device = rt_device_find(usbd_storage->dev_name);
    if (device == NULL)
        usbd_storage->storage_exist = false;
    else
        usbd_storage->storage_exist = true;

    if (usbd_storage->is_forbidden == true)
        usbd_storage->storage_exist = false;

    return usbd_storage->storage_exist;
}

static void usbd_msc_detection_thread(void *parameter)
{
retry:
    if (msc_storage_init(msc_storage_path) < 0) {
        rt_thread_mdelay(400);
        goto retry;
    }

    if ((_fs_is_exist(msc_storage_path) < 0)) {
        rt_thread_mdelay(400);
        goto retry;
    }

    /* When you need to disable the USB function without using MSC,
    *  please initialize the USB function here and use it in conjunction
    *  with the msc_storage_deinit function below.
    */
    // msc_storage_init(msc_storage_path);

    if (usbd_storage_init(msc_storage_path) < 0) {
        USB_LOG_ERR("Failed to detect %s !\n", msc_storage_path);
        goto retry;
    }

    usbd_msc_configured();
    usbd_msc_thread_init();

    USB_LOG_INFO("Msc storage detected.\n");

#ifdef USBD_MSC_STORAGE_USING_HOTPLUG
    while (1) {
        rt_thread_mdelay(400);
        if (false == usbd_msc_check_storage()) {
            USB_LOG_INFO("Msc storage ejected.\n");
            usbd_msc_thread_deinit();
            usbd_msc_set_popup();
            // msc_storage_deinit(); /* this will completely shut down USB.*/
            goto retry;
        }
    }
#endif
}
int usbd_msc_detection(void)
{
    usbd_msc_tid = rt_thread_create("usbd_msc_detection",
                                    usbd_msc_detection_thread, RT_NULL,
                                    2560, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (usbd_msc_tid != RT_NULL)
        rt_thread_startup(usbd_msc_tid);
    else
        printf("create usbd_msc_detection thread err!\n");

    return RT_EOK;
}
#if !defined(LPKG_CHERRYUSB_DYNAMIC_REGISTRATION_MODE)
INIT_APP_EXPORT(usbd_msc_detection);
#endif

#include <getopt.h>
static void cmd_msc_usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -a, \t allowed \n");
    printf("\t -f [usb] or [media],   \t forbidden usb or media\n");
    printf("\t -n [media name],       \t media name\n");

    printf("\t -h ,\tusage\n");
}

static char sopts[] = "af:n:ih";
static struct option lopts[] = {
    {"-a allowed msc ",    no_argument, NULL, 'a'},
    {"-f forbidden usb or media",  required_argument, NULL, 'f'},
    {"-n media name",       required_argument, NULL, 'n'},
    {"-i show msc info",    required_argument, NULL, 'n'},
    {0, 0, 0, 0}
    };

static int cmd_test_msc_mount(int argc, char **argv)
{
    int opt;
    struct usbd_storage_p *usbd_storage = get_usbd_storage();

    optind = 0;
    while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (opt) {
        case 'a':
            msc_storage_allow();
            break;
        case 'f':
            if (!strncmp("media", optarg, strlen(optarg))) {
                if (msc_storage_forbid() < 0)
                    USB_LOG_ERR("MSC function has be forbidden.\n");
            }

            if (!strncmp("usb", optarg, strlen(optarg))) {
                if (msc_usbd_forbid() < 0)
                    USB_LOG_ERR("MSC function has be forbidden.\n");
            }
            break;
        case 'n':
            if (usbd_storage->is_forbidden == false) {
                USB_LOG_WRN("Please forbidden msc function.\n"
                            "such as: test_msc -f [usb] or [media]\n");
            }
            strcpy(msc_storage_path, optarg);
        case 'i':
            USB_LOG_INFO("MSC STORAGE PATH: %s\n", msc_storage_path);
            break;
        case'h':
        default:
            cmd_msc_usage(argv[0]);
            break;
        }
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_test_msc_mount, test_msc, Test USB MSC);

#endif
