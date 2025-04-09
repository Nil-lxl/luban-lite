/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usbd_mtp.h"
#include "usbd_mtp_config.h"
#include "usb_osal.h"
#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <rtdevice.h>
#endif
/* Max USB packet size */
#ifndef CONFIG_USB_HS
#define MTP_BULK_EP_MPS 64
#else
#define MTP_BULK_EP_MPS 512
#endif
#define DATA_BUFFER_SIZE CONFIG_USBDEV_MTP_MAX_BUFSIZE
#define MAX_WITTE_FILE_SIZE CONFIG_USBDEV_MTP_MAX_BUFSIZE
#define MTP_OUT_EP_IDX 0
#define MTP_IN_EP_IDX  1
#define MTP_INT_EP_IDX 2

/* MTP Stage */
enum Stage {
    MTP_READ_COMMAND = 0,
    MTP_DATA_OUT = 1,
    MTP_DATA_IN = 2,
    MTP_SEND_RESPONSE = 3,
    MTP_WAIT_RESPONSE = 4,
};

USB_NOCACHE_RAM_SECTION struct usbd_mtp_priv {
    USB_MEM_ALIGNX struct mtp_container_command con_command;
    USB_MEM_ALIGNX struct mtp_container_data con_data;
    USB_MEM_ALIGNX struct mtp_container_response con_response;
    enum Stage stage;
    uint8_t session_state;
    uint32_t response_code;
    /*-----priv-----*/
    char path[512];
    uint32_t handle_counter;
    struct mtp_object *mtp_objects;
    USB_MEM_ALIGNX uint8_t usbd_mtp_data_in[CONFIG_USBDEV_MTP_MAX_BUFSIZE];
    USB_MEM_ALIGNX uint8_t usbd_mtp_data_out[CONFIG_USBDEV_MTP_MAX_BUFSIZE];
    uint32_t usbd_mtp_rcnt; //total data
    uint32_t usbd_mtp_wcnt;
    struct mtp_file mtp_file_in; // file data
    struct mtp_file mtp_file_out;
#if defined(CONFIG_USBDEV_MTP_THREAD)
    usb_osal_mq_t usbd_mtp_mq;
    usb_osal_thread_t usbd_mtp_thread;
    uint32_t nbytes;
#endif
} g_usbd_mtp;


/* Describe EndPoints configuration */
static struct usbd_endpoint mtp_ep_data[3];
struct mtp_object mtp_objects[MTP_OBJECT_HANDLES_MAX_NUM];

static int mtp_class_interface_request_handler(struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    USB_LOG_DBG("MTP Class request: "
                "bRequest 0x%02x\r\n",
                setup->bRequest);

    switch (setup->bRequest) {
        case MTP_REQUEST_CANCEL:
            break;
        case MTP_REQUEST_GET_EXT_EVENT_DATA:
            break;
        case MTP_REQUEST_RESET:
            break;
        case MTP_REQUEST_GET_DEVICE_STATUS:
            *(uint16_t *)(*data) = 0x08; //len
            *(uint16_t *)(*data + 2) = MTP_RESPONSE_OK; // resepone
            *len = 8;
            break;

        default:
            USB_LOG_WRN("Unhandled MTP Class bRequest 0x%02x\r\n", setup->bRequest);
            return -1;
    }

    return 0;
}

static int usbd_mtp_get_object_by_handle(struct mtp_object **object, uint32_t handle)
{
    if (handle < 0 || handle > MTP_OBJECT_HANDLES_MAX_NUM) {
        USB_LOG_ERR("Failed to get object:%ld(%d)\n", handle, MTP_OBJECT_HANDLES_MAX_NUM);
        return -1;
    }
    *object = &mtp_objects[handle];
    return 0;
}

static int usbd_mtp_creat_object(struct mtp_object **object, uint32_t *index)
{
    uint32_t handle_index;
    if (g_usbd_mtp.handle_counter >= MTP_OBJECT_HANDLES_MAX_NUM) {
        USB_LOG_ERR("The maximum number of files has been reached !\r\n");
        return -1;
    }
    for (handle_index = 1U; handle_index < MTP_OBJECT_HANDLES_MAX_NUM; handle_index++) {
        if (mtp_objects[handle_index].handle == 0U) {
            *object = &mtp_objects[handle_index];
            *index = handle_index;
            g_usbd_mtp.handle_counter++;
            break;
        }
    }
    return 0;
}

static int usbd_mtp_unlink_object(struct mtp_object *object)
{
    int ret;
    uint32_t child;
    if (object->property.format != MTP_OBJ_FORMAT_ASSOCIATION) {
        ret = usbd_mtp_unlink_file((char *)object->property.file_full_name);
        if (ret < 0)
            return -1;
    } else {
        for (child = 1; child < MTP_OBJECT_HANDLES_MAX_NUM; child++) {
            if (mtp_objects[child].property.parent_object == object->handle)
                usbd_mtp_unlink_object(&mtp_objects[child]);
        }
        usbd_mtp_remove_dir((char *)object->property.file_full_name);
    }
    memset(object, 0U, sizeof(struct mtp_object));
    g_usbd_mtp.handle_counter--;
    return 0;
}

static void usbd_mtp_send_response_param(uint32_t code, uint32_t param1,
                                    uint32_t param2, uint32_t param3,
                                    uint32_t param4, uint32_t param5)
{
    USB_LOG_DBG("Send response\r\n");

    g_usbd_mtp.stage = MTP_WAIT_RESPONSE;

    g_usbd_mtp.con_response.conlen = 32;
    g_usbd_mtp.con_response.contype = MTP_CONTAINER_TYPE_RESPONSE;
    g_usbd_mtp.con_response.code = code;
    g_usbd_mtp.con_response.trans_id = g_usbd_mtp.con_command.trans_id;
    g_usbd_mtp.con_response.param1 = (uint32_t)param1;
    g_usbd_mtp.con_response.param2 = (uint32_t)param2;
    g_usbd_mtp.con_response.param3 = (uint32_t)param3;
    g_usbd_mtp.con_response.param4 = (uint32_t)param4;
    g_usbd_mtp.con_response.param5 = (uint32_t)param5;

    usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr, (uint8_t *)&g_usbd_mtp.con_response, 32);
}

static void usbd_mtp_send_response(uint32_t code)
{
    USB_LOG_DBG("Send response\r\n");

    g_usbd_mtp.stage = MTP_WAIT_RESPONSE;

    g_usbd_mtp.con_response.conlen = CONFIG_MTP_COMMAND_LEN;
    g_usbd_mtp.con_response.contype = MTP_CONTAINER_TYPE_RESPONSE;
    g_usbd_mtp.con_response.code = code;
    g_usbd_mtp.con_response.trans_id = g_usbd_mtp.con_command.trans_id;

    usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr,
                        (uint8_t *)&g_usbd_mtp.con_response,
                        CONFIG_MTP_COMMAND_LEN);
}

static void usbd_mtp_send_info(uint8_t *data, uint32_t len)
{
    USB_LOG_DBG("Send info\r\n");

    g_usbd_mtp.stage = MTP_SEND_RESPONSE;

    g_usbd_mtp.con_data.conlen = CONFIG_MTP_COMMAND_LEN + len;
    g_usbd_mtp.con_data.contype = MTP_CONTAINER_TYPE_DATA;
    g_usbd_mtp.con_data.code = MTP_RESPONSE_OK;
    g_usbd_mtp.con_data.trans_id = g_usbd_mtp.con_command.trans_id;

    memcpy(g_usbd_mtp.con_data.data, data, len);
    usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr,
                        (uint8_t *)&g_usbd_mtp.con_data,
                        CONFIG_MTP_COMMAND_LEN + len);
}

