#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mpp_fb.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "aic_ui.h"
#include "lv_aic_player.h"

#include "app_ui.h"

static lv_obj_t *player;

static void player_exit_cb(lv_event_t *e);

void play_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
    }
}
lv_obj_t *app_player_init() {
    lv_obj_t *player_ui = lv_obj_create(NULL);

    // lv_obj_t *play_btn = lv_btn_create(lv_layer_top());
    // lv_obj_set_size(play_btn, 100, 60);
    // lv_obj_set_pos(play_btn, 100, 100);
    // lv_obj_add_event_cb(play_btn, play_cb, LV_EVENT_CLICKED, NULL);

    player = lv_aic_player_create(player_ui);
    lv_aic_player_set_src(player, LVGL_VIDEO_PATH(cartoon.mp4));
    lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);

    lv_obj_center(player);

    // lv_aic_player_set_auto_restart(player, true);
    lv_obj_add_event_cb(player_ui, player_exit_cb, LV_EVENT_ALL, NULL);

    return player_ui;
}
static void player_exit_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        rt_kprintf("exit player ui");
        // lv_obj_clean(obj);
    }
}