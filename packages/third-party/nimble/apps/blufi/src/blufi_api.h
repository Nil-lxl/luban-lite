/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BLUFI_API_H__
#define __BLUFI_API_H__

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <aic_common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* BLUFI init complete */
    BLUFI_EVENT_INIT_FINISH = 0,
    /* BLUFI deinit complete */
    BLUFI_EVENT_DEINIT_FINISH,
    /* Phone set Device wifi operation mode(AP/STA/AP_STA) */
    BLUFI_EVENT_SET_WIFI_OPMODE,
    /* Phone connect to Device with BLE */
    BLUFI_EVENT_BLE_CONNECT,
    /* Phone disconnect with BLE */
    BLUFI_EVENT_BLE_DISCONNECT,
    /* Phone request Device's STA connect to AP */
    BLUFI_EVENT_REQ_CONNECT_TO_AP,
    /* Phone request Device's STA disconnect from AP */
    BLUFI_EVENT_REQ_DISCONNECT_FROM_AP,
    /* Phone get Device wifi status */
    BLUFI_EVENT_GET_WIFI_STATUS,
    /* Phone deauthenticate sta from SOFTAP */
    BLUFI_EVENT_DEAUTHENTICATE_STA,
    /* Phone send STA BSSID to Device to connect */
    BLUFI_EVENT_RECV_STA_BSSID,
    /* Phone send STA SSID to Device to connect */
    BLUFI_EVENT_RECV_STA_SSID,
    /* Phone send STA PASSWORD to Device to connect */
    BLUFI_EVENT_RECV_STA_PASSWD,
    /* Phone send SOFTAP SSID to Device to start SOFTAP */
    BLUFI_EVENT_RECV_SOFTAP_SSID,
    /* Phone send SOFTAP PASSWORD to Device to start SOFTAP */
    BLUFI_EVENT_RECV_SOFTAP_PASSWD,
    /* Phone send SOFTAP max connection number to Device to start SOFTAP */
    BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM,
    /* Phone send SOFTAP authentication mode to Device to start SOFTAP */
    BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE,
    /* Phone send SOFTAP channel to Device to start SOFTAP */
    BLUFI_EVENT_RECV_SOFTAP_CHANNEL,
    /* Phone send username to Device */
    BLUFI_EVENT_RECV_USERNAME,
    /* Phone send CA certificate to Device */
    BLUFI_EVENT_RECV_CA_CERT,
    /* Phone send Client certificate to Device */
    BLUFI_EVENT_RECV_CLIENT_CERT,
    /* Phone send Server certificate to Device */
    BLUFI_EVENT_RECV_SERVER_CERT,
    /* Phone send Client Private key to Device */
    BLUFI_EVENT_RECV_CLIENT_PRIV_KEY,
    /* Phone send Server Private key to Device */
    BLUFI_EVENT_RECV_SERVER_PRIV_KEY,
    /* Phone send Disconnect key to Device */
    BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE,
    /* Phone send get wifi list command to Device */
    BLUFI_EVENT_GET_WIFI_LIST,
    /* Blufi report error */
    BLUFI_EVENT_REPORT_ERROR,
    /* Phone send custom data to Device */
    BLUFI_EVENT_RECV_CUSTOM_DATA,
} blufi_cb_event_t;

/// BLUFI config status
typedef enum {
    BLUFI_STA_CONN_SUCCESS = 0x00,
    BLUFI_STA_CONN_FAIL    = 0x01,
    BLUFI_STA_CONNECTING   = 0x02,
    BLUFI_STA_NO_IP        = 0x03,
} blufi_sta_conn_state_t;

/// BLUFI init status
typedef enum {
    BLUFI_INIT_OK = 0,
    BLUFI_INIT_FAILED,
} blufi_init_state_t;

/// BLUFI deinit status
typedef enum {
    BLUFI_DEINIT_OK = 0,
    BLUFI_DEINIT_FAILED,
} blufi_deinit_state_t;

typedef enum {
    BLUFI_SEQUENCE_ERROR = 0,
    BLUFI_CHECKSUM_ERROR,
    BLUFI_DECRYPT_ERROR,
    BLUFI_ENCRYPT_ERROR,
    BLUFI_INIT_SECURITY_ERROR,
    BLUFI_DH_MALLOC_ERROR,
    BLUFI_DH_PARAM_ERROR,
    BLUFI_READ_PARAM_ERROR,
    BLUFI_MAKE_PUBLIC_ERROR,
    BLUFI_DATA_FORMAT_ERROR,
    BLUFI_CALC_MD5_ERROR,
    BLUFI_WIFI_SCAN_FAIL,
    BLUFI_MSG_STATE_ERROR,
} blufi_error_state_t;