static void usbd_mtp_get_device_info(void)
{
    struct mtp_device_info device_info;
    uint16_t i;

    device_info.StandardVersion = 100;
    device_info.VendorExtensionID = 0x06;
    device_info.VendorExtensionVersion = 100;
    device_info.VendorExtensionDesc_len = (uint8_t)CONFIG_MTP_VEND_EXT_DESC_LEN;
    for (i = 0; i < CONFIG_MTP_VEND_EXT_DESC_LEN; i++) {
        device_info.VendorExtensionDesc[i] = VendExtDesc[i];
    }

    /* device supports one mode , standard mode */
    device_info.FunctionalMode = 0x0000;

    /* All supported operation */
    device_info.OperationsSupported_len = CONFIG_MTP_SUPP_OP_LEN;
    for (i = 0U; i < CONFIG_MTP_SUPP_OP_LEN; i++) {
        device_info.OperationsSupported[i] = SuppOP[i];
    }

    /* event that are currently generated by the device*/
    device_info.EventsSupported_len = CONFIG_MTP_SUPP_EVENTS_LEN;

    for (i = 0U; i < CONFIG_MTP_SUPP_EVENTS_LEN; i++) {
        device_info.EventsSupported[i] = SuppEvents[i];
    }

    device_info.DevicePropertiesSupported_len = CONFIG_MTP_SUPP_DEVICE_PROP_LEN;

    for (i = 0U; i < CONFIG_MTP_SUPP_DEVICE_PROP_LEN; i++) {
        device_info.DevicePropertiesSupported[i] = DevicePropSupp[i];
    }

    device_info.CaptureFormats_len = CONFIG_MTP_SUPP_CAPT_FORMAT_LEN;

    for (i = 0U; i < CONFIG_MTP_SUPP_CAPT_FORMAT_LEN; i++) {
        device_info.CaptureFormats[i] = SuppCaptFormat[i];
    }
    /* number of image formats that are supported by the device*/
    device_info.ImageFormats_len = CONFIG_MTP_SUPP_IMG_FORMAT_LEN;
    for (i = 0U; i < CONFIG_MTP_SUPP_IMG_FORMAT_LEN; i++) {
        device_info.ImageFormats[i] = SuppImgFormat[i];
    }

    device_info.Manufacturer_len = (uint8_t)CONFIG_MTP_MANUF_LEN;
    for (i = 0U; i < CONFIG_MTP_MANUF_LEN; i++) {
        device_info.Manufacturer[i] = Manuf[i];
    }

    device_info.Model_len = (uint8_t)CONFIG_MTP_MODEL_LEN;
    for (i = 0U; i < CONFIG_MTP_MODEL_LEN; i++) {
        device_info.Model[i] = Model[i];
    }

    device_info.DeviceVersion_len = (uint8_t)CONFIG_MTP_DEVICE_VERSION_LEN;
    for (i = 0U; i < CONFIG_MTP_DEVICE_VERSION_LEN; i++) {
        device_info.DeviceVersion[i] = DeviceVers[i];
    }

    device_info.SerialNumber_len = (uint8_t)CONFIG_MTP_SERIAL_NBR_LEN;
    for (i = 0U; i < CONFIG_MTP_SERIAL_NBR_LEN; i++) {
        device_info.SerialNumber[i] = SerialNbr[i];
    }

    usbd_mtp_send_info((uint8_t *)&device_info, sizeof(struct mtp_device_info));
}

static void usbd_mtp_open_session(void)
{
    usbd_mtp_send_response(MTP_RESPONSE_OK);
}

static void usbd_mtp_get_storage_ids(void)
{
    struct mtp_storage_id storage_id;

    storage_id.StorageIDS_len = CONFIG_MTP_STORAGE_ID_LEN;
    storage_id.StorageIDS[0] = MTP_STORAGE_ID;

    usbd_mtp_send_info((uint8_t *)&storage_id, sizeof(struct mtp_storage_id));
}

static void usbd_mtp_get_storage_info(void)
{
    struct mtp_storage_info storage_info;
    uint64_t MaxCapability, FreeSpaceInBytes;

    storage_info.StorageType = MTP_STORAGE_REMOVABLE_RAM;
    storage_info.FilesystemType = MTP_FILESYSTEM_GENERIC_FLAT;
    storage_info.AccessCapability = MTP_ACCESS_CAP_RW;
    if (usbd_mtp_get_cap(&MaxCapability, &FreeSpaceInBytes))
        USB_LOG_ERR("Failed to get storage info \r\n");
    storage_info.MaxCapability = MaxCapability;
    storage_info.FreeSpaceInBytes = FreeSpaceInBytes;
    storage_info.FreeSpaceInObjects = 0xFFFFFFFFU;
    storage_info.StorageDescription = 0U;
    storage_info.VolumeLabel = 0U;

    usbd_mtp_send_info((uint8_t *)&storage_info, sizeof(struct mtp_storage_info));
}

uint16_t _get_format_by_name(char *file_name)
{
    uint32_t objformat;
    char file_ext[5];

    memset(file_ext, 0, sizeof(file_ext));

    char* ext = strrchr(file_name, '.');

    /* Get file type */
    if (ext != 0U && (strlen(ext + 1) <= 5)) {
        strcpy(file_ext, (ext + 1));
    } else {
        objformat = MTP_OBJ_FORMAT_ASSOCIATION;
        return objformat;
    }

    if ((strcmp(file_ext, "TXT") == 0) || (strcmp(file_ext, "txt") == 0)) {
        objformat = MTP_OBJ_FORMAT_TEXT;
    } else if ((strcmp(file_ext, "JPG") == 0) || (strcmp(file_ext, "jpg") == 0)) {
        objformat = MTP_OBJ_FORMAT_EXIF_JPEG;
    } else if ((strcmp(file_ext, "BMP") == 0) || (strcmp(file_ext, "bmp") == 0)) {
        objformat = MTP_OBJ_FORMAT_BMP;
    } else if ((strcmp(file_ext, "PNG") == 0) || (strcmp(file_ext, "png") == 0)) {
        objformat = MTP_OBJ_FORMAT_PNG;
    } else if ((strcmp(file_ext, "MP4") == 0) || (strcmp(file_ext, "mp4") == 0)) {
        objformat = MTP_OBJ_FORMAT_MP4_CONTAINER;
    } else if ((strcmp(file_ext, "WAV") == 0) || (strcmp(file_ext, "wav") == 0)) {
        objformat = MTP_OBJ_FORMAT_WAV;
    } else if ((strcmp(file_ext, "PDF") == 0) || (strcmp(file_ext, "pdf") == 0)) {
        objformat = 0x3000U;
    } else {
        objformat = 0x3000U;
    }

    /* Return object format */
    return objformat;
}

void _string_to_unicode(void *dest, void *src, int len)
{
    uint16_t *dest_str = (uint16_t *)dest;
    uint8_t *src_str = (uint8_t *)src;

    for (int i = 0; i < len; i++) {
        *dest_str = (uint16_t)*src_str;
        dest_str++;
        src_str++;
    }
}

