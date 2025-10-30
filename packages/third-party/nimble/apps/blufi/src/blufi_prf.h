/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BLUFI_PRF_H__
#define __BLUFI_PRF_H__

#include "blufi_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GATT_UUID_CHAR_CLIENT_CONFIG 0x2902 /* Client Characteristic Configuration */

/* Blufi configuration */
#define BLUFI_SERVICE_UUID  0xFFFF
//define the blufi Char uuid (HOST to DEVICE)
#define BLUFI_CHAR_H2D_UUID 0xFF01
//define the blufi Char uuid (DEVICE to HOST)
#define BLUFI_CHAR_D2H_UUID 0xFF02
//define the blufi Descriptor uuid (DEVICE to HOST)
#define BLUFI_DESCR_D2H_UUID GATT_UUID_CHAR_CLIENT_CONFIG
#define BLUFI_APP_UUID      0xFFFF

#define BLUFI_HDL_NUM   6

#define BD_ADDR_LEN     6   // Device address length


struct pkt_info {
    uint8_t *pkt;
    int pkt_len;
};

typedef enum {
    BLUFI_ACT_INIT = 0,
    BLUFI_ACT_DEINIT,
    BLUFI_ACT_SEND_CFG_REPORT,
    BLUFI_ACT_SEND_WIFI_LIST,
    BLUFI_ACT_SEND_ERR_INFO,
    BLUFI_ACT_SEND_CUSTOM_DATA,
} blufi_act_t;

typedef union {
    /* BLUFI_ACT_SEND_ERR_INFO */
    struct blufi_error_infor {
        blufi_error_state_t state;
    } blufi_err_infor;

    /* BLUFI_ACT_SEND_CUSTOM_DATA */
    struct blufi_custom_data {
         uint8_t *data;
         uint32_t data_len;
    } custom_data;
} blufi_args_t;

/* service engine control block */
typedef struct {
    /* Protocol reference */
    uint8_t gatt_if;
    uint8_t srvc_inst;
    uint16_t handle_srvc;
    uint16_t handle_char_p2e;
    uint16_t handle_char_e2p;
    uint16_t handle_descr_e2p;
    uint16_t conn_id;
    bool is_connected;
    uint8_t remote_bda[BD_ADDR_LEN];
    uint32_t trans_id;
    uint8_t congest;
    uint16_t frag_size;
    // Deprecated: This macro will be removed in the future
#define BLUFI_PREPAIR_BUF_MAX_SIZE 1024
#define BLUFI_PREPARE_BUF_MAX_SIZE 1024
    uint8_t *prepare_buf;
    int prepare_len;
    /* Control reference */
    blufi_callbacks_t *cbs;
    bool enabled;
    uint8_t send_seq;
    uint8_t recv_seq;
    uint8_t sec_mode;
    uint8_t *aggr_buf;
    uint16_t total_len;
    uint16_t offset;
} blufi_env_t;

/* BLUFI protocol */
struct blufi_hdr{
    uint8_t type;
    uint8_t fc;
    uint8_t seq;
    uint8_t data_len;
    uint8_t data[0];
};
typedef struct blufi_hdr blufi_hd_t;

struct blufi_frag_hdr {
    uint8_t type;
    uint8_t fc;
    uint8_t seq;
    uint8_t data_len;
    uint16_t total_len;
    uint8_t content[0];
};
typedef struct blufi_frag_hdr blufi_frag_hdr_t;

#if GATT_DYNAMIC_MEMORY == FALSE
extern blufi_env_t blufi_env;
#else
extern blufi_env_t *blufi_env_ptr;
#define blufi_env (*blufi_env_ptr)
#endif

#define BLUFI_DATA_SEC_MODE_CHECK_MASK  0x01
#define BLUFI_DATA_SEC_MODE_ENC_MASK    0x02
#define BLUFI_CTRL_SEC_MODE_CHECK_MASK  0x10
#define BLUFI_CTRL_SEC_MODE_ENC_MASK    0x20
#define BLUFI_MAX_DATA_LEN              255

// packet type
#define BLUFI_TYPE_MASK         0x03
#define BLUFI_TYPE_SHIFT        0
#define BLUFI_SUBTYPE_MASK      0xFC
#define BLUFI_SUBTYPE_SHIFT     2

#define BLUFI_GET_TYPE(type)            ((type) & BLUFI_TYPE_MASK)
#define BLUFI_GET_SUBTYPE(type)         (((type) & BLUFI_SUBTYPE_MASK) >> BLUFI_SUBTYPE_SHIFT)
#define BLUFI_BUILD_TYPE(type, subtype) (((type) & BLUFI_TYPE_MASK) | ((subtype) << BLUFI_SUBTYPE_SHIFT))

