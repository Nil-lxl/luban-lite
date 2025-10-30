/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscfg/syscfg.h>
#define BLE_NPL_LOG_MODULE BLE_BLUFI_LOG
#include <nimble/nimble_npl_log.h>
#include "blufi_protocol.h"
#include "blufi_api.h"
#include "blufi_prf.h"

int blufi_register_callbacks(blufi_callbacks_t *callbacks)
{
    if (callbacks == NULL) {
        return -EINVAL;
    }

    ble_blufi_set_callbacks(callbacks);

    return 0;
}

int blufi_profile_init(void)
{
    ble_blufi_call_handler(BLUFI_ACT_INIT, NULL);

    return 0;
}

int blufi_profie_deinit(void)
{
    ble_blufi_call_handler(BLUFI_ACT_DEINIT, NULL);

    return 0;
}

uint16_t blufi_get_version(void)
{
    return BLUFI_VERSION;
}

int blufi_send_error_info(blufi_error_state_t state)
{
    blufi_args_t arg;

    arg.blufi_err_infor.state = state;

    ble_blufi_call_handler(BLUFI_ACT_SEND_ERR_INFO, &arg);

    return 0;
}

int blufi_send_custom_data(uint8_t *data, uint32_t data_len)
{
    blufi_args_t arg;

    if(data == NULL || data_len == 0) {
        return -EINVAL;
    }

    arg.custom_data.data = data;
    arg.custom_data.data_len = data_len;

    ble_blufi_call_handler(BLUFI_ACT_SEND_CUSTOM_DATA, &arg);

    return 0;
}
