/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "ui_module.h"
#include "../core/module.h"
#include "../core/msg_def.h"
#include "../core/msg_bus.h"
#include "aic_ui.h"
#include "lvgl.h"
#include "lv_aic_spi.h"
#include "lv_aic_player.h"
#include "aic_hal_gpio.h"
#include "drivers/pin.h"
#include <stdio.h>
#include <rtthread.h>
#include <rtdbg.h>

#define DBG_SECTION_NAME "UI_MOD"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#define MAIN_BACKLIGHT_PIN   "PD.16"
#define MAIN_BACKLIGHT_FUNC  1
#define SLAVE_BACKLIGHT_PIN  "PD.7"
#define SLAVE_BACKLIGHT_FUNC 1

#define DUAL_EYE_COUNT 12

/**
 * WiFi RSSI 0~3 -> LVGL built-in symbols: LV_SYMBOL_WIFI + bar (battery glyphs used as signal bars).
 */
static void ui_format_wifi_signal_icons(char *buf, size_t len, int8_t rssi_level_0_to_3)
{
    const char *bar;

    if (!buf || len == 0)
        return;
    if (rssi_level_0_to_3 < 0 || rssi_level_0_to_3 > 3) {
        rt_snprintf(buf, len, "%s %s", LV_SYMBOL_WIFI, LV_SYMBOL_OK);
        return;
    }
    switch (rssi_level_0_to_3) {
    case 0:
        bar = LV_SYMBOL_BATTERY_EMPTY;
        break;
    case 1:
        bar = LV_SYMBOL_BATTERY_1;
        break;
    case 2:
        bar = LV_SYMBOL_BATTERY_2;
        break;
    case 3:
    default:
        bar = LV_SYMBOL_BATTERY_FULL;
        break;
    }
    rt_snprintf(buf, len, "%s %s", LV_SYMBOL_WIFI, bar);
}

/** Not connected: WiFi + close (FontAwesome-style "off" hint) */
static void ui_format_wifi_disconnected_icon(char *buf, size_t len)
{
    if (!buf || len == 0)
        return;
    rt_snprintf(buf, len, "%s %s", LV_SYMBOL_WIFI, LV_SYMBOL_CLOSE);
}

/** Connecting in progress */
static void ui_format_wifi_connecting_icon(char *buf, size_t len)
{
    if (!buf || len == 0)
        return;
    rt_snprintf(buf, len, "%s %s", LV_SYMBOL_WIFI, LV_SYMBOL_REFRESH);
}

/* UI module private data (must be defined before static functions that reference ui_priv) */
typedef struct {
    lv_obj_t *maser_player;
    lv_obj_t *slave_player;
    lv_obj_t *wifi_icon_spi;     /* SPI top: WiFi icon/signal only */
    lv_obj_t *wifi_status_spi;   /* SPI bottom: status text */
    lv_obj_t *wifi_icon_de;      /* DE top */
    lv_obj_t *wifi_status_de;    /* DE bottom */
    lv_obj_t *scan_logo_img0;
    lv_obj_t *scan_logo_img1;
    wifi_state_t wifi_state;
    emotion_status_t current_emotion;
    rt_bool_t initialized;
} ui_module_priv_t;

static ui_module_priv_t ui_priv = {0};

/* LVGL timer handle: released in ui_deinit */
static lv_timer_t *ui_timer = RT_NULL;

/* Max messages processed per LVGL tick to prevent excessive UI latency from backlog */
#define UI_MSG_DRAIN_MAX  8


/* Bottom status ~25px: prefer 26 (closest built-in), else 24, else default */
static const lv_font_t *ui_wifi_status_font(void)
{
#if defined(LV_FONT_MONTSERRAT_26) && LV_FONT_MONTSERRAT_26
    return &lv_font_montserrat_26;
#elif defined(LV_FONT_MONTSERRAT_24) && LV_FONT_MONTSERRAT_24
    return &lv_font_montserrat_24;
#else
    return lv_font_default();
#endif
}

static void ui_wifi_style_status_label(lv_obj_t *lbl)
{
    if (lbl == RT_NULL)
        return;
    lv_obj_set_style_text_font(lbl, ui_wifi_status_font(), LV_PART_MAIN);
}

static void ui_wifi_set_icon_both(const char *icon_text)
{
    if (ui_priv.wifi_icon_spi)
        lv_label_set_text(ui_priv.wifi_icon_spi, icon_text);
    if (ui_priv.wifi_icon_de)
        lv_label_set_text(ui_priv.wifi_icon_de, icon_text);
}