void _unicode_to_string(void *dest, void *src, int len)
{
    uint8_t *dest_str = (uint8_t *)dest;
    uint16_t *src_str = (uint16_t *)src;

    for (int i = 0; i < len; i++) {
        *dest_str = (uint8_t)*src_str;
        dest_str++;
        src_str++;
    }
}

static int mtp_object_handles_list(struct mtp_object_handle *object_handle,
                                uint32_t parent_handle, const char *pathname)
{
    uint32_t i = 0, index = 0, size = 0, protection_status;
    char *path = (char*)pathname;
    char file_name[255], fullpath[255];
    void *dirent = NULL;
    uint8_t file_name_len;
    struct mtp_object *object = NULL;

    dirent = usbd_mtp_open_dir(pathname);
    if (dirent == NULL) {
        USB_LOG_DBG("Failed to open %s\r\n", pathname);
        return -1;
    }

    while(usbd_mtp_get_file_info(dirent, file_name,
            &file_name_len, &protection_status)) {
        /* Get file information */
        usbd_mtp_get_fullpath(fullpath, path, file_name);
        if (fullpath == NULL)
            return -1;
        USB_LOG_DBG("fullpath:%s\r\n", fullpath);

        size = usbd_mtp_get_file_size(fullpath);
        if (size < 0)
            break;

        /* Creat mtp object */
        if (usbd_mtp_creat_object(&object, &index) < 0)
            goto __finsh;
        object->handle = (uint32_t)(index);
        /* property */
        object->property.storage_id = MTP_STORAGE_ID;
        object->property.format = (uint16_t)_get_format_by_name(file_name);
        object->property.protection_status = protection_status;
        object->property.size = size;
        object->property.file_name_length = file_name_len;
        strcpy((char *)object->property.file_name, (char *)file_name);
        object->property.file_full_name_length = strlen((char *)fullpath);
        strcpy((char *)object->property.file_full_name, (char *)fullpath);
        object->property.parent_object = parent_handle;
        object->property.identifier[0] = 0U;
        /* Add  to ObjectHandleList */
        object_handle->ObjectHandle[i] = (uint32_t)(index);

        USB_LOG_DBG("file:%s(%ld-%ld-%ld-%ld)\r\n", object->property.file_full_name,
                        object->handle, object_handle->ObjectHandle[i], g_usbd_mtp.handle_counter, i);
        memset(file_name, 0, sizeof(file_name));
        memset(fullpath, 0, sizeof(fullpath));
        if (i >= CONFIG_USBDEV_MTP_GET_MAX_HANDLES)
            goto __finsh;
        i++;
    }

__finsh:
    object_handle->ObjectHandle_len = i;
    usbd_mtp_close_dir(dirent);

    return 0;
}

static void usbd_mtp_get_object_handles(void)
{
    struct mtp_object_handle object_handle;
    struct mtp_object *object = NULL;
    int ret;

    memset(&object_handle, 0U, sizeof(struct mtp_object_handle));

    if (g_usbd_mtp.con_command.param1 != MTP_STORAGE_ID &&
        g_usbd_mtp.con_command.param1 != 0xffffffff) {
        usbd_mtp_send_response(MTP_RESPONSE_STORE_NOT_AVAILABLE);
        return;
    }

    if (g_usbd_mtp.con_command.param2 != 0x00000000) {
        usbd_mtp_send_response(MTP_RESPONSE_SPECIFICATION_BY_FORMAT_NOT_SUPPORTED);
        return;
    }

    if (g_usbd_mtp.con_command.param3 == 0x00000000 ||
        g_usbd_mtp.con_command.param3 == 0xffffffff) {
        ret = mtp_object_handles_list(&object_handle, 0U, ROOT_PATH);
        if (ret < 0) {
            usbd_mtp_send_response(MTP_RESPONSE_INVALID_OBJECT_HANDLE);
            return;
        }
    } else {
        usbd_mtp_get_object_by_handle(&object, g_usbd_mtp.con_command.param3);
        ret = mtp_object_handles_list(&object_handle, g_usbd_mtp.con_command.param3,
                                        (char *)object->property.file_full_name);
        if (ret < 0) {
            usbd_mtp_send_response(MTP_RESPONSE_INVALID_OBJECT_HANDLE);
            return;
        }
    }

    usbd_mtp_send_info((uint8_t *)&object_handle, sizeof(uint32_t) * (object_handle.ObjectHandle_len + 1U));
}

struct mtp_object_info object_info;
static void usbd_mtp_get_object_info(void)
{
    struct mtp_object *object = NULL;

    usbd_mtp_get_object_by_handle(&object, (uint32_t)g_usbd_mtp.con_command.param1);
    if (object == NULL) {
        USB_LOG_ERR("Invalid Object!\r\n");
        usbd_mtp_send_response(MTP_RESPONSE_INVALID_OBJECT_HANDLE);
        return;
    }
    memset(&object_info, 0, sizeof(struct mtp_object_info));
    object_info.Storage_id = MTP_STORAGE_ID;
    object_info.ObjectFormat = object->property.format;
    object_info.ProtectionStatus = 0U;
    object_info.ObjectCompressedSize = object->property.size;
    object_info.ThumbFormat = MTP_OBJ_FORMAT_UNDEFINED;
    object_info.ParentObject = object->property.parent_object;
    if (object->property.format == MTP_OBJ_FORMAT_ASSOCIATION)
        object_info.AssociationType = 0x0001; /* generic folder */
    else
        object_info.AssociationType = 0U;
    object_info.Filename_len = object->property.file_name_length;
    _string_to_unicode(object_info.Filename,
                        object->property.file_name,
                        object->property.file_name_length);

    usbd_mtp_send_info((uint8_t *)&object_info, sizeof(struct mtp_object_info));
}

static void usbd_mtp_get_object(void)
{
    int fd;
    int32_t ret;
    struct mtp_object *object = NULL;

    ret = usbd_mtp_get_object_by_handle(&object, g_usbd_mtp.con_command.param1);
    if (ret < 0) {
        USB_LOG_ERR("Faile to get flie(%s)\r\n", (char *)object->property.file_full_name);
        usbd_mtp_send_response(MTP_RESPONSE_INVALID_OBJECT_HANDLE);
        return;
    }
    if (object->property.format == MTP_OBJ_FORMAT_ASSOCIATION) {
        USB_LOG_ERR("The object is a folder\r\n");
        usbd_mtp_send_response(MTP_RESPONSE_INVALID_OBJECT_HANDLE);
        return;
    }

    /* Read */
    fd = usbd_mtp_open_file_rdonly((char *)object->property.file_full_name);
    if (fd < 0) {
        USB_LOG_ERR("Faile to open file(%s)\r\n", object->property.file_full_name);
        usbd_mtp_send_response(MTP_RESPONSE_INCOMPLETE_TRANSFER);
        return;
    }

    int32_t data_length = object->property.size;

    /* first packet */
    ret = usbd_mtp_read_file(fd, g_usbd_mtp.con_data.data,
                            MIN(data_length, CONFIG_USBDEV_MTP_MAX_BUFSIZE - CONFIG_MTP_COMMAND_LEN));
    g_usbd_mtp.con_data.conlen = CONFIG_MTP_COMMAND_LEN + data_length;
    g_usbd_mtp.con_data.contype = MTP_CONTAINER_TYPE_DATA;
    g_usbd_mtp.con_data.code = MTP_RESPONSE_OK;
    g_usbd_mtp.con_data.trans_id = g_usbd_mtp.con_command.trans_id;

    data_length = ((data_length - ret) < 0) ? data_length : (data_length - ret);

    if (data_length == 0) {
        g_usbd_mtp.stage = MTP_SEND_RESPONSE;
        if (usbd_mtp_close_file(fd) < 0) {
            USB_LOG_ERR("Falie to close file(%s)\r\n", object->property.file_full_name);
            usbd_mtp_send_response(MTP_RESPONSE_INCOMPLETE_TRANSFER);
            return;
        }
    } else {
        g_usbd_mtp.stage = MTP_DATA_IN;
        g_usbd_mtp.mtp_file_in.handle = g_usbd_mtp.con_command.param1;
        g_usbd_mtp.mtp_file_in.fd = fd;
        g_usbd_mtp.mtp_file_in.data_length = data_length;
    }
    usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr,
                        (uint8_t *)&g_usbd_mtp.con_data,
                        MIN(g_usbd_mtp.con_data.conlen, CONFIG_USBDEV_MTP_MAX_BUFSIZE));

    return;
}

