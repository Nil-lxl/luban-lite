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

#ifndef __BLUFI_H__
#define __BLUFI_H__

#include "host/ble_gatt.h"
#include "nimble/ble.h"
#include "modlog/modlog.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLUFI_APP_UUID    0xFFFF
#define BLUFI_DEVICE_NAME "BLUFI_DEVICE"

struct gatt_value {
    struct os_mbuf *buf;
    uint16_t val_handle;
    uint8_t type;
    void *ptr;
};

#define SERVER_MAX_VALUES       3
#define MAX_VAL_SIZE            512
extern struct gatt_value gatt_values[SERVER_MAX_VALUES];

/* GATT server callback */
void blufi_gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);

int blufi_gatt_svr_init(void);
void blufi_send_notify(void *arg);
void blufi_send_encap(void *arg);

#ifdef __cplusplus
}
#endif

#endif