#define BLUFI_TYPE_CTRL                                 0x0
#define BLUFI_TYPE_CTRL_SUBTYPE_ACK                     0x00
#define BLUFI_TYPE_CTRL_SUBTYPE_SET_SEC_MODE            0x01
#define BLUFI_TYPE_CTRL_SUBTYPE_SET_WIFI_OPMODE         0x02
#define BLUFI_TYPE_CTRL_SUBTYPE_CONN_TO_AP              0x03
#define BLUFI_TYPE_CTRL_SUBTYPE_DISCONN_FROM_AP         0x04
#define BLUFI_TYPE_CTRL_SUBTYPE_GET_WIFI_STATUS         0x05
#define BLUFI_TYPE_CTRL_SUBTYPE_DEAUTHENTICATE_STA      0x06
#define BLUFI_TYPE_CTRL_SUBTYPE_GET_VERSION             0x07
#define BLUFI_TYPE_CTRL_SUBTYPE_DISCONNECT_BLE          0x08
#define BLUFI_TYPE_CTRL_SUBTYPE_GET_WIFI_LIST           0x09

#define BLUFI_TYPE_DATA                                 0x1
#define BLUFI_TYPE_DATA_SUBTYPE_NEG                     0x00
#define BLUFI_TYPE_DATA_SUBTYPE_STA_BSSID               0x01
#define BLUFI_TYPE_DATA_SUBTYPE_STA_SSID                0x02
#define BLUFI_TYPE_DATA_SUBTYPE_STA_PASSWD              0x03
#define BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_SSID             0x04
#define BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_PASSWD           0x05
#define BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_MAX_CONN_NUM     0x06
#define BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_AUTH_MODE        0x07
#define BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_CHANNEL          0x08
#define BLUFI_TYPE_DATA_SUBTYPE_USERNAME                0x09
#define BLUFI_TYPE_DATA_SUBTYPE_CA                      0x0a
#define BLUFI_TYPE_DATA_SUBTYPE_CLIENT_CERT             0x0b
#define BLUFI_TYPE_DATA_SUBTYPE_SERVER_CERT             0x0c
#define BLUFI_TYPE_DATA_SUBTYPE_CLIENT_PRIV_KEY         0x0d
#define BLUFI_TYPE_DATA_SUBTYPE_SERVER_PRIV_KEY         0x0e
#define BLUFI_TYPE_DATA_SUBTYPE_WIFI_REP                0x0f
#define BLUFI_TYPE_DATA_SUBTYPE_REPLY_VERSION           0x10
#define BLUFI_TYPE_DATA_SUBTYPE_WIFI_LIST               0x11
#define BLUFI_TYPE_DATA_SUBTYPE_ERROR_INFO              0x12
#define BLUFI_TYPE_DATA_SUBTYPE_CUSTOM_DATA             0x13
#define BLUFI_TYPE_DATA_SUBTYPE_STA_MAX_CONN_RETRY      0x14
#define BLUFI_TYPE_DATA_SUBTYPE_STA_CONN_END_REASON     0x15
#define BLUFI_TYPE_DATA_SUBTYPE_STA_CONN_RSSI           0x16
#define BLUFI_TYPE_IS_CTRL(type)        (BLUFI_GET_TYPE((type)) == BLUFI_TYPE_CTRL)
#define BLUFI_TYPE_IS_DATA(type)        (BLUFI_GET_TYPE((type)) == BLUFI_TYPE_DATA)

#define BLUFI_TYPE_IS_CTRL_ACK(type)                 (BLUFI_TYPE_IS_CTRL((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_CTRL_SUBTYPE_ACK)
#define BLUFI_TYPE_IS_CTRL_START_NEG(type)           (BLUFI_TYPE_IS_CTRL((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_CTRL_SUBTYPE_START_NEG)
#define BLUFI_TYPE_IS_CTRL_STOP_NEG(type)            (BLUFI_TYPE_IS_CTRL((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_CTRL_SUBTYPE_STOP_NEG)
#define BLUFI_TYPE_IS_CTRL_SET_WIFI_OPMODE(type)     (BLUFI_TYPE_IS_CTRL((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_CTRL_SUBTYPE_SET_WIFI_OPMODE)
#define BLUFI_TYPE_IS_CTRL_CONN_WIFI(type)           (BLUFI_TYPE_IS_CTRL((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_CTRL_SUBTYPE_CONN_TO_AP)
#define BLUFI_TYPE_IS_CTRL_DISCONN_WIFI(type)        (BLUFI_TYPE_IS_CTRL((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_CTRL_SUBTYPE_DISCONN_FROM_AP)
#define BLUFI_TYPE_IS_CTRL_GET_WIFI_STATUS(type)     (BLUFI_TYPE_IS_CTRL((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_CTRL_SUBTYPE_GET_WIFI_STATUS)
#define BLUFI_TYPE_IS_CTRL_DEAUTHENTICATE_STA(type)  (BLUFI_TYPE_IS_CTRL((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_CTRL_SUBTYPE_DEAUTHENTICATE_STA)
#define BLUFI_TYPE_IS_CTRL_GET_VERSION(type)         (BLUFI_TYPE_IS_CTRL((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_CTRL_SUBTYPE_GET_VERSION)