static void ui_wifi_set_status_both(const char *status_text)
{
    if (ui_priv.wifi_status_spi)
        lv_label_set_text(ui_priv.wifi_status_spi, status_text);
    if (ui_priv.wifi_status_de)
        lv_label_set_text(ui_priv.wifi_status_de, status_text);
}

/* Forward declarations */
static int ui_msg_handler(const app_message_t *msg);
static void ui_handle_wifi_state(wifi_state_t state);
static void ui_handle_emotion_direct(emotion_status_t emotion);

/* Emotion image resources */
static char *dual_eye_src[] = {
    LVGL_PATH(LR1.png),
    LVGL_PATH(LR2.png),
    LVGL_PATH(LR3.png),
    LVGL_PATH(LR4.png),
    LVGL_PATH(LR5.png),
    LVGL_PATH(LR6.png),
    LVGL_PATH(LR7.png),
    LVGL_PATH(LR8.png),
    LVGL_PATH(LR9.png),
    LVGL_PATH(LR10.png),
    LVGL_PATH(LR11.png),
    LVGL_PATH(LR12.png),
};

/* Configure GPIO function */
static void set_gpio_function(const char *pin_name, unsigned char func,
                              unsigned char bias, unsigned char drive)
{
    long pin = 0;
    unsigned int g;
    unsigned int p;
    pin = hal_gpio_name2pin(pin_name);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_set_func(g, p, func);
    hal_gpio_set_bias_pull(g, p, bias);
    hal_gpio_set_drive_strength(g, p, drive);
}

/* Turn on backlight */
static void backlight_on(void)
{
    static u32 main_backligh_pin = 0;
    static u32 slave_backligh_pin = 0;
    set_gpio_function(MAIN_BACKLIGHT_PIN, MAIN_BACKLIGHT_FUNC, PIN_PULL_DIS, 3);
    set_gpio_function(SLAVE_BACKLIGHT_PIN, SLAVE_BACKLIGHT_FUNC, PIN_PULL_DIS, 3);

    main_backligh_pin = rt_pin_get(MAIN_BACKLIGHT_PIN);
    slave_backligh_pin = rt_pin_get(SLAVE_BACKLIGHT_PIN);
    rt_pin_mode(main_backligh_pin, PIN_MODE_OUTPUT);
    rt_pin_mode(slave_backligh_pin, PIN_MODE_OUTPUT);
    rt_pin_write(main_backligh_pin, PIN_HIGH);
    rt_pin_write(slave_backligh_pin, PIN_HIGH);
}

/* Set two-eye emotion */
static void set_dual_eye_src(uint8_t index)
{
    static uint8_t last_index = 0xff;
    if (index == last_index) return;
    last_index = index;

    if (index < DUAL_EYE_COUNT && ui_priv.maser_player != RT_NULL) {
        lv_aic_player_set_src(ui_priv.maser_player, dual_eye_src[index]);
        lv_aic_player_set_auto_restart(ui_priv.maser_player, true);
        lv_aic_player_set_cmd(ui_priv.maser_player, LV_AIC_PLAYER_CMD_START, NULL);
        if (ui_priv.slave_player != RT_NULL) {
            lv_aic_player_set_cmd(ui_priv.maser_player, LV_AIC_PLAYER_CMD_ATTACH_SLAVE,
                                  ui_priv.slave_player);
        }
    }
}

/* Set connecting emotion */
static void set_dual_eye_disp_connecting(void)
{
    static uint8_t last_index = 0xff;
    if (254 == last_index) return;
    last_index = 254;

    if (ui_priv.maser_player != RT_NULL) {
        lv_aic_player_set_src(ui_priv.maser_player, LVGL_PATH(wif_connecting.png));
        lv_aic_player_set_auto_restart(ui_priv.maser_player, true);
        lv_aic_player_set_cmd(ui_priv.maser_player, LV_AIC_PLAYER_CMD_START, NULL);
        if (ui_priv.slave_player != RT_NULL) {
            lv_aic_player_set_cmd(ui_priv.maser_player, LV_AIC_PLAYER_CMD_ATTACH_SLAVE,
                                  ui_priv.slave_player);
        }
    }
}