void usbd_mtp_data_in(void)
{
    int32_t ret;
    int32_t data_length = g_usbd_mtp.mtp_file_in.data_length;

    if (data_length <= 0)
        return;

    ret = usbd_mtp_read_file(g_usbd_mtp.mtp_file_in.fd,
                                g_usbd_mtp.usbd_mtp_data_in,
                                MIN(data_length, CONFIG_USBDEV_MTP_MAX_BUFSIZE));
    if (ret < 0) {
        USB_LOG_ERR("Failed to read file\r\n");
        if (usbd_mtp_close_file(g_usbd_mtp.mtp_file_in.fd) < 0) {
            USB_LOG_ERR("Falie to close file\r\n");
        }
        usbd_mtp_send_response(MTP_RESPONSE_INCOMPLETE_TRANSFER);
        return;
    }

    data_length = ((data_length - ret) < 0) ? data_length : (data_length - ret);

    g_usbd_mtp.mtp_file_in.data_length = data_length;

    if (g_usbd_mtp.mtp_file_in.data_length == 0) {
        if (usbd_mtp_close_file(g_usbd_mtp.mtp_file_in.fd) < 0) {
            USB_LOG_ERR("Falie to close file\r\n");
            return;
        }
        g_usbd_mtp.stage = MTP_SEND_RESPONSE;
    }

    usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr,
                        g_usbd_mtp.usbd_mtp_data_in, ret);

    return;
}

static void usbd_mtp_get_object_prop_desc(void)
{
    struct mtp_object_prop_desc object_prop_desc;
    struct mtp_string mtp_filename;
    uint16_t undef_format = MTP_OBJ_FORMAT_UNDEFINED;
    uint32_t storageid = MTP_STORAGE_ID;
    USB_LOG_DBG("param:%#lx(1)-%#lx(2)\r\n", g_usbd_mtp.con_command.param1, g_usbd_mtp.con_command.param2);

    if (g_usbd_mtp.con_command.param2 == MTP_OBJ_FORMAT_UNDEFINED &&
        g_usbd_mtp.con_command.param2 == MTP_OBJ_FORMAT_ASSOCIATION) {
        usbd_mtp_send_response(MTP_RESPONSE_INVALID_OBJECT_FORMAT_CODE);
        return;
    }

    switch (g_usbd_mtp.con_command.param1) /* switch obj prop code */
    {
        case MTP_OB_PROP_STORAGE_ID:
            object_prop_desc.ObjectPropertyCode = (uint16_t)(g_usbd_mtp.con_command.param1);
            object_prop_desc.DataType = MTP_DATATYPE_UINT32;
            object_prop_desc.GetSet = MTP_PROP_GET;
            object_prop_desc.DefValue = (uint8_t *)&storageid;
            object_prop_desc.GroupCode = 0U;
            object_prop_desc.FormFlag = 0U;
            break;

        case MTP_OB_PROP_OBJECT_FORMAT:
            object_prop_desc.ObjectPropertyCode = (uint16_t)(g_usbd_mtp.con_command.param1);
            object_prop_desc.DataType = MTP_DATATYPE_UINT16;
            object_prop_desc.GetSet = MTP_PROP_GET;
            object_prop_desc.DefValue = (uint8_t *)&undef_format;
            object_prop_desc.GroupCode = 0U;
            object_prop_desc.FormFlag = 0U;
            break;

        case MTP_OB_PROP_PROTECTION_STATUS:
            object_prop_desc.ObjectPropertyCode = (uint16_t)(g_usbd_mtp.con_command.param1);
            object_prop_desc.DataType = MTP_DATATYPE_UINT16;
            object_prop_desc.GetSet = MTP_PROP_GET_SET;
            object_prop_desc.DefValue = 0U;
            object_prop_desc.GroupCode = 0U;
            object_prop_desc.FormFlag = 0U;
            break;

        case MTP_OB_PROP_OBJ_FILE_NAME:
            object_prop_desc.ObjectPropertyCode = (uint16_t)(g_usbd_mtp.con_command.param1);
            object_prop_desc.DataType = MTP_DATATYPE_STR;
            object_prop_desc.GetSet = MTP_PROP_GET_SET;
            mtp_filename.len = (uint8_t)(sizeof(DefaultFileName));
            _string_to_unicode(mtp_filename.string,
                                (char *)DefaultFileName,
                                mtp_filename.len);
            object_prop_desc.DefValue = (uint8_t *)&mtp_filename;
            object_prop_desc.GroupCode = 0U;
            object_prop_desc.FormFlag = 0U;
            break;

        case MTP_OB_PROP_PARENT_OBJECT:
            object_prop_desc.ObjectPropertyCode = (uint16_t)(g_usbd_mtp.con_command.param1);
            object_prop_desc.DataType = MTP_DATATYPE_UINT32;
            object_prop_desc.GetSet = MTP_PROP_GET;
            object_prop_desc.DefValue = 0U;
            object_prop_desc.GroupCode = 0U;
            object_prop_desc.FormFlag = 0U;
            break;

        case MTP_OB_PROP_OBJECT_SIZE:
            object_prop_desc.ObjectPropertyCode = (uint16_t)(g_usbd_mtp.con_command.param1);
            object_prop_desc.DataType = MTP_DATATYPE_UINT64;
            object_prop_desc.GetSet = MTP_PROP_GET;
            object_prop_desc.DefValue = 0U;
            object_prop_desc.GroupCode = 0U;
            object_prop_desc.FormFlag = 0U;
            break;

        case MTP_OB_PROP_NAME:
            object_prop_desc.ObjectPropertyCode = (uint16_t)(g_usbd_mtp.con_command.param1);
            object_prop_desc.DataType = MTP_DATATYPE_STR;
            object_prop_desc.GetSet = MTP_PROP_GET_SET;
            mtp_filename.len = (uint8_t)(sizeof(DefaultFileName));
            memcpy((void *)(mtp_filename.string),
                    (void *)DefaultFileName, sizeof(DefaultFileName));
            object_prop_desc.DefValue = (uint8_t *)&mtp_filename;
            object_prop_desc.GroupCode = 0U;
            object_prop_desc.FormFlag = 0U;
            break;

        case MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN:
            object_prop_desc.ObjectPropertyCode = (uint16_t)(g_usbd_mtp.con_command.param1);
            object_prop_desc.DataType = MTP_DATATYPE_UINT128;
            object_prop_desc.GetSet = MTP_PROP_GET;
            object_prop_desc.DefValue = 0U;
            object_prop_desc.GroupCode = 0U;
            object_prop_desc.FormFlag = 0U;
            break;
        default:
            break;
    }

    usbd_mtp_send_info((uint8_t *)&object_prop_desc, sizeof(struct mtp_object_prop_desc));
}

