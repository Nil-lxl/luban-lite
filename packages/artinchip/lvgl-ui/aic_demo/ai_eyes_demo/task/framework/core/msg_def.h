/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#ifndef __MSG_DEF_H__
#define __MSG_DEF_H__

#include <rtthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Message type enum ==================== */

/**
 * Message type enum
 * Each module can define its own message type range
 */
typedef enum {
    /* WiFi module messages (0x00 ~ 0x0F) */
    MSG_TYPE_WIFI_CONNECTING          = 0x01,
    MSG_TYPE_WIFI_CONNECTED           = 0x02,
    MSG_TYPE_WIFI_DISCONNECTED        = 0x03,
    MSG_TYPE_WIFI_SCAN_DONE           = 0x04,
    MSG_TYPE_WIFI_BROADCAST_START     = 0x05,
    MSG_TYPE_NET_READY                = 0x06,
    /* WiFi -> UI: target SSID, signal level, provisioning required, session failure, provisioning state */
    MSG_TYPE_WIFI_RSSI_LEVEL          = 0x07,
    MSG_TYPE_WIFI_NEED_PROVISION      = 0x08,
    MSG_TYPE_WIFI_SESSION_FAILED      = 0x09,
    MSG_TYPE_WIFI_PROVISIONING        = 0x0A,
    /* GPIO/other modules -> WiFi: enter provisioning immediately */
    MSG_TYPE_WIFI_PROVISION_REQUEST   = 0x0B,
    /* Provisioning service -> WiFi: provisioning result */
    MSG_TYPE_WIFI_PROVISION_RESULT    = 0x0C,

    /* XiaoZhi module messages (0x10 ~ 0x2F) */
    MSG_TYPE_XIAOZHI_CONNECTED        = 0x10,
    MSG_TYPE_XIAOZHI_DISCONNECTED     = 0x11,
    MSG_TYPE_XIAOZHI_EMOTION          = 0x12,
    MSG_TYPE_XIAOZHI_AUDIO_SEND       = 0x13,
    MSG_TYPE_XIAOZHI_AUDIO_RECV       = 0x14,
    MSG_TYPE_XIAOZHI_ABORT_SPEAK      = 0x15,
    MSG_TYPE_XIAOZHI_START_LISTEN     = 0x16,

    /* UI module messages (0x30 ~ 0x4F) */
    MSG_TYPE_UI_UPDATE_STATUS         = 0x30,
    MSG_TYPE_UI_UPDATE_EMOTION        = 0x31,
    MSG_TYPE_UI_SHOW_SCAN_LOGO        = 0x32,
    MSG_TYPE_UI_HIDE_SCAN_LOGO        = 0x33,

    /* Audio module messages (0x50 ~ 0x6F) */
    MSG_TYPE_AUDIO_PLAY_PCM           = 0x50,
    MSG_TYPE_AUDIO_RECORD_PCM         = 0x51,
    MSG_TYPE_AUDIO_ENCODE_OPUS        = 0x52,
    MSG_TYPE_AUDIO_DECODE_OPUS        = 0x53,
    MSG_TYPE_AUDIO_SET_VOLUME         = 0x54,
    MSG_TYPE_AUDIO_GET_VOLUME         = 0x55,

    /* GPIO module messages (0x70 ~ 0x7F) */
    MSG_TYPE_GPIO_KEY_PRESSED         = 0x70,
    MSG_TYPE_GPIO_VOICE_WAKE          = 0x71,

    /* OTA auth module messages (0x80 ~ 0x8F) */
    MSG_TYPE_OTA_AUTH_REQUEST         = 0x80,
    MSG_TYPE_OTA_AUTH_SUCCESS         = 0x81,
    MSG_TYPE_OTA_AUTH_FAILED          = 0x82,

    /* System messages (0xF0 ~ 0xFF) */
    MSG_TYPE_SYS_INIT                 = 0xF0,
    MSG_TYPE_SYS_DEINIT               = 0xF1,
    MSG_TYPE_SYS_ERROR                = 0xFE,
    MSG_TYPE_SYS_UNKNOWN              = 0xFF,
} msg_type_t;

/**
 * WiFi state enum
 */