/**
 * @brief BLUFI  extra information structure
 */
typedef struct {
    //station
    uint8_t sta_bssid[6];         /* BSSID of station interface */
    bool sta_bssid_set;           /* is BSSID of station interface set */
    uint8_t *sta_ssid;            /* SSID of station interface */
    int sta_ssid_len;             /* length of SSID of station interface */
    uint8_t *sta_passwd;          /* password of station interface */
    int sta_passwd_len;           /* length of password of station interface */
    uint8_t *softap_ssid;         /* SSID of softap interface */
    int softap_ssid_len;          /* length of SSID of softap interface */
    uint8_t *softap_passwd;       /* password of station interface */
    int softap_passwd_len;        /* length of password of station interface */
    uint8_t softap_authmode;      /* authentication mode of softap interface */
    bool softap_authmode_set;     /* is authentication mode of softap interface set */
    uint8_t softap_max_conn_num;  /* max connection number of softap interface */
    bool softap_max_conn_num_set; /* is max connection number of softap interface set */
    uint8_t softap_channel;       /* channel of softap interface */
    bool softap_channel_set;      /* is channel of softap interface set */
    uint8_t sta_max_conn_retry;   /* max retry of sta establish connection */
    bool sta_max_conn_retry_set;  /* is max retry of sta establish connection set */
    uint8_t sta_conn_end_reason;  /* reason of sta connection end */
    bool sta_conn_end_reason_set; /* is reason of sta connection end set */
    int8_t sta_conn_rssi;         /* rssi of sta connection */
    bool sta_conn_rssi_set;       /* is rssi of sta connection set */
} blufi_extra_info_t;

/** @brief Description of an WiFi AP */
typedef struct {
    uint8_t ssid[33]; /* SSID of AP */
    int8_t rssi;      /* signal strength of AP */
} blufi_ap_record_t;

/// Bluetooth address length
#define BLUFI_BD_ADDR_LEN     6
/// Bluetooth device address
typedef uint8_t blufi_bd_addr_t[BLUFI_BD_ADDR_LEN];

/**
 * @brief BLUFI callback parameters union
 */
