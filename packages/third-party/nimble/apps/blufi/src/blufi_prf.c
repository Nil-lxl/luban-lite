/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
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
#include "blufi_protocol.h"
#include "blufi_prf.h"
#include "blufi_api.h"
#include "blufi.h"

#if GATT_DYNAMIC_MEMORY == FALSE
blufi_env_t blufi_env;
#else
blufi_env_t *blufi_env_ptr;
#endif

// static functions declare
static void ble_blufi_send_ack(uint8_t seq);

static int ble_blufi_profile_init(void)
{
    blufi_cb_param_t param;
    blufi_callbacks_t *store_p = blufi_env.cbs;

    if (blufi_env.enabled) {
        MODLOG_DFLT_ERROR("BLUFI already initialized");
        return -1;
    }

    memset(&blufi_env, 0x0, sizeof(blufi_env));
    blufi_env.enabled = true;
    blufi_env.cbs = store_p; /* if set callback prior, restore the point */
    blufi_env.frag_size = BLUFI_FRAG_DATA_DEFAULT_LEN;

    param.init_finish.state = BLUFI_INIT_OK;
    blufi_env.cbs->event_cb(BLUFI_EVENT_INIT_FINISH, &param);

    return 0;
}

static uint8_t ble_blufi_profile_deinit(void)
{
    blufi_cb_param_t param;

    if (!blufi_env.enabled) {
        MODLOG_DFLT_ERROR("BLUFI already de-initialized");
        return -1;
    }

    blufi_env.enabled = false;

    param.deinit_finish.state = BLUFI_DEINIT_OK;
    blufi_env.cbs->event_cb(BLUFI_EVENT_DEINIT_FINISH, &param);

    return 0;
}

void ble_blufi_send_notify(uint8_t *pkt, int pkt_len)
{
   struct pkt_info pkts;
   pkts.pkt = pkt;
   pkts.pkt_len = pkt_len;
   blufi_send_notify(&pkts);
}

void ble_blufi_report_error(blufi_error_state_t state)
{
    blufi_cb_param_t param;

    param.report_error.state = state;
    blufi_env.cbs->event_cb(BLUFI_EVENT_REPORT_ERROR, &param);
}

