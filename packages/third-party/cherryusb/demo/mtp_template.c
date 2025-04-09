/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usbd_mtp.h"

#define WCID_VENDOR_CODE 0x01

__ALIGN_BEGIN const uint8_t WCID_StringDescriptor_MSOS[18] __ALIGN_END = {
    ///////////////////////////////////////
    /// MS OS string descriptor
    ///////////////////////////////////////
    0x12,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    /* MSFT100 */
    'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00, /* wcChar_7 */
    '1', 0x00, '0', 0x00, '0', 0x00,            /* wcChar_7 */
    WCID_VENDOR_CODE,                           /* bVendorCode */
    0x00,                                       /* bReserved */
};

__ALIGN_BEGIN const uint8_t WINUSB_WCIDDescriptor[40] __ALIGN_END = {
    ///////////////////////////////////////
    /// WCID descriptor
    ///////////////////////////////////////
    0x28, 0x00, 0x00, 0x00,                   /* dwLength */
    0x00, 0x01,                               /* bcdVersion */
    0x04, 0x00,                               /* wIndex */
    0x01,                                     /* bCount */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* bReserved_7 */

    ///////////////////////////////////////
    /// WCID function descriptor
    ///////////////////////////////////////
    0x00, /* bFirstInterfaceNumber */
    0x01, /* bReserved */
    /* MTP */
    'M', 'T', 'P', 0x00, 0x00, 0x00, 0x00, 0x00, /* cCID_8 */
    /*  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* cSubCID_8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             /* bReserved_6 */
};

__ALIGN_BEGIN const uint8_t WINUSB_IF0_WCIDProperties[142] __ALIGN_END = {
  ///////////////////////////////////////
  /// WCID property descriptor
  ///////////////////////////////////////
  0x8e, 0x00, 0x00, 0x00,                           /* dwLength */
  0x00, 0x01,                                       /* bcdVersion */
  0x05, 0x00,                                       /* wIndex */
  0x01, 0x00,                                       /* wCount */

  ///////////////////////////////////////
  /// registry propter descriptor
  ///////////////////////////////////////
  0x84, 0x00, 0x00, 0x00,                           /* dwSize */
  0x01, 0x00, 0x00, 0x00,                           /* dwPropertyDataType */
  0x28, 0x00,                                       /* wPropertyNameLength */
  /* DeviceInterfaceGUID */
  'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00,       /* wcName_20 */
  'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00,       /* wcName_20 */
  't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00,       /* wcName_20 */
  'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00,       /* wcName_20 */
  'U', 0x00, 'I', 0x00, 'D', 0x00, 0x00, 0x00,      /* wcName_20 */
  0x4e, 0x00, 0x00, 0x00,                           /* dwPropertyDataLength */

  /* {1D4B2365-4749-48EA-B38A-7C6FDDDD7E26} */
  '{', 0x00, '1', 0x00, 'D', 0x00, '4', 0x00,       /* wcData_39 */
  'B', 0x00, '2', 0x00, '3', 0x00, '6', 0x00,       /* wcData_39 */
  '5', 0x00, '-', 0x00, '4', 0x00, '7', 0x00,       /* wcData_39 */
  '4', 0x00, '9', 0x00, '-', 0x00, '4', 0x00,       /* wcData_39 */
  '8', 0x00, 'E', 0x00, 'A', 0x00, '-', 0x00,       /* wcData_39 */
  'B', 0x00, '3', 0x00, '8', 0x00, 'A', 0x00,       /* wcData_39 */
  '-', 0x00, '7', 0x00, 'C', 0x00, '6', 0x00,       /* wcData_39 */
  'F', 0x00, 'D', 0x00, 'D', 0x00, 'D', 0x00,       /* wcData_39 */
  'D', 0x00, '7', 0x00, 'E', 0x00, '2', 0x00,       /* wcData_39 */
  '6', 0x00, '}', 0x00, 0x00, 0x00,                 /* wcData_39 */

};