static void usbd_mtp_get_object_props_supported(void)
{
    struct mtp_object_props_support object_props_support;
    uint32_t i;

    object_props_support.ObjectPropCode_len = CONFIG_MTP_SUPP_OBJ_PROP_LEN;

    for (i = 0U; i < CONFIG_MTP_SUPP_OBJ_PROP_LEN; i++) {
        object_props_support.ObjectPropCode[i] = ObjectPropCode[i];
    }
    usbd_mtp_send_info((uint8_t *)&object_props_support, sizeof(struct mtp_object_props_support));
}

#if 0
static void usbd_mtp_get_object_prop_value(void)
{
    uint8_t *value = NULL;
    uint32_t length = 0U;
    struct mtp_string file_name;
    struct mtp_object *object = NULL;
    uint32_t object_handle = g_usbd_mtp.con_command.param1;
    uint32_t object_prop_code = g_usbd_mtp.con_command.param2;

    usbd_mtp_get_object_by_handle(&object, object_handle);

    switch(object_prop_code) {
        case MTP_OB_PROP_STORAGE_ID:
            length = sizeof(uint32_t);
            value = (uint8_t *)&object->property.storage_id;
            break;

        case MTP_OB_PROP_OBJECT_FORMAT:
            length = sizeof(uint16_t);
            value = (uint8_t *)&object->property.format;
            break;

        case MTP_OB_PROP_PROTECTION_STATUS:
            length = sizeof(uint16_t);
            value = (uint8_t *)&object->property.protection_status;
            break;

        case MTP_OB_PROP_OBJ_FILE_NAME:
            file_name.len = object->property.file_name_length;
            _string_to_unicode(file_name.string,
                        object->property.file_name,
                        object->property.file_name_length);
            length = file_name.len + 1;
            value = (uint8_t *)&file_name;
            break;

        case MTP_OB_PROP_PARENT_OBJECT:
            length = sizeof(uint16_t);
            value = (uint8_t *)&object->property.parent_object;
            break;

        case MTP_OB_PROP_OBJECT_SIZE:
            length = sizeof(uint16_t);
            value = (uint8_t *)&object->property.size;
            break;

        case MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN:
            break;
        default:
            usbd_mtp_send_response(MTP_RESPONSE_ACCESS_DENIED);
            return;
            break;
    }
    memcpy(g_usbd_mtp.usbd_mtp_data_out, (uint8_t *)value, length);

    usbd_mtp_send_info((uint8_t *)g_usbd_mtp.usbd_mtp_data_out, length);
}

static void usbd_mtp_set_object_prop_value(void)
{
    static uint32_t last_trans_id = 0x0, object_handle, object_prop_code;
    uint32_t length = 0U;
    struct mtp_object *object = NULL;
    if (g_usbd_mtp.con_command.trans_id != last_trans_id) {
        last_trans_id = g_usbd_mtp.con_command.trans_id;
        object_handle = g_usbd_mtp.con_command.param1;
        object_prop_code = g_usbd_mtp.con_command.param2;
        usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, (uint8_t *)&g_usbd_mtp.con_data, CONFIG_USBDEV_MTP_MAX_BUFSIZE);
        return;
    }
    usbd_mtp_get_object_by_handle(&object, object_handle);
    /* Object Prop Code */
    switch(object_prop_code) {
        case MTP_OB_PROP_STORAGE_ID:
            length = sizeof(uint32_t);
            memcpy(&object->property.storage_id,
                    g_usbd_mtp.con_data.data,
                    length);
            break;

        case MTP_OB_PROP_OBJECT_FORMAT:
            length = sizeof(uint16_t);
            memcpy(&object->property.format,
                    g_usbd_mtp.con_data.data,
                    length);
            break;

        case MTP_OB_PROP_PROTECTION_STATUS:
            length = sizeof(uint16_t);
            memcpy(&object->property.protection_status,
                    g_usbd_mtp.con_data.data,
                    length);
            break;

        case MTP_OB_PROP_OBJ_FILE_NAME:
            length = g_usbd_mtp.con_data.data[0];
            object->property.file_full_name_length = length;
            _unicode_to_string(object->property.file_name,
                                (char *)&g_usbd_mtp.con_data.data[1],
                                length);
            break;

        case MTP_OB_PROP_PARENT_OBJECT:
            length = sizeof(uint16_t);
            memcpy(&object->property.parent_object,
                    g_usbd_mtp.con_data.data,
                    length);
            break;

        case MTP_OB_PROP_OBJECT_SIZE:
            length = sizeof(uint16_t);
            memcpy(&object->property.size,
                    g_usbd_mtp.con_data.data,
                    length);
            break;

        case MTP_OB_PROP_NAME:
            length = object->property.file_name_length;
            memcpy(object->property.file_name,
                    g_usbd_mtp.con_data.data,
                    length);
            break;

        case MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN:
            break;
        default:
            usbd_mtp_send_response(MTP_RESPONSE_OK);
            break;
    }
    usbd_mtp_send_response(MTP_RESPONSE_OK);
}
#endif