typedef enum {
    WIFI_STATE_WATE_OPEN,
    WIFI_STATE_OPENING,
    WIFI_STATE_SCAN,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_HAVE_CONNECTED_SSID,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_SCAN_FAILED,
    WIFI_STATE_CONNECT_FAILED,
    WIFI_STATE_BROADCAST_START,
    WIFI_STATE_NET_READY,
    WIFI_STATE_PROVISIONING,
    WIFI_STATE_NEED_PROVISION,
    WIFI_STATE_SESSION_FAILED,
} wifi_state_t;

/** msg_wifi_data_t.provision_reason */
#define WIFI_PROVISION_REASON_NEVER_OK     0
#define WIFI_PROVISION_REASON_SESSION_FAIL 1
#define WIFI_PROVISION_REASON_USER         2

/**
 * Emotion state enum
 */
typedef enum {
    EMOTION_STATUS_LOW,
    EMOTION_STATUS_NEUTRAL,
    EMOTION_STATUS_SURPRISED,
    EMOTION_STATUS_SHOCKED,
    EMOTION_STATUS_SAD,
    EMOTION_STATUS_ANGRY,
    EMOTION_STATUS_HAPPY,
    EMOTION_STATUS_THINKING,
    EMOTION_STATUS_LOVING,
    EMOTION_STATUS_KISSY,
    EMOTION_STATUS_SLEEPY,
    EMOTION_STATUS_COOL,
    EMOTION_STATUS_LAUGHING,
    EMOTION_STATUS_FUNNY,
    EMOTION_STATUS_CRYING,
    EMOTION_STATUS_EMBARRASSED,
    EMOTION_STATUS_CONFUSED,
    EMOTION_STATUS_HIGH,
} emotion_status_t;

/* ==================== Message struct definitions ==================== */

/**
 * Base message header
 * All messages must include this header
 */
typedef struct {
    msg_type_t type;          /* Message type */
    uint32_t   src_module;    /* Source module ID */
    uint32_t   dst_module;    /* Destination module ID (0 means broadcast) */
    uint32_t   timestamp;     /* Timestamp */
    uint32_t   data_len;      /* Data length */
} msg_header_t;

/**
 * WiFi connection message data
 */
typedef struct {
    wifi_state_t state;
    char         ssid[64];
    char         password[64];
    int          rssi;              /* Raw dBm, optional */
    int8_t       rssi_level;        /* 0~3, converted by wifi_module for UI */
    uint8_t      provision_reason;  /* WIFI_PROVISION_REASON_* */
    char         mac_str[20];       /* "aa:bb:..." */
    uint8_t      reserved[3];
} msg_wifi_data_t;

/**
 * Emotion message data
 */
typedef struct {
    emotion_status_t emotion;
    char             desc[32];
} msg_emotion_data_t;

/**
 * Audio message data
 */
typedef struct {
    void    *data;        /* Audio data pointer */
    uint32_t data_len;    /* Data length */
    uint32_t sample_rate; /* Sample rate */
    uint16_t channels;    /* Channel count */
    uint16_t bits;        /* Bit depth */
} msg_audio_data_t;

/**
 * GPIO key message data
 */
typedef struct {
    uint32_t pin;
    uint32_t state;       /* 0: released, 1: pressed */
} msg_gpio_data_t;

/**
 * Audio volume message data
 */
typedef struct {
    int32_t volume;       /* Volume value (0-100) */
} msg_audio_volume_data_t;

/**
 * Generic message struct
 */
typedef struct {
    msg_header_t header;
    union {
        msg_wifi_data_t  wifi;
        msg_emotion_data_t emotion;
        msg_audio_data_t audio;
        msg_gpio_data_t  gpio;
        msg_audio_volume_data_t audio_volume;
        uint8_t          raw[256];  /* Raw data */
    } data;
} app_message_t;

/* ==================== Message size definitions ==================== */
#define MSG_HEADER_SIZE       sizeof(msg_header_t)
#define MSG_MAX_DATA_SIZE     256
#define MSG_TOTAL_SIZE        sizeof(app_message_t)

#ifdef __cplusplus
}
#endif

#endif /* __MSG_DEF_H__ */