/* Initialize SPI screen: top WiFi icon + bottom status (~25px font) */
static void init_screen_spi(void *user_data)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    ui_priv.maser_player = lv_aic_player_create(screen);
    lv_obj_align(ui_priv.maser_player, LV_ALIGN_TOP_RIGHT, 0, 0);

    ui_priv.scan_logo_img1 = lv_img_create(screen);
    lv_img_set_src(ui_priv.scan_logo_img1, LVGL_PATH(scan_logo2.png));
    lv_obj_set_pos(ui_priv.scan_logo_img1, 0, 0);
    lv_obj_add_flag(ui_priv.scan_logo_img1, LV_OBJ_FLAG_HIDDEN);

    ui_priv.wifi_icon_spi = lv_label_create(screen);
    lv_label_set_text(ui_priv.wifi_icon_spi, LV_SYMBOL_WIFI " " LV_SYMBOL_REFRESH);
    lv_obj_set_width(ui_priv.wifi_icon_spi, LV_HOR_RES);
    lv_obj_align(ui_priv.wifi_icon_spi, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_text_align(ui_priv.wifi_icon_spi, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    ui_priv.wifi_status_spi = lv_label_create(screen);
    lv_label_set_text(ui_priv.wifi_status_spi, "Connecting");
    lv_label_set_long_mode(ui_priv.wifi_status_spi, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ui_priv.wifi_status_spi, LV_HOR_RES);
    lv_obj_align(ui_priv.wifi_status_spi, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(ui_priv.wifi_status_spi, lv_color_hex(0x5500ff), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui_priv.wifi_status_spi, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    ui_wifi_style_status_label(ui_priv.wifi_status_spi);

    lv_scr_load(screen);
}

/* Initialize DE screen: top icon + bottom status */
static void init_screen_de(void *user_data)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    ui_priv.slave_player = lv_aic_slave_player_create(screen);
    lv_obj_align(ui_priv.slave_player, LV_ALIGN_TOP_LEFT, 0, 0);

    ui_priv.wifi_icon_de = lv_label_create(ui_priv.slave_player);
    lv_label_set_text(ui_priv.wifi_icon_de, LV_SYMBOL_WIFI " " LV_SYMBOL_REFRESH);
    lv_obj_set_width(ui_priv.wifi_icon_de, LV_HOR_RES);
    lv_obj_align(ui_priv.wifi_icon_de, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_text_align(ui_priv.wifi_icon_de, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    ui_priv.wifi_status_de = lv_label_create(ui_priv.slave_player);
    lv_label_set_text(ui_priv.wifi_status_de, "Connecting");
    lv_label_set_long_mode(ui_priv.wifi_status_de, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ui_priv.wifi_status_de, LV_HOR_RES);
    lv_obj_align(ui_priv.wifi_status_de, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(ui_priv.wifi_status_de, lv_color_hex(0x5500ff), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui_priv.wifi_status_de, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    ui_wifi_style_status_label(ui_priv.wifi_status_de);

    ui_priv.scan_logo_img0 = lv_img_create(ui_priv.slave_player);
    lv_img_set_src(ui_priv.scan_logo_img0, LVGL_PATH(scan_logo.png));
    lv_obj_set_pos(ui_priv.scan_logo_img0, 0, 0);
    lv_obj_add_flag(ui_priv.scan_logo_img0, LV_OBJ_FLAG_HIDDEN);

    lv_scr_load(screen);
}

/* Execute operation on specified display */
static void lv_obj_with_disp(lv_disp_t* disp, void (*operation)(void*), void* user_data)
{
    if (!disp || !operation) return;

    lv_disp_t* old_disp = lv_disp_get_default();
    lv_disp_set_default(disp);

    operation(user_data);

    lv_disp_set_default(old_disp);
}

/* Handle WiFi status update (called in LVGL thread): top icon + bottom text */
static void ui_handle_wifi_state(wifi_state_t state)
{
    char icon_line[64];

    ui_priv.wifi_state = state;

    switch (state) {
        case WIFI_STATE_CONNECTED:
        case WIFI_STATE_NET_READY:
            ui_format_wifi_signal_icons(icon_line, sizeof(icon_line), -1);
            ui_wifi_set_icon_both(icon_line);
            ui_wifi_set_status_both("Connected");
            if (ui_priv.scan_logo_img0 != RT_NULL) {
                lv_obj_add_flag(ui_priv.scan_logo_img0, LV_OBJ_FLAG_HIDDEN);
            }
            if (ui_priv.scan_logo_img1 != RT_NULL) {
                lv_obj_add_flag(ui_priv.scan_logo_img1, LV_OBJ_FLAG_HIDDEN);
            }
            set_dual_eye_src(8);
            break;

        case WIFI_STATE_CONNECT_FAILED:
            ui_format_wifi_disconnected_icon(icon_line, sizeof(icon_line));
            ui_wifi_set_icon_both(icon_line);
            ui_wifi_set_status_both("WiFi error");
            break;

        case WIFI_STATE_CONNECTING:
            ui_format_wifi_connecting_icon(icon_line, sizeof(icon_line));
            ui_wifi_set_icon_both(icon_line);
            ui_wifi_set_status_both("WiFi connecting...");
            if (ui_priv.scan_logo_img0 != RT_NULL) {
                lv_obj_add_flag(ui_priv.scan_logo_img0, LV_OBJ_FLAG_HIDDEN);
            }
            if (ui_priv.scan_logo_img1 != RT_NULL) {
                lv_obj_add_flag(ui_priv.scan_logo_img1, LV_OBJ_FLAG_HIDDEN);
            }
            set_dual_eye_disp_connecting();
            break;

        case WIFI_STATE_BROADCAST_START:
            ui_format_wifi_disconnected_icon(icon_line, sizeof(icon_line));
            ui_wifi_set_icon_both(icon_line);
            ui_wifi_set_status_both("Please provision WiFi");
            if (ui_priv.scan_logo_img0 != RT_NULL) {
                lv_obj_clear_flag(ui_priv.scan_logo_img0, LV_OBJ_FLAG_HIDDEN);
            }
            break;

        case WIFI_STATE_PROVISIONING:
            ui_format_wifi_connecting_icon(icon_line, sizeof(icon_line));
            ui_wifi_set_icon_both(icon_line);
            ui_wifi_set_status_both("Provisioning...");
            set_dual_eye_disp_connecting();
            break;

        case WIFI_STATE_NEED_PROVISION:
            ui_format_wifi_disconnected_icon(icon_line, sizeof(icon_line));
            ui_wifi_set_icon_both(icon_line);
            ui_wifi_set_status_both("Use app to configure WiFi");
            if (ui_priv.scan_logo_img0 != RT_NULL) {
                lv_obj_clear_flag(ui_priv.scan_logo_img0, LV_OBJ_FLAG_HIDDEN);
            }
            break;

        case WIFI_STATE_SESSION_FAILED:
            ui_format_wifi_disconnected_icon(icon_line, sizeof(icon_line));
            ui_wifi_set_icon_both(icon_line);
            ui_wifi_set_status_both("WiFi failed (2 tries)");
            break;

        default:
            break;
    }
}

/* Update text by message payload (top signal icon / bottom SSID/MAC etc.) */
static void ui_handle_wifi_message(const app_message_t *msg)
{
    char line[192];
    char icon_line[64];

    if (msg == RT_NULL || ui_priv.wifi_status_spi == RT_NULL) {
        return;
    }

    switch (msg->header.type) {
    case MSG_TYPE_WIFI_CONNECTING:
        ui_handle_wifi_state(WIFI_STATE_CONNECTING);
        if (msg->data.wifi.ssid[0] != '\0') {
            rt_snprintf(line, sizeof(line), "Connecting\n%s", msg->data.wifi.ssid);
            ui_wifi_set_status_both(line);
        }
        break;

    case MSG_TYPE_NET_READY:
    case MSG_TYPE_WIFI_CONNECTED:
        ui_handle_wifi_state(WIFI_STATE_NET_READY);
        if (msg->data.wifi.rssi_level >= 0 && msg->data.wifi.rssi_level <= 3) {
            ui_format_wifi_signal_icons(icon_line, sizeof(icon_line), (int8_t)msg->data.wifi.rssi_level);
        } else {
            ui_format_wifi_signal_icons(icon_line, sizeof(icon_line), -1);
        }
        ui_wifi_set_icon_both(icon_line);
        if (msg->data.wifi.ssid[0] != '\0') {
            rt_snprintf(line, sizeof(line), "Connected\n%s", msg->data.wifi.ssid);
            ui_wifi_set_status_both(line);
        } else {
            ui_wifi_set_status_both("Connected");
        }
        break;

    case MSG_TYPE_WIFI_RSSI_LEVEL:
        if (msg->data.wifi.rssi_level >= 0 && msg->data.wifi.rssi_level <= 3) {
            ui_format_wifi_signal_icons(icon_line, sizeof(icon_line), (int8_t)msg->data.wifi.rssi_level);
            ui_wifi_set_icon_both(icon_line);
        }
        break;

    case MSG_TYPE_WIFI_NEED_PROVISION:
        ui_handle_wifi_state(WIFI_STATE_NEED_PROVISION);
        rt_snprintf(line, sizeof(line), "MAC %s\nProvision required",
                    (msg->data.wifi.mac_str[0] != '\0') ? msg->data.wifi.mac_str : "--");
        ui_wifi_set_status_both(line);
        break;

    case MSG_TYPE_WIFI_SESSION_FAILED:
        ui_handle_wifi_state(WIFI_STATE_SESSION_FAILED);
        if (msg->data.wifi.mac_str[0] != '\0') {
            rt_snprintf(line, sizeof(line), "Failed (2 tries)\nMAC %s", msg->data.wifi.mac_str);
            ui_wifi_set_status_both(line);
        } else {
            ui_wifi_set_status_both("Failed (2 tries)");
        }
        break;

    case MSG_TYPE_WIFI_PROVISIONING:
        ui_handle_wifi_state(WIFI_STATE_PROVISIONING);
        break;

    default:
        break;
    }
}

/* Handle emotion update directly (called in LVGL thread) */
static void ui_handle_emotion_direct(emotion_status_t emotion)
{
    ui_priv.current_emotion = emotion;

    /* Convert emotion to index (enum value minus EMOTION_STATUS_LOW) */
    if (emotion > EMOTION_STATUS_LOW && emotion < EMOTION_STATUS_HIGH) {
        uint8_t index = emotion - EMOTION_STATUS_LOW - 1;
        if (index < DUAL_EYE_COUNT) {
            set_dual_eye_src(index);
        }
    }
}

/* Timer callback (runs in LVGL main thread) */
static void timer_callback(lv_timer_t *timer)
{
    /* Backlight only needs to be enabled once */
    static rt_bool_t ui_backlight_on = RT_FALSE;
    static uint32_t ui_backlight_delay_cnt = 0;

    app_message_t msg;
    uint32_t processed = 0;

    /* Non-blocking message receive: drain as many as possible per tick to reduce UI latency */
    while (processed < UI_MSG_DRAIN_MAX && msg_bus_try_receive(MODULE_ID_UI, &msg) == RT_EOK) {
        ui_msg_handler(&msg);
        processed++;
    }

    /* Backlight control: keep legacy behavior of enabling after 1-2 ticks */
    if (!ui_backlight_on) {
        if (ui_backlight_delay_cnt++ >= 1) {
            backlight_on();
            ui_backlight_on = RT_TRUE;
        }
    }
}

/* UI module message handler */
static int ui_msg_handler(const app_message_t *msg)
{
    if (msg == RT_NULL) {
        return -RT_EINVAL;
    }

    switch (msg->header.type) {
        case MSG_TYPE_SYS_INIT:
            LOG_I("UI module received init message");
            break;

        case MSG_TYPE_WIFI_CONNECTING:
        case MSG_TYPE_WIFI_CONNECTED:
        case MSG_TYPE_NET_READY:
        case MSG_TYPE_WIFI_RSSI_LEVEL:
        case MSG_TYPE_WIFI_NEED_PROVISION:
        case MSG_TYPE_WIFI_SESSION_FAILED:
        case MSG_TYPE_WIFI_PROVISIONING:
            ui_handle_wifi_message(msg);
            break;

        case MSG_TYPE_WIFI_DISCONNECTED:
            /* Handle directly since already in LVGL thread */
            ui_handle_wifi_state(WIFI_STATE_CONNECT_FAILED);
            break;

        case MSG_TYPE_WIFI_BROADCAST_START:
            /* Handle directly since already in LVGL thread */
            ui_handle_wifi_state(WIFI_STATE_BROADCAST_START);
            break;

        case MSG_TYPE_XIAOZHI_EMOTION:
            /* Handle directly since already in LVGL thread */
            ui_handle_emotion_direct(msg->data.emotion.emotion);
            break;

        default:
            LOG_D("UI module received unhandled msg type: 0x%02X", msg->header.type);
            break;
    }

    return RT_EOK;
}

/* UI module init */
static int ui_init(void)
{
    rt_memset(&ui_priv, 0, sizeof(ui_priv));
    ui_priv.wifi_state = WIFI_STATE_WATE_OPEN;
    ui_priv.current_emotion = EMOTION_STATUS_NEUTRAL;

    if (ui_timer != RT_NULL) {
        lv_timer_del(ui_timer);
        ui_timer = RT_NULL;
    }

    /* Initialize display */
    lv_disp_t *spi_disp = lv_disp_get_next(NULL);
    lv_disp_t *de_disp = lv_disp_get_next(spi_disp);

    lv_obj_with_disp(spi_disp, init_screen_spi, NULL);
    lv_obj_with_disp(de_disp, init_screen_de, NULL);

    /* Set initial emotion */
    set_dual_eye_src(5);

    /* Create timer for message handling and backlight control (runs in LVGL main thread) */
    ui_timer = lv_timer_create(timer_callback, 50, NULL);  /* Shorten interval to 50ms for faster message response */
    if (ui_timer == RT_NULL) {
        LOG_E("Failed to create UI timer");
    }

    LOG_I("UI module initialized");

    /* Subscribe messages */
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_WIFI_CONNECTING);
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_WIFI_CONNECTED);
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_WIFI_DISCONNECTED);
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_WIFI_BROADCAST_START);
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_NET_READY);
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_WIFI_RSSI_LEVEL);
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_WIFI_NEED_PROVISION);
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_WIFI_SESSION_FAILED);
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_WIFI_PROVISIONING);
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_XIAOZHI_EMOTION);
    msg_bus_subscribe(MODULE_ID_UI, MSG_TYPE_SYS_INIT);

    ui_priv.initialized = RT_TRUE;

    return RT_EOK;
}