#if 0
//todo
uint8_t usbd_mtp_dataset[RINGBUFFER_FIFO];
void usbd_mtp_get_object_prop_list(void)
{
    struct mtp_object_prop_list object_prop_list;

    uint32_t storageid = MTP_STORAGE_ID;
    uint32_t default_val = 0U;
    uint16_t format = MTP_OBJ_FORMAT_TEXT;
    uint64_t objsize = 3;
    uint32_t parent_proval = 4;
    uint32_t dataLength = sizeof(uint32_t);
    uint32_t element_len =0;

    object_prop_list.Properties_len = CONFIG_MTP_SUPP_OBJ_PROP_LEN;
    rt_memcpy(usbd_mtp_dataset, (uint8_t *)&object_prop_list.Properties_len, sizeof(uint32_t));

    USB_LOG_INFO("usbd_mtp_get_object_prop_list\n param:%#lx(1)-%#lx(2)", g_usbd_mtp.con_command.param1, g_usbd_mtp.con_command.param2);
    for (uint8_t i = 0U; i < CONFIG_MTP_SUPP_OBJ_PROP_LEN; i++) {
        object_prop_list.Properties[i].ObjectHandle = g_usbd_mtp.con_command.param1;
        switch (ObjectPropCode[i]) {
            case MTP_OB_PROP_STORAGE_ID:
                object_prop_list.Properties[i].PropertyCode = MTP_OB_PROP_STORAGE_ID;
                object_prop_list.Properties[i].Datatype = MTP_DATATYPE_UINT32;
                object_prop_list.Properties[i].propval.u32 = (uint32_t)storageid;

                /* Copy object property code to the payload data */
                element_len = sizeof(object_prop_list.Properties[i]) - 8U + sizeof(uint32_t);
                rt_memcpy(usbd_mtp_dataset + dataLength, &object_prop_list.Properties[i],
                        element_len);
                break;

            case MTP_OB_PROP_OBJECT_FORMAT:
                object_prop_list.Properties[i].PropertyCode = MTP_OB_PROP_OBJECT_FORMAT;
                object_prop_list.Properties[i].Datatype = MTP_DATATYPE_UINT16;
                object_prop_list.Properties[i].propval.u16 = (uint16_t)format;

                /* Copy object property code to the payload data */
                element_len = sizeof(object_prop_list.Properties[i]) - 8U + sizeof(uint16_t);
                rt_memcpy(usbd_mtp_dataset + dataLength, &object_prop_list.Properties[i],
                        element_len);
                break;

            case MTP_OB_PROP_OBJECT_SIZE:
                object_prop_list.Properties[i].PropertyCode = MTP_OB_PROP_OBJECT_SIZE;
                object_prop_list.Properties[i].Datatype = MTP_DATATYPE_UINT64;
                object_prop_list.Properties[i].propval.u64 = (uint64_t)objsize;

                element_len = sizeof(object_prop_list.Properties[i]) - 8U + sizeof(uint16_t);
                rt_memcpy(usbd_mtp_dataset + dataLength, &object_prop_list.Properties[i],
                        element_len);
                break;

            case MTP_OB_PROP_OBJ_FILE_NAME:
                object_prop_list.Properties[i].PropertyCode = MTP_OB_PROP_OBJ_FILE_NAME;
                object_prop_list.Properties[i].Datatype = MTP_DATATYPE_STR;
                mtp_filename.len = CONFIG_MTP_FILE_NAME_LEN;
                memcpy(mtp_filename.string, DefaultFileName, sizeof(DefaultFileName));
                object_prop_list.Properties[i].propval.str = (uint8_t *)&mtp_filename;

                element_len = sizeof(object_prop_list.Properties[i]) - 8U;
                rt_memcpy(usbd_mtp_dataset + dataLength, &object_prop_list.Properties[i],
                        element_len);
                dataLength += element_len;
                element_len = mtp_filename.len * 2U + 1U;
                rt_memcpy(usbd_mtp_dataset + dataLength, object_prop_list.Properties[i].propval.str,
                        element_len);
                break;

            case MTP_OB_PROP_PARENT_OBJECT:
                object_prop_list.Properties[i].PropertyCode = MTP_OB_PROP_PARENT_OBJECT;
                object_prop_list.Properties[i].Datatype = MTP_DATATYPE_UINT32;
                object_prop_list.Properties[i].propval.u32 = (uint32_t)parent_proval;

                element_len = sizeof(object_prop_list.Properties[i]) - 8U + sizeof(uint32_t);
                rt_memcpy(usbd_mtp_dataset + dataLength, &object_prop_list.Properties[i],
                        element_len);
                break;

            case MTP_OB_PROP_NAME:
                object_prop_list.Properties[i].PropertyCode = MTP_OB_PROP_NAME;
                object_prop_list.Properties[i].Datatype = MTP_DATATYPE_STR;
                mtp_filename.len = CONFIG_MTP_FILE_NAME_LEN;
                rt_memcpy((void *)(mtp_filename.string), (void *)DefaultFileName,
                        sizeof(DefaultFileName));
                object_prop_list.Properties[i].propval.str = (uint8_t *)&mtp_filename;

                element_len = sizeof(object_prop_list.Properties[i]) - 8U;
                rt_memcpy(usbd_mtp_dataset + dataLength, &object_prop_list.Properties[i],
                        element_len);
                dataLength += element_len;
                element_len = mtp_filename.len * 2U + 1U;
                rt_memcpy(usbd_mtp_dataset + dataLength, object_prop_list.Properties[i].propval.str,
                        element_len);
                break;

            case MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN:
                object_prop_list.Properties[i].PropertyCode = MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN;
                object_prop_list.Properties[i].Datatype = MTP_DATATYPE_UINT8;
                object_prop_list.Properties[i].propval.u8 = (uint8_t)g_usbd_mtp.con_command.param1;

                element_len = sizeof(object_prop_list.Properties[i]) - 8U + sizeof(uint8_t);
                rt_memcpy(usbd_mtp_dataset + dataLength, &object_prop_list.Properties[i],
                        element_len);
                break;

            case MTP_OB_PROP_PROTECTION_STATUS:
                object_prop_list.Properties[i].PropertyCode = MTP_OB_PROP_PROTECTION_STATUS;
                object_prop_list.Properties[i].Datatype = MTP_DATATYPE_UINT16;
                object_prop_list.Properties[i].propval.u16 = (uint16_t)default_val;

                element_len = sizeof(object_prop_list.Properties[i]) - 8U + sizeof(uint16_t);
                rt_memcpy(usbd_mtp_dataset + dataLength, &object_prop_list.Properties[i],
                        element_len);
                break;

            default:
                break;
        }
        dataLength += element_len;
    }

    usbd_mtp_send_info((uint8_t *)usbd_mtp_dataset, dataLength + sizeof(uint32_t));
}
# endif

void usbd_mtp_send_object_info(void)
{
    static uint32_t last_trans_id = 0x0;
    uint32_t index, parent_handle = g_usbd_mtp.con_command.param2;
    uint64_t MaxCapability, FreeSpaceInBytes;
    int fd = 0;
    char filename_tmp[255];
    struct mtp_object_info *object_info;
    struct mtp_object *object = NULL;

    if (g_usbd_mtp.con_command.trans_id != last_trans_id) {
        last_trans_id = g_usbd_mtp.con_command.trans_id;
        memset(g_usbd_mtp.path, 0, sizeof(g_usbd_mtp.path));
        if (parent_handle == 0xffffffff) {
            /* place object in root */
            strcat(g_usbd_mtp.path, ROOT_PATH);
        } else {
            usbd_mtp_get_object_by_handle(&object, parent_handle);
            strcpy(g_usbd_mtp.path, (char *)object->property.file_full_name);
        }

        /* No response */
        usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, (uint8_t *)&g_usbd_mtp.con_data,
                            CONFIG_USBDEV_MSC_MAX_BUFSIZE);
        return;
    }

    /* Make sure we can accommodate a new object here */
    if ((g_usbd_mtp.handle_counter + 1U) < MTP_OBJECT_HANDLES_MAX_NUM) {
        /* Get ObjecyInfo Dataset*/
        object_info = (struct mtp_object_info *)&g_usbd_mtp.con_data.data;

        _unicode_to_string(filename_tmp,
                            object_info->Filename,
                            object_info->Filename_len);
        strcat(g_usbd_mtp.path, "/");
        strcat(g_usbd_mtp.path, filename_tmp);
        /* Create file or directory */
        USB_LOG_DBG("%s\n", g_usbd_mtp.path);
        if (object_info->ObjectFormat != MTP_OBJ_FORMAT_ASSOCIATION)
            fd = usbd_mtp_creat_file(g_usbd_mtp.path);
        else
            usbd_mtp_make_dir(g_usbd_mtp.path);
        if (fd >= 0) {
            usbd_mtp_creat_object(&object, &index);
            object->handle = index;
            object->property.storage_id = MTP_STORAGE_ID;
            object->property.format = object_info->ObjectFormat;
            object->property.protection_status = object_info->ProtectionStatus;
            object->property.size = object_info->ObjectCompressedSize;
            if (usbd_mtp_get_cap(&MaxCapability, &FreeSpaceInBytes))
                USB_LOG_ERR("Failed to get storage info \r\n");
            if (object->property.size > FreeSpaceInBytes) {
                USB_LOG_ERR("Store full! object size:%ld freespace:%lld\r\n",
                            object->property.size, FreeSpaceInBytes);
                goto __store_full;
            }
            object->property.parent_object = parent_handle;
            object->property.file_name_length = object_info->Filename_len;
            _unicode_to_string(object->property.file_name,
                                object_info->Filename,
                                object->property.file_name_length);
            object->property.file_full_name_length = strlen(g_usbd_mtp.path);
            strcpy((char *)object->property.file_full_name, g_usbd_mtp.path);
            /* Close file */
            if (object_info->ObjectFormat != MTP_OBJ_FORMAT_ASSOCIATION)
                usbd_mtp_close_file(fd);
            g_usbd_mtp.mtp_file_out.handle = object->handle;
        } else {
            /* Identify the problem type of file creation */
            switch(fd) {
                // case :
                // break;
                default :
                    goto __store_full;
                break;
            }
        /* Failed to creat file */
        memset(object_info, 0, sizeof(struct mtp_object_info));
        return;
        }
    } else {
        USB_LOG_ERR("Store full! handle_counter%ld\r\n", g_usbd_mtp.handle_counter + 1U);
        goto __store_full;
    }

    usbd_mtp_send_response_param(MTP_RESPONSE_OK, object->property.storage_id,
                                    object->property.parent_object,
                                    object->handle, 0, 0);
    return;