#define BLUFI_TYPE_IS_DATA_NEG(type)                 (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_NEG)
#define BLUFI_TYPE_IS_DATA_STA_BSSID(type)           (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_STA_BSSID)
#define BLUFI_TYPE_IS_DATA_STA_SSID(type)            (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_STA_SSID)
#define BLUFI_TYPE_IS_DATA_STA_PASSWD(type)          (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_STA_PASSWD)
#define BLUFI_TYPE_IS_DATA_SOFTAP_SSID(type)         (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_SSID)
#define BLUFI_TYPE_IS_DATA_SOFTAP_PASSWD(type)       (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_PASSWD)
#define BLUFI_TYPE_IS_DATA_SOFTAP_MAX_CONN_NUM(type) (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_MAX_CONN_NUM)
#define BLUFI_TYPE_IS_DATA_SOFTAP_AUTH_MODE(type)    (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_AUTH_MODE)
#define BLUFI_TYPE_IS_DATA_SOFTAP_CHANNEL(type)      (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_CHANNEL)
#define BLUFI_TYPE_IS_DATA_USERNAME(type)            (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_USERNAME)
#define BLUFI_TYPE_IS_DATA_CA(type)                  (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_CA)
#define BLUFI_TYPE_IS_DATA_CLEINT_CERT(type)         (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_CLIENT_CERT)
#define BLUFI_TYPE_IS_DATA_SERVER_CERT(type)         (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_SERVER_CERT)
#define BLUFI_TYPE_IS_DATA_CLIENT_PRIV_KEY(type)     (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_CLIENT_PRIV_KEY)
#define BLUFI_TYPE_IS_DATA_SERVER_PRIV_KEY(type)     (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_SERVER_PRIV_KEY)
#define BLUFI_TYPE_IS_DATA_ERROR_INFO(type)          (BLUFI_TYPE_IS_DATA((type)) && BLUFI_GET_SUBTYPE((type)) == BLUFI_TYPE_DATA_SUBTYPE_ERROR_INFO)

// packet frame control
#define BLUFI_FC_ENC_MASK       0x01
#define BLUFI_FC_CHECK_MASK     0x02
#define BLUFI_FC_DIR_MASK       0x04
#define BLUFI_FC_REQ_ACK_MASK   0x08
#define BLUFI_FC_FRAG_MASK      0x10

#define BLUFI_FC_ENC            0x01
#define BLUFI_FC_CHECK          0x02
#define BLUFI_FC_DIR_P2E        0x00
#define BLUFI_FC_DIR_E2P        0x04
#define BLUFI_FC_REQ_ACK        0x08
#define BLUFI_FC_FRAG           0x10

#define BLUFI_FC_IS_ENC(fc)       ((fc) & BLUFI_FC_ENC_MASK)
#define BLUFI_FC_IS_CHECK(fc)     ((fc) & BLUFI_FC_CHECK_MASK)
#define BLUFI_FC_IS_DIR_H2D(fc)   ((fc) & BLUFI_FC_DIR_H2D_MASK)
#define BLUFI_FC_IS_DIR_D2H(fc)   (!((fc) & BLUFI_DIR_H2D_MASK))
#define BLUFI_FC_IS_REQ_ACK(fc)   ((fc) & BLUFI_FC_REQ_ACK_MASK)
#define BLUFI_FC_IS_FRAG(fc)      ((fc) & BLUFI_FC_FRAG_MASK)

/* default GATT MTU size over LE link
*/
#define GATT_DEF_BLE_MTU_SIZE               23
/* BLUFI HEADER + TOTAL(REMAIN) LENGTH + CRC + L2CAP RESERVED */
#define BLUFI_MTU_RESERVED_SIZE     (sizeof(struct blufi_hdr) + 2 + 2 + 3)
#define BLUFI_FRAG_DATA_DEFAULT_LEN (GATT_DEF_BLE_MTU_SIZE - BLUFI_MTU_RESERVED_SIZE)

void ble_blufi_call_handler(uint8_t act, blufi_args_t *arg);
void ble_blufi_set_callbacks(blufi_callbacks_t *callbacks);

void ble_blufi_recv_handler(uint8_t *data, int len);
void ble_blufi_send_notify(uint8_t *pkt, int pkt_len);
void ble_blufi_send_encap(uint8_t type, uint8_t *data, int total_data_len);

#ifdef __cplusplus
}
#endif

#endif