void ble_blufi_recv_handler(uint8_t *data, int len)
{
    struct blufi_hdr *hdr = (struct blufi_hdr *)data;
    int target_data_len;
    uint16_t checksum, checksum_pkt;
    int ret;

    MODLOG_DFLT(ERROR, "%s, %d\n", __func__, __LINE__);
    if (len < sizeof(struct blufi_hdr)) {
        MODLOG_DFLT_ERROR("%s invalid data length: %d", __func__, len);
        ble_blufi_report_error(BLUFI_DATA_FORMAT_ERROR);
        return;
    }

    // Verify if the received data length matches the expected length based on the BLUFI protocol
    if (BLUFI_FC_IS_CHECK(hdr->fc)) {
        target_data_len = hdr->data_len + 4 + 2; // Data + (Type + Frame Control + Sequence Number + Data Length) + Checksum
    } else {
        target_data_len = hdr->data_len + 4; // Data + (Type + Frame Control + Sequence Number + Data Length)
    }

    if (len != target_data_len) {
        MODLOG_DFLT_ERROR("%s: Invalid data length: %d, expected: %d", __func__, len, target_data_len);
        ble_blufi_report_error(BLUFI_DATA_FORMAT_ERROR);
        return;
    }

    if (hdr->seq != blufi_env.recv_seq) {
        MODLOG_DFLT_ERROR("%s seq %d is not expect %d\n", __func__, hdr->seq, blufi_env.recv_seq + 1);
        ble_blufi_report_error(BLUFI_SEQUENCE_ERROR);
        return;
    }

    blufi_env.recv_seq++;

    // first step, decrypt
    if (BLUFI_FC_IS_ENC(hdr->fc) && (blufi_env.cbs && blufi_env.cbs->decrypt_func)) {
        ret = blufi_env.cbs->decrypt_func(hdr->seq, hdr->data, hdr->data_len);
        if (ret != hdr->data_len) { /* enc must be success and enc len must equal to plain len */
            MODLOG_DFLT_ERROR("%s decrypt error %d\n", __func__, ret);
            ble_blufi_report_error(BLUFI_DECRYPT_ERROR);
            return;
        }
    }

    // second step, check sum
    if (BLUFI_FC_IS_CHECK(hdr->fc) && (blufi_env.cbs && blufi_env.cbs->checksum_func)) {
        checksum = blufi_env.cbs->checksum_func(hdr->seq, &hdr->seq, hdr->data_len + 2);
        checksum_pkt = hdr->data[hdr->data_len] | (((uint16_t) hdr->data[hdr->data_len + 1]) << 8);
        if (checksum != checksum_pkt) {
            MODLOG_DFLT_ERROR("%s checksum error %04x, pkt %04x\n", __func__, checksum, checksum_pkt);
            ble_blufi_report_error(BLUFI_CHECKSUM_ERROR);
            return;
        }
    }

    if (BLUFI_FC_IS_REQ_ACK(hdr->fc)) {
        ble_blufi_send_ack(hdr->seq);
    }

    if (BLUFI_FC_IS_FRAG(hdr->fc)) {
        if (blufi_env.offset == 0) {
            /*
             * blufi_env.aggr_buf should be NULL if blufi_env.offset is 0.
             * It is possible that the process of sending fragment packet
             * has not been completed
             */
            if (blufi_env.aggr_buf) {
                MODLOG_DFLT_ERROR("%s msg error, blufi_env.aggr_buf is not freed\n", __func__);
                ble_blufi_report_error(BLUFI_MSG_STATE_ERROR);
                return;
            }
            blufi_env.total_len = hdr->data[0] | (((uint16_t) hdr->data[1]) << 8);
            blufi_env.aggr_buf = malloc(blufi_env.total_len);
            if (blufi_env.aggr_buf == NULL) {
                MODLOG_DFLT_ERROR("%s no mem, len %d\n", __func__, blufi_env.total_len);
                ble_blufi_report_error(BLUFI_DH_MALLOC_ERROR);
                return;
            }
        }
        if (blufi_env.offset + hdr->data_len  - 2 <= blufi_env.total_len){
            memcpy(blufi_env.aggr_buf + blufi_env.offset, hdr->data + 2, hdr->data_len  - 2);
            blufi_env.offset += (hdr->data_len - 2);
        } else {
            MODLOG_DFLT_ERROR("%s payload is longer than packet length, len %d \n", __func__, blufi_env.total_len);
            ble_blufi_report_error(BLUFI_DATA_FORMAT_ERROR);
            return;
        }

    } else {
        if (blufi_env.offset > 0) {   /* if previous pkt is frag */
            /* blufi_env.aggr_buf should not be NULL */
            if (blufi_env.aggr_buf == NULL) {
                MODLOG_DFLT_ERROR("%s buffer is NULL\n", __func__);
                ble_blufi_report_error(BLUFI_DH_MALLOC_ERROR);
                return;
            }
            /* payload length should be equal to total_len */
            if ((blufi_env.offset + hdr->data_len) != blufi_env.total_len) {
                MODLOG_DFLT_ERROR("%s payload is longer than packet length, len %d \n", __func__, blufi_env.total_len);
                ble_blufi_report_error(BLUFI_DATA_FORMAT_ERROR);
                return;
            }
            memcpy(blufi_env.aggr_buf + blufi_env.offset, hdr->data, hdr->data_len);

            blufi_protocol_handler(hdr->type, blufi_env.aggr_buf, blufi_env.total_len);
            blufi_env.offset = 0;
            free(blufi_env.aggr_buf);
            blufi_env.aggr_buf = NULL;
        } else {
            blufi_protocol_handler(hdr->type, hdr->data, hdr->data_len);
            blufi_env.offset = 0;
        }
    }
}