__store_full:
    USB_LOG_ERR("Store full! Failed to create file!%d\r\n", fd);
    usbd_mtp_send_response(MTP_RESPONSE_STORE_FULL);
}

void usbd_mtp_data_out(void)
{
    int32_t ret;
    int32_t data_length = g_usbd_mtp.usbd_mtp_wcnt;
    if (data_length <= 0) {
        usbd_mtp_send_response(MTP_RESPONSE_OK);
        return;
    }

    ret = usbd_mtp_write_file(g_usbd_mtp.mtp_file_out.fd,
                                g_usbd_mtp.usbd_mtp_data_out,
                                g_usbd_mtp.mtp_file_out.data_length);
    if (ret <= 0) {
        usbd_mtp_send_response(MTP_RESPONSE_INCOMPLETE_TRANSFER);
        if (usbd_mtp_close_file(g_usbd_mtp.mtp_file_out.fd) < 0) {
            USB_LOG_ERR("Falie to close file\r\n");
        }
        return;
    }

    data_length = ((data_length - ret) < 0) ? data_length : (data_length - ret);

    g_usbd_mtp.mtp_file_out.data_length = MIN(data_length, CONFIG_USBDEV_MTP_MAX_BUFSIZE);

    g_usbd_mtp.usbd_mtp_wcnt = data_length;
    if (g_usbd_mtp.usbd_mtp_wcnt == 0) {
        if (usbd_mtp_close_file(g_usbd_mtp.mtp_file_out.fd) < 0) {
            USB_LOG_ERR("Falie to close file\r\n");
            return;
        }
        usbd_mtp_send_response(MTP_RESPONSE_OK);
        return;
    }
    usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr,
                        g_usbd_mtp.usbd_mtp_data_out, CONFIG_USBDEV_MTP_MAX_BUFSIZE);
}

void usbd_mtp_send_object(void)
{
    static uint32_t last_trans_id = 0x0;
    int fd;
    int32_t ret;
    struct mtp_object *object = NULL;
    if (g_usbd_mtp.con_command.trans_id != last_trans_id) {
        last_trans_id = g_usbd_mtp.con_command.trans_id;
        usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, (uint8_t *)&g_usbd_mtp.con_data, CONFIG_USBDEV_MTP_MAX_BUFSIZE);
        return;
    }

    ret = usbd_mtp_get_object_by_handle(&object, g_usbd_mtp.mtp_file_out.handle);

    if (ret < 0) {
        USB_LOG_ERR("Faile to get flie(%s)\r\n", (char *)object->property.file_full_name);
        usbd_mtp_send_response(MTP_RESPONSE_INVALID_OBJECT_HANDLE);
        return;
    }

    /* Write */
    fd = usbd_mtp_open_file_wronly((char *)object->property.file_full_name);
    if (fd < 0) {
        USB_LOG_ERR("Faile to open file(%s)\r\n", (char *)object->property.file_full_name);
        usbd_mtp_send_response(MTP_RESPONSE_INCOMPLETE_TRANSFER);
        return;
    }

    uint32_t data_length = g_usbd_mtp.con_data.conlen - CONFIG_MTP_COMMAND_LEN;
    if (data_length > CONFIG_USBDEV_MTP_MAX_BUFSIZE - CONFIG_MTP_COMMAND_LEN)
        g_usbd_mtp.stage = MTP_DATA_OUT;

    g_usbd_mtp.usbd_mtp_wcnt = data_length;
    g_usbd_mtp.mtp_file_out.fd = fd;
    g_usbd_mtp.mtp_file_out.data_length = MIN(data_length,
                                                CONFIG_USBDEV_MTP_MAX_BUFSIZE - CONFIG_MTP_COMMAND_LEN);
    memcpy(g_usbd_mtp.usbd_mtp_data_out, g_usbd_mtp.con_data.data,
            MIN(data_length, CONFIG_USBDEV_MTP_MAX_BUFSIZE - CONFIG_MTP_COMMAND_LEN));

    usbd_mtp_data_out();

    return;
}

void usbd_mtp_delete_object(void)
{
    int ret = 0;
    struct mtp_object *object = NULL;
    /* Delete all objects */
    if (g_usbd_mtp.con_command.param1 == 0xffffffff) {
        USB_LOG_ERR("Delete all objects!\r\n");
    }
    usbd_mtp_get_object_by_handle(&object, g_usbd_mtp.con_command.param1);

    usbd_mtp_unlink_object(object);

    if (ret < 0) {
        USB_LOG_ERR("Failed to detele flle(%s)\r\n", (char *)object->property.file_name);
        usbd_mtp_send_response(MTP_RESPONSE_INVALID_OBJECT_HANDLE);
        return;
    }

    usbd_mtp_send_response(MTP_RESPONSE_OK);
}

static void usbd_mtp_get_device_prop_desc(void)
{
    struct mtp_device_prop_desc device_prop_desc;
    uint32_t i;

    device_prop_desc.DevicePropertyCode = MTP_DEV_PROP_DEVICE_FRIENDLY_NAME;
    device_prop_desc.DataType = MTP_DATATYPE_STR;
    device_prop_desc.GetSet = MTP_PROP_GET_SET;
    device_prop_desc.DefaultValue_len = CONFIG_MTP_DEVICE_PROP_DESC_DEF_LEN;

    for (i = 0U; i < (sizeof(DevicePropDefVal) / 2U); i++) {
        device_prop_desc.DefaultValue[i] = DevicePropDefVal[i];
    }

    device_prop_desc.CurrentValue_len = CONFIG_MTP_DEVICE_PROP_DESC_CUR_LEN;

    for (i = 0U; i < (sizeof(DevicePropCurDefVal) / 2U); i++) {
        device_prop_desc.CurrentValue[i] = DevicePropCurDefVal[i];
    }

    device_prop_desc.FormFlag = 0U;

    usbd_mtp_send_info((uint8_t *)&device_prop_desc, sizeof(struct mtp_device_prop_desc));
}