struct usb_msosv1_descriptor msosv1_desc = {
    .string = (uint8_t *)WCID_StringDescriptor_MSOS,
    .vendor_code = WCID_VENDOR_CODE,
    .compat_id = (uint8_t *)WINUSB_WCIDDescriptor,
    .comp_id_property = (const uint8_t **)&WINUSB_IF0_WCIDProperties,
};

/*!< endpoint address */
#define MTP_IN_EP  0x81
#define MTP_OUT_EP 0x02
#define MTP_INT_EP 0x83

#define USBD_VID           0x33C3
#define USBD_PID           0xFFFF
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

/*!< config descriptor size */
#define USB_CONFIG_SIZE (9 + MTP_DESCRIPTOR_LEN)

#ifdef CONFIG_USB_HS
#define MTP_MAX_MPS 512
#else
#define MTP_MAX_MPS 64
#endif

const uint8_t mtp_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_1, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0201, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    MTP_DESCRIPTOR_INIT(0x00, MTP_OUT_EP, MTP_IN_EP, MTP_INT_EP, MTP_MAX_MPS, 0x02),
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
    'T', 0x00,                  /* wcChar11 */
    'P', 0x00,                  /* wcChar12 */
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
    '1', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '3', 0x00,                  /* wcChar5 */
    '1', 0x00,                  /* wcChar6 */
    '0', 0x00,                  /* wcChar7 */
    '0', 0x00,                  /* wcChar8 */
    '0', 0x00,                  /* wcChar9 */
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

uint8_t bos_descriptor[] = {
    0x05, 0x0f, 0x16, 0x00, 0x02,
    0x07, 0x10, 0x02, 0x06, 0x00, 0x00, 0x00,
    0x0a, 0x10, 0x03, 0x00, 0x0f, 0x00, 0x01, 0x01, 0xf4, 0x01
};

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
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}


#include <string.h>
#include <string.h>
#include "ff.h"			/* Declarations of FatFs API */
/* ELM FatFs provide a DIR struct */
#define HAVE_DIR_STRUCTURE
#include <dfs_fs.h>
#include <dfs_file.h>
#include <dfs_elm.h>
#include "diskio.h"		/* Declarations of device I/O functions */
#if defined(KERNEL_RTTHREAD)
#include "rtthread.h"
#else
#include "dfs_bare.h"
#endif
struct usbd_interface intf0;

struct usb_bos_descriptor bos_desc = {
    .string = bos_descriptor,
    .string_len = 22
};

struct usbd_mtp_fs_p {
    struct dfs_filesystem *fs;
    struct statfs stat; /* 1. block size 2. total data blocks in file system 3. free blocks in file system*/
    char fs_path[10];
    char fs_type[5];
};

struct usbd_mtp_fs_p usbd_mtp_fs;
struct usbd_mtp_fs_p *get_usbd_mtp_fs(void)
{
    return &usbd_mtp_fs;
}

int usbd_mtp_get_cap(uint64_t *max_capability, uint64_t *free_space)
{
    int ret;
    struct usbd_mtp_fs_p *usbd_mtp_fs = get_usbd_mtp_fs();

    ret = dfs_statfs(usbd_mtp_fs->fs_path, &usbd_mtp_fs->stat);
    if (ret < 0)
        return -1;

    *max_capability = (uint64_t)usbd_mtp_fs->stat.f_bsize * (uint64_t)usbd_mtp_fs->stat.f_blocks;
    *free_space = (uint64_t)usbd_mtp_fs->stat.f_bsize * (uint64_t)usbd_mtp_fs->stat.f_bfree;

    USB_LOG_DBG("max_capability:%lld free_space:%lld\n", *max_capability, *free_space);
    return 0;
}

#include "stdio.h"
#include "fcntl.h"
#include "dirent.h"
#include "unistd.h"
/* directory operation */
int usbd_mtp_make_dir(const char *path)
{
    return mkdir(path, 0);
}