typedef union {
    /* @brief BLUFI_EVENT_INIT_FINISH */
    struct blufi_init_finish_evt_param {
        blufi_init_state_t state; /* Initial status */
    } init_finish;                /* Blufi callback param of BLUFI_EVENT_INIT_FINISH */

    /* @brief BLUFI_EVENT_DEINIT_FINISH */
    struct blufi_deinit_finish_evt_param {
        blufi_deinit_state_t state; /* De-initial status */
    } deinit_finish;                /* Blufi callback param of BLUFI_EVENT_DEINIT_FINISH */

    /* @brief BLUFI_EVENT_SET_WIFI_MODE */
    struct blufi_set_wifi_mode_evt_param {
        rt_wlan_mode_t op_mode; /* Wifi operation mode */
    } wifi_mode;             /* Blufi callback param of BLUFI_EVENT_INIT_FINISH */

    /* @brief BLUFI_EVENT_CONNECT */
    struct blufi_connect_evt_param {
        blufi_bd_addr_t remote_bda; /* Blufi Remote bluetooth device address */
        uint8_t server_if;          /* server interface */
        uint16_t conn_id;           /* Connection id */
    } connect;                      /* Blufi callback param of BLUFI_EVENT_CONNECT */

    /* @brief BLUFI_EVENT_DISCONNECT */
    struct blufi_disconnect_evt_param {
        blufi_bd_addr_t remote_bda; /* Blufi Remote bluetooth device address */
    } disconnect;                   /* Blufi callback param of BLUFI_EVENT_DISCONNECT */

    /* BLUFI_EVENT_REQ_WIFI_CONNECT */          /* No callback param */
    /* BLUFI_EVENT_REQ_WIFI_DISCONNECT */       /* No callback param */

    /* @brief BLUFI_EVENT_RECV_STA_BSSID */
    struct blufi_recv_sta_bssid_evt_param {
        uint8_t bssid[6]; /* BSSID */
    } sta_bssid;          /* Blufi callback param of BLUFI_EVENT_RECV_STA_BSSID */

    /* @brief BLUFI_EVENT_RECV_STA_SSID */
    struct blufi_recv_sta_ssid_evt_param {
        uint8_t *ssid; /* SSID */
        int ssid_len;  /* SSID length */
    } sta_ssid;        /* Blufi callback param of BLUFI_EVENT_RECV_STA_SSID */

    /* @brief BLUFI_EVENT_RECV_STA_PASSWD */
    struct blufi_recv_sta_passwd_evt_param {
        uint8_t *passwd; /* Password */
        int passwd_len;  /* Password Length */
    } sta_passwd;        /* Blufi callback param of BLUFI_EVENT_RECV_STA_PASSWD */

    /* @brief BLUFI_EVENT_RECV_SOFTAP_SSID */
    struct blufi_recv_softap_ssid_evt_param {
        uint8_t *ssid; /* SSID */
        int ssid_len;  /* SSID length */
    } softap_ssid;     /* Blufi callback param of BLUFI_EVENT_RECV_SOFTAP_SSID */

    /* @brief BLUFI_EVENT_RECV_SOFTAP_PASSWD */
    struct blufi_recv_softap_passwd_evt_param {
        uint8_t *passwd; /* Password */
        int passwd_len;  /* Password Length */
    } softap_passwd;     /* Blufi callback param of BLUFI_EVENT_RECV_SOFTAP_PASSWD */

    /* @brief BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM */
    struct blufi_recv_softap_max_conn_num_evt_param {
        int max_conn_num;  /* SSID */
    } softap_max_conn_num; /* Blufi callback param of BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM */

    /* @brief BLUFI_EVENT_RECV_SOFTAP_CHANNEL */
    struct blufi_recv_softap_channel_evt_param {
        uint8_t channel; /* Authentication mode */
    } softap_channel;    /* Blufi callback param of BLUFI_EVENT_RECV_SOFTAP_CHANNEL */

    /* @brief BLUFI_EVENT_RECV_USERNAME */
    struct blufi_recv_username_evt_param {
        uint8_t *name; /* Username point */
        int name_len;  /* Username length */
    } username;        /* Blufi callback param of BLUFI_EVENT_RECV_USERNAME*/

    /* @brief BLUFI_EVENT_RECV_CA_CERT */
    struct blufi_recv_ca_evt_param {
        uint8_t *cert; /* CA certificate point */
        int cert_len;  /* CA certificate length */
    } ca;              /* Blufi callback param of BLUFI_EVENT_RECV_CA_CERT */

    /* BLUFI_EVENT_RECV_CLIENT_CERT */
    struct blufi_recv_client_cert_evt_param {
        uint8_t *cert; /* Client certificate point */
        int cert_len;  /* Client certificate length */
    } client_cert;     /* Blufi callback param of BLUFI_EVENT_RECV_CLIENT_CERT */

    /* BLUFI_EVENT_RECV_SERVER_CERT */
    struct blufi_recv_server_cert_evt_param {
        uint8_t *cert; /* Client certificate point */
        int cert_len;  /* Client certificate length */
    } server_cert;     /* Blufi callback param of BLUFI_EVENT_RECV_SERVER_CERT */

    /* BLUFI_EVENT_RECV_CLIENT_PRIV_KEY */
    struct blufi_recv_client_pkey_evt_param {
        uint8_t *pkey; /* Client Private Key point, if Client certificate not contain Key */
        int pkey_len;  /* Client Private key length */
    } client_pkey;     /* Blufi callback param of BLUFI_EVENT_RECV_CLIENT_PRIV_KEY */

    /* BLUFI_EVENT_RECV_SERVER_PRIV_KEY */
    struct blufi_recv_server_pkey_evt_param {
        uint8_t *pkey; /* Client Private Key point, if Client certificate not contain Key */
        int pkey_len;  /* Client Private key length */
    } server_pkey;     /* Blufi callback param of BLUFI_EVENT_RECV_SERVER_PRIV_KEY */

    /* @brief BLUFI_EVENT_REPORT_ERROR */
    struct blufi_get_error_evt_param {
        blufi_error_state_t state; /* Blufi error state */
    } report_error;                /* Blufi callback param of BLUFI_EVENT_REPORT_ERROR */
    /* @brief BLUFI_EVENT_RECV_CUSTOM_DATA */
    struct blufi_recv_custom_data_evt_param {
        uint8_t *data;     /* Custom data */
        uint32_t data_len; /* Custom data Length */
    } custom_data;         /* Blufi callback param of BLUFI_EVENT_RECV_CUSTOM_DATA */
} blufi_cb_param_t;