static int usbd_mtp_decode(struct mtp_container *container)
{
    switch (container->code) {
        case MTP_OP_GET_DEVICE_INFO:
            usbd_mtp_get_device_info();
            break;
        case MTP_OP_OPEN_SESSION:
            usbd_mtp_open_session();
            break;
        case MTP_OP_CLOSE_SESSION:
            break;
        case MTP_OP_GET_STORAGE_IDS:
            usbd_mtp_get_storage_ids();
            break;
        case MTP_OP_GET_STORAGE_INFO:
            usbd_mtp_get_storage_info();
            break;
        case MTP_OP_GET_OBJECT_HANDLES:
            usbd_mtp_get_object_handles();
            break;
        case MTP_OP_GET_OBJECT_INFO:
            usbd_mtp_get_object_info();
            break;
        case MTP_OP_GET_OBJECT_PROP_REFERENCES:
            break;
        case MTP_OP_GET_OBJECT_PROPS_SUPPORTED:
            usbd_mtp_get_object_props_supported();
            break;
        case MTP_OP_GET_OBJECT_PROP_DESC:
            usbd_mtp_get_object_prop_desc();
            break;
        case MTP_OP_GET_OBJECT_PROPLIST:
            break;
        case MTP_OP_SET_OBJECT_PROP_VALUE:
            // usbd_mtp_set_object_prop_value();
            break;
        case MTP_OP_GET_OBJECT_PROP_VALUE:
            // usbd_mtp_get_object_prop_value();
            break;
        case MTP_OP_GET_DEVICE_PROP_DESC:
            usbd_mtp_get_device_prop_desc();
            break;
        case MTP_OP_GET_OBJECT:
            usbd_mtp_get_object();
            break;
        case MTP_OP_SEND_OBJECT_INFO:
            usbd_mtp_send_object_info();
            break;
        case MTP_OP_SEND_OBJECT:
            usbd_mtp_send_object();
            break;
        case MTP_OP_DELETE_OBJECT:
            usbd_mtp_delete_object();
            break;

        default:
            USB_LOG_WRN("code:%04x\r\n", container->code);
            break;
    }
    return 0;
}

static void usbd_mtp_bulk_out(uint8_t ep, uint32_t nbytes)
{
    switch (g_usbd_mtp.stage) {
        case MTP_READ_COMMAND:
            #ifdef CONFIG_USBDEV_MTP_THREAD
            usb_osal_mq_send(g_usbd_mtp.usbd_mtp_mq, MTP_READ_COMMAND);
            #else
            usbd_mtp_decode((struct mtp_container *)&g_usbd_mtp.con_command);
            #endif
            break;
        case MTP_DATA_OUT:
            #ifdef CONFIG_USBDEV_MTP_THREAD
            usb_osal_mq_send(g_usbd_mtp.usbd_mtp_mq, MTP_DATA_OUT);
            #else
            usbd_mtp_data_out();
            #endif
            break;
        default:
            break;
    }
}

static void usbd_mtp_bulk_in(uint8_t ep, uint32_t nbytes)
{
    switch (g_usbd_mtp.stage) {
        case MTP_DATA_IN:
            #ifdef CONFIG_USBDEV_MTP_THREAD
            usb_osal_mq_send(g_usbd_mtp.usbd_mtp_mq, MTP_DATA_IN);
            #else
            usbd_mtp_data_in();
            #endif
            break;
        case MTP_SEND_RESPONSE:
            usbd_mtp_send_response(MTP_RESPONSE_OK);
            break;
        case MTP_WAIT_RESPONSE:
            g_usbd_mtp.stage = MTP_READ_COMMAND;
            usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, (uint8_t *)&g_usbd_mtp.con_command, CONFIG_USBDEV_MTP_MAX_BUFSIZE);
            break;
        default:
            break;
    }
}

static void mtp_notify_handler(uint8_t event, void *arg)
{
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONFIGURED:
            USB_LOG_DBG("Start reading command\r\n");
            g_usbd_mtp.stage = MTP_READ_COMMAND;
            usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, (uint8_t *)&g_usbd_mtp.con_command, CONFIG_USBDEV_MTP_MAX_BUFSIZE);
            break;

        default:
            break;
    }
}

#ifdef CONFIG_USBDEV_MTP_THREAD
static void usbdev_mtp_thread(void *argument)
{
    uintptr_t event;
    int ret;

    while (1) {
        ret = usb_osal_mq_recv(g_usbd_mtp.usbd_mtp_mq, (uintptr_t *)&event, USB_OSAL_WAITING_FOREVER);
        if (ret < 0) {
            continue;
        }
        USB_LOG_DBG("%d\r\n", event);
        if (event == MTP_DATA_OUT) {
            usbd_mtp_data_out();
        } else if (event == MTP_DATA_IN) {
            usbd_mtp_data_in();
        } else if (event == MTP_READ_COMMAND) {
            usbd_mtp_decode((struct mtp_container *)&g_usbd_mtp.con_command);
        } else {
        }
    }
}
#endif

struct usbd_interface *usbd_mtp_init_intf(struct usbd_interface *intf,
                                          const uint8_t out_ep,
                                          const uint8_t in_ep,
                                          const uint8_t int_ep)
{
    g_usbd_mtp.handle_counter = 0U;
    g_usbd_mtp.mtp_objects = mtp_objects;

    intf->class_interface_handler = mtp_class_interface_request_handler;
    intf->class_endpoint_handler = NULL;
    intf->vendor_handler = NULL;
    intf->notify_handler = mtp_notify_handler;

    mtp_ep_data[MTP_OUT_EP_IDX].ep_addr = out_ep;
    mtp_ep_data[MTP_OUT_EP_IDX].ep_cb = usbd_mtp_bulk_out;
    mtp_ep_data[MTP_IN_EP_IDX].ep_addr = in_ep;
    mtp_ep_data[MTP_IN_EP_IDX].ep_cb = usbd_mtp_bulk_in;
    mtp_ep_data[MTP_INT_EP_IDX].ep_addr = int_ep;
    mtp_ep_data[MTP_INT_EP_IDX].ep_cb = NULL;

    usbd_add_endpoint(&mtp_ep_data[MTP_OUT_EP_IDX]);
    usbd_add_endpoint(&mtp_ep_data[MTP_IN_EP_IDX]);
    usbd_add_endpoint(&mtp_ep_data[MTP_INT_EP_IDX]); //event

#ifdef CONFIG_USBDEV_MTP_THREAD
    g_usbd_mtp.usbd_mtp_mq = usb_osal_mq_create(DATA_BUFFER_SIZE / MAX_WITTE_FILE_SIZE);
    if (g_usbd_mtp.usbd_mtp_mq == NULL) {
        return NULL;
    }
    g_usbd_mtp.usbd_mtp_thread = usb_osal_thread_create("usbd_mtp", 1024 * 6, CONFIG_USBDEV_MSC_PRIO, usbdev_mtp_thread, NULL);
    if (g_usbd_mtp.usbd_mtp_thread == NULL) {
        return NULL;
    }
#endif

    return intf;
}
