#pragma once

#include "app_dashboard.h"
#include "app_wifi.h"
#include "app_player.h"
#include "app_image.h"
#include "app_weather.h"
#include "app_music.h"
// #include "app_system.h"

void app_ui_init();

typedef enum {
    APP_HOME = 0,
    APP_DASHBOARD,
    APP_WIFI,
    APP_PLAYER,
    APP_WEATHER,
    APP_IMAGE,
    APP_MUSIC,
    APP_SYSTEM,
}app_index_t;

void application_entrance(app_index_t index, bool auto_del);
app_index_t app_running(void);

static void app_dashboard_cb(lv_event_t *e);
static void app_wifi_cb(lv_event_t *e);
static void app_player_cb(lv_event_t *e);
static void app_weather_cb(lv_event_t *e);
static void app_image_cb(lv_event_t *e);
static void app_music_cb(lv_event_t *e);
static void app_system_cb(lv_event_t *e);
static void back_home_cb(lv_event_t *e);

void hide_home_btn(void);
void show_home_btn(void);