void ble_blufi_send_encap(uint8_t type, uint8_t *data, int total_data_len)
{
    struct blufi_hdr *hdr = NULL;
    int remain_len = total_data_len;
    uint16_t checksum;
    int ret;

    if (blufi_env.is_connected == false) {
        MODLOG_DFLT_ERROR("blufi connection has been disconnected \n");
        return;
    }

    while (remain_len > 0) {
        if (remain_len > blufi_env.frag_size) {
            hdr = malloc(sizeof(struct blufi_hdr) + 2 + blufi_env.frag_size + 2);
            if (hdr == NULL) {
                MODLOG_DFLT_ERROR("%s no mem\n", __func__);
                return;
            }
            hdr->fc = 0x0;
            hdr->data_len = blufi_env.frag_size + 2;
            hdr->data[0] = remain_len & 0xff;
            hdr->data[1] = (remain_len >> 8) & 0xff;
            memcpy(hdr->data + 2, &data[total_data_len - remain_len], blufi_env.frag_size); //copy first, easy for check sum
            hdr->fc |= BLUFI_FC_FRAG;
        } else {
            hdr = malloc(sizeof(struct blufi_hdr) + remain_len + 2);
            if (hdr == NULL) {
                MODLOG_DFLT_ERROR("%s no mem\n", __func__);
                return;
            }
            hdr->fc = 0x0;
            hdr->data_len = remain_len;
            memcpy(hdr->data, &data[total_data_len - remain_len], hdr->data_len); //copy first, easy for check sum
        }

        hdr->type = type;
        hdr->fc |= BLUFI_FC_DIR_E2P;
        hdr->seq = blufi_env.send_seq++;

        if (BLUFI_TYPE_IS_CTRL(hdr->type)) {
            if ((blufi_env.sec_mode & BLUFI_CTRL_SEC_MODE_CHECK_MASK)
                    && (blufi_env.cbs && blufi_env.cbs->checksum_func)) {
                hdr->fc |= BLUFI_FC_CHECK;
                checksum = blufi_env.cbs->checksum_func(hdr->seq, &hdr->seq, hdr->data_len + 2);
                memcpy(&hdr->data[hdr->data_len], &checksum, 2);
            }
        } else if (!BLUFI_TYPE_IS_DATA_NEG(hdr->type) && !BLUFI_TYPE_IS_DATA_ERROR_INFO(hdr->type)) {
            if ((blufi_env.sec_mode & BLUFI_DATA_SEC_MODE_CHECK_MASK)
                    && (blufi_env.cbs && blufi_env.cbs->checksum_func)) {
                hdr->fc |= BLUFI_FC_CHECK;
                checksum = blufi_env.cbs->checksum_func(hdr->seq, &hdr->seq, hdr->data_len + 2);
                memcpy(&hdr->data[hdr->data_len], &checksum, 2);
            }

            if ((blufi_env.sec_mode & BLUFI_DATA_SEC_MODE_ENC_MASK)
                    && (blufi_env.cbs && blufi_env.cbs->encrypt_func)) {
                ret = blufi_env.cbs->encrypt_func(hdr->seq, hdr->data, hdr->data_len);
                if (ret == hdr->data_len) { /* enc must be success and enc len must equal to plain len */
                    hdr->fc |= BLUFI_FC_ENC;
                } else {
                    MODLOG_DFLT_ERROR("%s encrypt error %d\n", __func__, ret);
                    ble_blufi_report_error(BLUFI_ENCRYPT_ERROR);
                    free(hdr);
                    return;
                }
            }
        }

        if (hdr->fc & BLUFI_FC_FRAG) {
            remain_len -= (hdr->data_len - 2);
        } else {
            remain_len -= hdr->data_len;
        }

        blufi_send_encap(hdr);

        free(hdr);
        hdr =  NULL;
    }
}

static void ble_blufi_send_ack(uint8_t seq)
{
    uint8_t type;
    uint8_t data;

    type = BLUFI_BUILD_TYPE(BLUFI_TYPE_CTRL, BLUFI_TYPE_CTRL_SUBTYPE_ACK);
    data = seq;

    ble_blufi_send_encap(type, &data, 1);
}

static void ble_blufi_send_error_info(uint8_t state)
{
    uint8_t type;
    uint8_t *data;
    int data_len;
    uint8_t *p;

    data_len = 1;
    p = data = malloc(data_len);
    if (data == NULL) {
        MODLOG_DFLT_ERROR("%s no mem\n", __func__);
        return;
    }

    type = BLUFI_BUILD_TYPE(BLUFI_TYPE_DATA, BLUFI_TYPE_DATA_SUBTYPE_ERROR_INFO);
    *p++ = state;
    if (p - data > data_len) {
        MODLOG_DFLT_ERROR("%s len error %d %d\n", __func__, (int)(p - data), data_len);
    }

    ble_blufi_send_encap(type, data, data_len);
    free(data);
}

static void ble_blufi_send_custom_data(uint8_t *value, uint32_t value_len)
{
    uint8_t type;

    if(value == NULL || value_len == 0) {
        MODLOG_DFLT_ERROR("%s value or value len error", __func__);
        return;
    }
    uint8_t *data = malloc(value_len);
    if (data == NULL) {
        MODLOG_DFLT_ERROR("%s mem malloc error", __func__);
        return;
    }

    type = BLUFI_BUILD_TYPE(BLUFI_TYPE_DATA, BLUFI_TYPE_DATA_SUBTYPE_CUSTOM_DATA);
    memcpy(data, value, value_len);
    ble_blufi_send_encap(type, data, value_len);
    free(data);
}

void ble_blufi_call_handler(uint8_t act, blufi_args_t *arg)
{
    uint8_t *data;

    switch (act) {
    case BLUFI_ACT_INIT:
        ble_blufi_profile_init();
        break;
    case BLUFI_ACT_DEINIT:
        ble_blufi_profile_deinit();
        break;
    case BLUFI_ACT_SEND_CFG_REPORT:
        break;
    case BLUFI_ACT_SEND_WIFI_LIST:
        break;
    case BLUFI_ACT_SEND_ERR_INFO:
        ble_blufi_send_error_info(arg->blufi_err_infor.state);
        break;
    case BLUFI_ACT_SEND_CUSTOM_DATA:
        ble_blufi_send_custom_data(arg->custom_data.data, arg->custom_data.data_len);
        data = arg->custom_data.data;
        if(data) {
            free(data);
        }
        break;
    default:
        MODLOG_DFLT_ERROR("%s UNKNOWN %d\n", __func__, act);
        break;
    }
}

void ble_blufi_set_callbacks(blufi_callbacks_t *callbacks)
{
    blufi_env.cbs = callbacks;
}