int usbd_mtp_remove_dir(const char *path)
{
    return rmdir(path);
}

void *usbd_mtp_open_dir(const char *path)
{
    return opendir(path);
}

int usbd_mtp_close_dir(void *d)
{
    DIR *dir = (DIR *)d;
    return closedir(dir);
}

int usbd_mtp_get_file_info(void *d,
                            char *file_name,
                            uint8_t *file_name_len,
                            uint32_t *protection_status)
{
    DIR *dir = (DIR *)d;
    struct dirent *dirent;

    dirent = readdir(dir);
    if (dirent == NULL)
        return 0;

    *file_name_len = dirent->d_namlen;
    strcpy(file_name, dirent->d_name);
    *protection_status = 0;

    return 1; /* if still read the file, please return 1 */
}

/* directory operation */
int usbd_mtp_creat_file(const char *path)
{
    return creat(path, O_RDWR);
}

int usbd_mtp_unlink_file(const char *pathname)
{
    return unlink(pathname);
}

int usbd_mtp_open_file_rdonly(const char *file)
{
    return open(file, O_RDONLY);
}

int usbd_mtp_open_file_wronly(const char *file)
{
    return open(file, O_WRONLY);
}

int usbd_mtp_close_file(int fd)
{
    return close(fd);
}

int usbd_mtp_read_file(int fd, void *buf, size_t len)
{
    return read(fd, (char *)buf, len);
}

int usbd_mtp_write_file(int fd, const void *buf, size_t len)
{
    return write(fd, buf, len);
}

uint32_t usbd_mtp_get_file_size(const char *file)
{
    struct stat s;
    if(stat(file, &s) < 0)
        return -1;

    return s.st_size;
}

int usbd_mtp_get_fullpath(char *buf, const char *directory, const char *filename)
{
    char *fullpath;
    fullpath = dfs_normalize_path(directory, filename);
    strcpy(buf, fullpath);
    rt_free(fullpath);
    return 0;
}

extern struct dfs_filesystem filesystem_table[];

int usbd_mtp_fs_init(char *path)
{
    struct dfs_filesystem *iter;
    struct usbd_mtp_fs_p *usbd_mtp_fs = get_usbd_mtp_fs();

    for (iter = &filesystem_table[0];
            iter < &filesystem_table[DFS_FILESYSTEMS_MAX]; iter++) {
        if ((iter != NULL) && (iter->path != NULL)) {
            if (strcmp(iter->path, path) == 0)
                goto __init;
        }
    }
    USB_LOG_WRN("Invalid file system!\n");
    return -1;
__init:
    usbd_mtp_fs->fs = dfs_filesystem_lookup(path);
    strcpy(usbd_mtp_fs->fs_type, iter->ops->name);
    strcpy(usbd_mtp_fs->fs_path, iter->path);
    return 0;
}

void mtp_init(void)
{

    usbd_mtp_fs_init(ROOT_PATH);
    usbd_desc_register(mtp_descriptor);
    usbd_msosv1_desc_register(&msosv1_desc);
    usbd_bos_desc_register(&bos_desc);
    usbd_add_interface(usbd_mtp_init_intf(&intf0, MTP_OUT_EP, MTP_IN_EP, MTP_INT_EP));
    usbd_initialize();
}

#if defined(KERNEL_RTTHREAD)
rt_thread_t usbd_mtp_tid;

static void usbd_mtp_detection_thread(void *parameter)
{
    // waiting to mount the correspongding file system
    rt_thread_mdelay(800);
    mtp_init();

}

int usbd_mtp_detection(void)
{
    usbd_mtp_tid = rt_thread_create("usbd_mtp_detection", usbd_mtp_detection_thread, RT_NULL,
                           1536, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (usbd_mtp_tid != RT_NULL)
        rt_thread_startup(usbd_mtp_tid);
    else
        printf("create usbd_mtp_detection thread err!\n");

    return RT_EOK;
}
INIT_APP_EXPORT(usbd_mtp_detection);
#endif

