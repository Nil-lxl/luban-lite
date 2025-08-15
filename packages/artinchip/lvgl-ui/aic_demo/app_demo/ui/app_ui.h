#pragma once


void app_ui_init();


enum app_index {
    APP_HOME = 0,
    APP_WIFI,
    APP_PLAYER,
};
typedef int app_ui_index_t;

void application_entrance(app_ui_index_t index, bool auto_del);
app_ui_index_t app_running(void);

static void app_wifi_cb(lv_event_t *e);
static void app_player_cb(lv_event_t *e);
static void back_home_cb(lv_event_t *e);

void hide_home_btn(void);
void show_home_btn(void);


