/*
 * SPDX-FileCopyrightText: 2015-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "selfie.h"

enum {
    HID_REMOTE_WAKE = 1,
    HID_NORMALLY_CONNECTABLE = 2,
};

struct hid_info {
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t flags;
};

struct hid_report {
    uint8_t id;
    uint8_t type;
};

struct hid_info info = {
    .bcdHID = 0x0111,
    .bCountryCode = 0,
    .flags = HID_NORMALLY_CONNECTABLE,
};

struct hid_report report = {
    .id = 0x01,
    .type = 0x01,
};

uint8_t report_map[] = {
    0x05, 0x0C,        // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,        // USAGE (Consumer Control)
    0xA1, 0x01,        // COLLECTION (Application)
    0x85, 0x01,        //   REPORT_ID (1)
    0x75, 0x10,        //   REPORT_SIZE (16)
    0x95, 0x01,        //   REPORT_COUNT (1)
    0x15, 0x01,        //   LOGICAL_MINIMUM (1)
    0x26, 0xFF, 0x07,  //   LOGICAL_MAXIMUM (2047)
    0x19, 0x01,        //   USAGE_MINIMUM (1)
    0x2A, 0xFF, 0x07,  //   USAGE_MAXIMUM (2047)
    0x81, 0x00,        //   INPUT (Data, Ary, Abs)
    0xC0,              // END_COLLECTION
};

uint8_t protocol_mode = 0;
uint8_t control_point = 0;
uint16_t selfie_report_handle;

static int selfie_gatt_dsc_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt, void *arg);
static int selfie_gatt_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /*** Service: HID */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(HID_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) { {
            /* Characteristic: HID Protocol Mode */
            .uuid = BLE_UUID16_DECLARE(HID_PROTOCOL_MODE_UUID),
            .access_cb = selfie_gatt_chr_access_cb,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE_NO_RSP,
        }, {
            /* Characteristic: HID Info */
            .uuid = BLE_UUID16_DECLARE(HID_INFO_UUID),
            .access_cb = selfie_gatt_chr_access_cb,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: HID Control Point */
            .uuid = BLE_UUID16_DECLARE(HID_CONTROL_POINT_UUID),
            .access_cb = selfie_gatt_chr_access_cb,
            .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
        }, {
            /* Characteristic: HID Report Map */
            .uuid = BLE_UUID16_DECLARE(HID_REPORT_MAP_UUID),
            .access_cb = selfie_gatt_chr_access_cb,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            /* Characteristic: HID Report1 */
            .uuid = BLE_UUID16_DECLARE(HID_REPORT_UUID),
            .access_cb = selfie_gatt_chr_access_cb,
            .val_handle = &selfie_report_handle,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            .descriptors = (struct ble_gatt_dsc_def[]) { {
                .uuid = BLE_UUID16_DECLARE(HID_REPORT_DESCRIPTOR_UUID),
                .access_cb = selfie_gatt_dsc_access_cb,
                .att_flags = BLE_ATT_F_READ,
            }, {
                0
            }, }
        }, {
            0, /* No more characteristics in this service */
        }, }
    },

    {
        0, /* No more services */
    },
};

static int selfie_gatt_dsc_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
    char buf[BLE_UUID_STR_LEN];

    MODLOG_DFLT(INFO, "dsc uuid:%s\n", ble_uuid_to_str(ctxt->dsc->uuid, buf));
    switch (ble_uuid_u16(ctxt->dsc->uuid)) {
    case HID_REPORT_DESCRIPTOR_UUID:
        return os_mbuf_append(ctxt->om, &report, sizeof(struct hid_report));

    default:
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;
    }

    /*
     * Unknown characteristic; the nimble stack should not have called this
     * function.
     */
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

static int selfie_gatt_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
    char buf[BLE_UUID_STR_LEN];

    MODLOG_DFLT(INFO, "uuid:%s, val_handle:%d\n", ble_uuid_to_str(ctxt->chr->uuid, buf), ctxt->chr->val_handle);
    switch (ble_uuid_u16(ctxt->chr->uuid)) {
    case HID_PROTOCOL_MODE_UUID:
        return os_mbuf_append(ctxt->om, &protocol_mode, sizeof(protocol_mode));
    case HID_CONTROL_POINT_UUID:
        if (om_len != sizeof(control_point)) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        return ble_hs_mbuf_to_flat(ctxt->om, &control_point, om_len, NULL);

    case HID_INFO_UUID:
        return os_mbuf_append(ctxt->om, &info, sizeof(struct hid_info));
        
    case HID_REPORT_MAP_UUID:
        return os_mbuf_append(ctxt->om, report_map, sizeof(report_map));

    case HID_REPORT_UUID:
        return os_mbuf_append(ctxt->om, NULL, 0);

    default:
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;
    }

    /*
     * Unknown characteristic; the nimble stack should not have called this
     * function.
     */
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void selfie_gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(INFO, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf), ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(INFO, "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(INFO, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf), ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int selfie_gatt_send_report(uint16_t conn_handle, uint8_t *data, int32_t len)
{
    struct os_mbuf *om;
    int rc = 0;

    om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL) {
        MODLOG_DFLT(ERROR, "Error in allocating memory\n");
        return -1;
    }

    rc = ble_gatts_notify_custom(conn_handle, selfie_report_handle, om);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error in sending notification\n");
        return -1;
    }

    return 0;
}

int selfie_gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