/* UI module deinit */
static int ui_deinit(void)
{
    if (ui_timer != RT_NULL) {
        lv_timer_del(ui_timer);
        ui_timer = RT_NULL;
    }

    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_WIFI_CONNECTING);
    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_WIFI_CONNECTED);
    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_WIFI_DISCONNECTED);
    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_WIFI_BROADCAST_START);
    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_NET_READY);
    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_WIFI_RSSI_LEVEL);
    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_WIFI_NEED_PROVISION);
    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_WIFI_SESSION_FAILED);
    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_WIFI_PROVISIONING);
    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_XIAOZHI_EMOTION);
    msg_bus_unsubscribe(MODULE_ID_UI, MSG_TYPE_SYS_INIT);

    /* Clean up LVGL objects to avoid leftovers on reboot/reinit */
    if (ui_priv.maser_player != RT_NULL) {
        lv_obj_del(ui_priv.maser_player);
        ui_priv.maser_player = RT_NULL;
    }
    if (ui_priv.wifi_icon_spi != RT_NULL) {
        lv_obj_del(ui_priv.wifi_icon_spi);
        ui_priv.wifi_icon_spi = RT_NULL;
    }
    if (ui_priv.wifi_status_spi != RT_NULL) {
        lv_obj_del(ui_priv.wifi_status_spi);
        ui_priv.wifi_status_spi = RT_NULL;
    }
    if (ui_priv.scan_logo_img1 != RT_NULL) {
        lv_obj_del(ui_priv.scan_logo_img1);
        ui_priv.scan_logo_img1 = RT_NULL;
    }

    /* DE: slave_player as container containing wifi_icon_de / wifi_status_de / scan_logo_img0 */
    if (ui_priv.slave_player != RT_NULL) {
        lv_obj_del(ui_priv.slave_player);
        ui_priv.slave_player = RT_NULL;
        ui_priv.wifi_icon_de = RT_NULL;
        ui_priv.wifi_status_de = RT_NULL;
        ui_priv.scan_logo_img0 = RT_NULL;
    }

    ui_priv.initialized = RT_FALSE;

    LOG_I("UI module deinitialized");
    return RT_EOK;
}


/* UI module operations */
static module_ops_t ui_ops = {
    .init = ui_init,
    .deinit = ui_deinit,
    .msg_handler = ui_msg_handler,
    .name = "UI Module",
    .id = MODULE_ID_UI,
};

/* UI module instance */
static module_t ui_module = {
    .ops = &ui_ops,
    .active = RT_FALSE,
    .thread = RT_NULL,
    .priv_data = &ui_priv,
};

/* Get UI module instance */
module_t *ui_module_get(void)
{
    return &ui_module;
}

/* UI module does not require a dedicated thread; it runs in LVGL thread */
int ui_module_thread_start(void)
{
    /* UI module runs in LVGL main thread; no dedicated thread needed */
    LOG_I("UI module runs in LVGL thread, no separate thread needed");
    return RT_EOK;
}