/**
 * @brief BLUFI event callback function type
 * @param event : Event type
 * @param param : Point to callback parameter, currently is union type
 */
typedef void (*blufi_event_cb_t)(blufi_cb_event_t event, blufi_cb_param_t *param);

/* security function declare */

/**
 * @brief BLUFI negotiate data handler
 * @param data : data from phone
 * @param len  : length of data from phone
 * @param output_data : data want to send to phone
 * @param output_len : length of data want to send to phone
 * @param need_free : output reporting if memory needs to be freed or not *
 */
typedef void (*blufi_negotiate_data_handler_t)(uint8_t *data, int len, uint8_t **output_data, int *output_len, bool *need_free);

/**
 * @brief BLUFI  encrypt the data after negotiate a share key
 * @param iv8  : initial vector(8bit), normally, blufi core will input packet sequence number
 * @param crypt_data : plain text and encrypted data, the encrypt function must support autochthonous encrypt
 * @param crypt_len  : length of plain text
 * @return  Nonnegative number is encrypted length, if error, return negative number;
 */
typedef int (*blufi_encrypt_func_t)(uint8_t iv8, uint8_t *crypt_data, int crypt_len);

/**
 * @brief BLUFI  decrypt the data after negotiate a share key
 * @param iv8  : initial vector(8bit), normally, blufi core will input packet sequence number
 * @param crypt_data : encrypted data and plain text, the encrypt function must support autochthonous decrypt
 * @param crypt_len  : length of encrypted text
 * @return  Nonnegative number is decrypted length, if error, return negative number;
 */
typedef int (*blufi_decrypt_func_t)(uint8_t iv8, uint8_t *crypt_data, int crypt_len);

/**
 * @brief BLUFI  checksum
 * @param iv8  : initial vector(8bit), normally, blufi core will input packet sequence number
 * @param data : data need to checksum
 * @param len  : length of data
 */
typedef uint16_t (*blufi_checksum_func_t)(uint8_t iv8, uint8_t *data, int len);

/**
 * @brief BLUFI  callback functions type
 */
typedef struct {
    blufi_event_cb_t event_cb; /* BLUFI event callback */
    blufi_negotiate_data_handler_t negotiate_data_handler; /* BLUFI negotiate data function for negotiate share key */
    blufi_encrypt_func_t encrypt_func; /* BLUFI encrypt data function with share key generated by negotiate_data_handler */
    blufi_decrypt_func_t decrypt_func; /* BLUFI decrypt data function with share key generated by negotiate_data_handler */
    blufi_checksum_func_t checksum_func; /* BLUFI check sum function (FCS) */
} blufi_callbacks_t;

/**
 *
 * @brief           This function is called to receive blufi callback event
 *
 * @param[in]       callbacks: callback functions
 *
 * @return          ESP_OK - success, other - failed
 *
 */
int blufi_register_callbacks(blufi_callbacks_t *callbacks);

/**
 *
 * @brief           This function is called to initialize blufi profile
 *
 * @return          ESP_OK - success, other - failed
 *
 */
int blufi_profile_init(void);

/**
 *
 * @brief           This function is called to de-initialize blufi profile
 *
 * @return          ESP_OK - success, other - failed
 *
 */
int blufi_profile_deinit(void);

/**
 *
 * @brief           Get BLUFI profile version
 *
 * @return          Most 8bit significant is Great version, Least 8bit is Sub version
 *
 */
uint16_t blufi_get_version(void);

/**
 *
 * @brief           This function is called to send blufi error information
 * @param state :  error state
 *
 * @return          ESP_OK - success, other - failed
 *
 */
int blufi_send_error_info(blufi_error_state_t state);
/**
 *
 * @brief           This function is called to custom data
 * @param data :  custom data value
 * @param data_len :  the length of custom data
 *
 * @return          ESP_OK - success, other - failed
 *
 */
int blufi_send_custom_data(uint8_t *data, uint32_t data_len);
#ifdef __cplusplus
}
#endif

#endif /* _BLUFI_API_ */
