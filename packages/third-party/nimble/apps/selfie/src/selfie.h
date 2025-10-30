/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef __SELFIE_H__
#define __SELFIE_H__

#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "nimble/ble.h"
#include "modlog/modlog.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SELFIE_DEVICE_NAME        "Selfie Device"
#define SELFIE_SERVICE_UUID       0x1812
#define SELFIE_APPEARANCE_GENERIC 0x03C1

#define HID_SERVICE_UUID           0x1812
#define HID_PROTOCOL_MODE_UUID     0x2A4E
#define HID_INFO_UUID              0x2A4A
#define HID_CONTROL_POINT_UUID     0x2A4C
#define HID_REPORT_MAP_UUID        0x2A4B
#define HID_REPORT_UUID            0x2A4D
#define HID_REPORT_DESCRIPTOR_UUID 0x2908

static uint16_t conn_handle;
static struct ble_npl_event ble_hs_ev_shutter;

/* GATT server callback */
void selfie_gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);

int selfie_gatt_svr_init(void);
int selfie_gatt_send_report(uint16_t conn_handle, uint8_t *data, int32_t len);

#ifdef __cplusplus
}
#endif

#endif
