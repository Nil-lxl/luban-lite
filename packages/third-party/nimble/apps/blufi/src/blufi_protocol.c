/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include <syscfg/syscfg.h>
#define BLE_NPL_LOG_MODULE BLE_BLUFI_LOG
#include <nimble/nimble_npl_log.h>
#include "host/ble_hs_mbuf.h"
#include "blufi_protocol.h"
#include "blufi_api.h"
#include "blufi_prf.h"
#include "blufi.h"

void blufi_protocol_handler(uint8_t type, uint8_t *data, int len)
{
    blufi_cb_param_t param;
    uint8_t *output_data = NULL;
    int output_len = 0;
    bool need_free = false;

    switch (BLUFI_GET_TYPE(type)) {
    case BLUFI_TYPE_CTRL:
        switch (BLUFI_GET_SUBTYPE(type)) {
        case BLUFI_TYPE_CTRL_SUBTYPE_ACK:
            /* TODO: check sequence */
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_SET_SEC_MODE:
            blufi_env.sec_mode = data[0];
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_SET_WIFI_OPMODE:
            param.wifi_mode.op_mode = data[0];
            blufi_env.cbs->event_cb(BLUFI_EVENT_SET_WIFI_OPMODE, &param);
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_CONN_TO_AP:
            blufi_env.cbs->event_cb(BLUFI_EVENT_REQ_CONNECT_TO_AP, NULL);
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_DISCONN_FROM_AP:
            blufi_env.cbs->event_cb(BLUFI_EVENT_REQ_DISCONNECT_FROM_AP, NULL);
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_GET_WIFI_STATUS:
            blufi_env.cbs->event_cb(BLUFI_EVENT_GET_WIFI_STATUS, NULL);
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_DEAUTHENTICATE_STA:
            blufi_env.cbs->event_cb(BLUFI_EVENT_DEAUTHENTICATE_STA, NULL);
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_GET_VERSION: {
            uint8_t type = BLUFI_BUILD_TYPE(BLUFI_TYPE_DATA, BLUFI_TYPE_DATA_SUBTYPE_REPLY_VERSION);
            uint8_t data[2];

            data[0] = BLUFI_GREAT_VER;
            data[1] = BLUFI_SUB_VER;
            ble_blufi_send_encap(type, &data[0], sizeof(data));
            break;
        }
        case BLUFI_TYPE_CTRL_SUBTYPE_DISCONNECT_BLE:
            blufi_env.cbs->event_cb(BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE, NULL);
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_GET_WIFI_LIST:
            blufi_env.cbs->event_cb(BLUFI_EVENT_GET_WIFI_LIST, NULL);
            break;
        default:
            MODLOG_DFLT_ERROR("%s Unkown Ctrl pkt %02x\n", __func__, type);
            break;
        }
        break;
    case BLUFI_TYPE_DATA:
        switch (BLUFI_GET_SUBTYPE(type)) {
        case BLUFI_TYPE_DATA_SUBTYPE_NEG:
            if (blufi_env.cbs && blufi_env.cbs->negotiate_data_handler) {
                blufi_env.cbs->negotiate_data_handler(data, len, &output_data, &output_len, &need_free);
            }

            if (output_data && output_len > 0) {
                ble_blufi_send_encap(BLUFI_BUILD_TYPE(BLUFI_TYPE_DATA, BLUFI_TYPE_DATA_SUBTYPE_NEG),
                             output_data, output_len);
            }
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_STA_BSSID:
            memcpy(param.sta_bssid.bssid, &data[0], 6);
            blufi_env.cbs->event_cb(BLUFI_EVENT_RECV_STA_BSSID, &param);
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_STA_SSID:
            param.sta_ssid.ssid = &data[0];
            param.sta_ssid.ssid_len = len;
            blufi_env.cbs->event_cb(BLUFI_EVENT_RECV_STA_SSID, &param);
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_STA_PASSWD:
            param.sta_passwd.passwd = &data[0];
            param.sta_passwd.passwd_len = len;
            blufi_env.cbs->event_cb(BLUFI_EVENT_RECV_STA_PASSWD, &param);
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_SSID:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_PASSWD:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_MAX_CONN_NUM:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_AUTH_MODE:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_CHANNEL:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_USERNAME:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_CA:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_CLIENT_CERT:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SERVER_CERT:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_CLIENT_PRIV_KEY:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SERVER_PRIV_KEY:
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_CUSTOM_DATA:
            param.custom_data.data = &data[0];
            param.custom_data.data_len = len;
            blufi_env.cbs->event_cb(BLUFI_EVENT_RECV_CUSTOM_DATA, &param);
            break;
        default:
            MODLOG_DFLT_ERROR("%s Unkown Ctrl pkt %02x\n", __func__, type);
            break;
        }
        break;
    default:
        break;
    }
}
