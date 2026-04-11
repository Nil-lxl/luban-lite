/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "aic_ui.h"
#include "lvgl.h"
#include "lv_aic_player.h"

/* Demo mode selection:
 * 0 - Normal single player playback
 * 1 - Master-slave configuration (single source)
 * 2 - Group synchronization (multiple sources)
 * 3 - Playback rate control with slider (0.1x ~ 10.0x)
 */
#define PNG_DEMO_TYPE 3

static char *g_apng_source[] = {
    LVGL_PATH(ayanami_rei.png),
    LVGL_PATH(world-cup.png),
    LVGL_PATH(clock.png),
};

#define SRC_NUM (sizeof(g_apng_source) / sizeof(g_apng_source[0]))

static lv_obj_t *create_apng_label(lv_obj_t *parent, char *text);
static lv_obj_t *create_apng_player(lv_obj_t *parent, void *src);

#if PNG_DEMO_TYPE == 0
static int g_normal_player_index = 0;
static void normal_apng_src_switch_cb(lv_timer_t *timer);
static void normal_apng_init(void);
#elif PNG_DEMO_TYPE == 1
static int g_slave_player_index = 0;
static void slave_apng_src_switch_cb(lv_timer_t *timer);
static void slave_apng_init(void);
#elif PNG_DEMO_TYPE == 2
static int g_sync_player_index = 0;
static void group_sync_apng_group_set_sync(lv_obj_t *group, lv_obj_t *player);
static void group_sync_apng_src_switch_cb(lv_timer_t *timer);
static void group_sync_apng_init(void);
#elif PNG_DEMO_TYPE == 3
static void rate_control_demo_init(void);
static void rate_slider_event_cb(lv_event_t *e);
#endif

void aic_png_demo()
{
#if PNG_DEMO_TYPE == 0
    normal_apng_init();
#elif PNG_DEMO_TYPE == 1
    slave_apng_init(); // a single src case
#elif PNG_DEMO_TYPE == 2
    group_sync_apng_init(); // multiple src case
#elif PNG_DEMO_TYPE == 3
    rate_control_demo_init(); // rate control with slider
#endif
}

static lv_obj_t *create_apng_label(lv_obj_t *parent, char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_bg_opa(label, LV_OPA_100, 0);
    lv_obj_set_style_bg_color(label, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    return label;
}

static lv_obj_t *create_apng_player(lv_obj_t *parent, void *src)
{
    lv_obj_t *player = lv_aic_player_create(parent);
    lv_aic_player_set_auto_restart(player, true);
    lv_aic_player_set_src(player, src);
    return player;
}

#if PNG_DEMO_TYPE == 0
static void normal_apng_src_switch_cb(lv_timer_t *timer)
{
    lv_obj_t *player = (lv_obj_t *)timer->user_data;
    g_normal_player_index++;
    if (g_normal_player_index > SRC_NUM - 1)
        g_normal_player_index = 0;
    lv_aic_player_set_src(player, g_apng_source[g_normal_player_index]);
    lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
}

static void normal_apng_init(void)
{
    lv_obj_t *player = create_apng_player(lv_scr_act(), g_apng_source[g_normal_player_index]);
    lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
    lv_obj_align(player, LV_ALIGN_CENTER, 0, 0);

    create_apng_label(player, "normal apng");

    lv_timer_create(normal_apng_src_switch_cb, 100, player);
}
#elif PNG_DEMO_TYPE == 1
static void slave_apng_src_switch_cb(lv_timer_t *timer)
{
    lv_obj_t *player = (lv_obj_t *)timer->user_data;
    g_slave_player_index++;
    if (g_slave_player_index > SRC_NUM - 1)
        g_slave_player_index = 0;
    lv_aic_player_set_src(player, g_apng_source[g_slave_player_index]);
    lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
}

static void slave_apng_init(void)
{
    lv_obj_t *master = create_apng_player(lv_scr_act(), g_apng_source[g_slave_player_index]);
    lv_aic_player_set_cmd(master, LV_AIC_PLAYER_CMD_START, NULL);
    lv_obj_align(master, LV_ALIGN_TOP_LEFT, 0, 0);

    create_apng_label(master, "master apng");

    lv_obj_t *slave0 = lv_aic_slave_player_create(lv_scr_act());
    lv_obj_align(slave0, LV_ALIGN_TOP_MID, 0, 0);

    create_apng_label(slave0, "slaver 1 apng");

    lv_obj_t *slave1 = lv_aic_slave_player_create(lv_scr_act());
    lv_obj_align(slave1, LV_ALIGN_TOP_RIGHT, 0, 0);

    create_apng_label(slave1, "slaver 2 apng");

    lv_aic_player_set_cmd(master, LV_AIC_PLAYER_CMD_ATTACH_SLAVE, slave0);
    lv_aic_player_set_cmd(master, LV_AIC_PLAYER_CMD_ATTACH_SLAVE, slave1);

    lv_timer_create(slave_apng_src_switch_cb, 20000, master);
}
#elif PNG_DEMO_TYPE == 2
static void group_sync_apng_src_switch_cb(lv_timer_t *timer)
{
    lv_obj_t *scr = (lv_obj_t *)timer->user_data;
    lv_obj_t *group = lv_obj_get_child(scr, -1);
    lv_obj_t *player_1 = lv_obj_get_child(scr, -2);
    lv_obj_t *player_2 = lv_obj_get_child(scr, -3);

    g_sync_player_index++;
    if (g_sync_player_index > SRC_NUM - 1)
        g_sync_player_index = 0;

    int player2_index = (g_sync_player_index + 1) % SRC_NUM;
    lv_aic_player_set_src(player_1, g_apng_source[g_sync_player_index]);
    lv_aic_player_set_src(player_2, g_apng_source[player2_index]);

    lv_aic_player_group_set_cmd(group, LV_AIC_PLAYER_CMD_START, NULL);
}

static void group_sync_apng_group_set_sync(lv_obj_t *group, lv_obj_t *player)
{
    lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_ATTACH_GROUP, group);
    lv_aic_player_group_add(group, player);
}

static void group_sync_apng_init(void)
{
    lv_obj_t *player_1 = create_apng_player(lv_scr_act(), g_apng_source[g_sync_player_index]);
    lv_obj_align(player_1, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    create_apng_label(player_1, "sync player 1");

    int player2_index = (g_sync_player_index + 1) % SRC_NUM;
    lv_obj_t *player_2 = create_apng_player(lv_scr_act(), g_apng_source[player2_index]);
    lv_obj_align(player_2, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    create_apng_label(player_2, "sync player 2");

    /* control by group to ensure uniform playback speed among group members. */
    lv_obj_t *group = lv_aic_player_group_create(lv_scr_act());
    group_sync_apng_group_set_sync(group, player_1);
    group_sync_apng_group_set_sync(group, player_2);

    lv_aic_player_group_set_cmd(group, LV_AIC_PLAYER_CMD_START, NULL);

    lv_timer_create(group_sync_apng_src_switch_cb, 30000, lv_scr_act());
}
#elif PNG_DEMO_TYPE == 3
typedef struct {
    lv_obj_t *player;
    lv_obj_t *rate_label;
} rate_control_data_t;

/* Slider value to rate conversion (segmented mapping)
 * 0-30:   0.1x ~ 0.5x  (slow motion, 30 ticks for 0.4 range)
 * 30-50:  0.5x ~ 1.0x  (approaching normal, 20 ticks for 0.5 range)
 * 50-75:  1.0x ~ 2.0x  (slightly fast, 25 ticks for 1.0 range)
 * 75-100: 2.0x ~ 10.0x (fast forward, 25 ticks for 8.0 range)
 */
static float slider_value_to_rate(int32_t value)
{
    if (value < 30) {
        return 0.1f + (value / 30.0f) * 0.4f;
    } else if (value < 50) {
        return 0.5f + ((value - 30) / 20.0f) * 0.5f;
    } else if (value < 75) {
        return 1.0f + ((value - 50) / 25.0f) * 1.0f;
    } else {
        return 2.0f + ((value - 75) / 25.0f) * 8.0f;
    }
}

static void rate_slider_event_cb(lv_event_t *e)
{
    rate_control_data_t *data = lv_event_get_user_data(e);
    lv_obj_t *slider = lv_event_get_target(e);

    int32_t slider_value = lv_slider_get_value(slider);
    float rate = slider_value_to_rate(slider_value);

    lv_aic_player_set_cmd(data->player, LV_AIC_PLAYER_CMD_SET_PLAYBACK_RATE, &rate);

    int rate_display = (int)(rate * 10);
    lv_label_set_text_fmt(data->rate_label, "Rate: %d.%dx", rate_display / 10, rate_display % 10);
}

static void rate_control_demo_init(void)
{
    lv_obj_t *player = create_apng_player(lv_scr_act(), g_apng_source[0]);
    lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
    lv_obj_align(player, LV_ALIGN_TOP_MID, 0, 20);

    create_apng_label(player, "rate control demo");

    lv_obj_t *panel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(panel, 300, 150);
    lv_obj_align(panel, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_opa(panel, LV_OPA_70, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 15, 0);

    lv_obj_t *title_label = lv_label_create(panel);
    lv_label_set_text(title_label, "Playback Rate Control");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xffffff), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *rate_label = lv_label_create(panel);
    lv_label_set_text(rate_label, "Rate: 1.0x");
    lv_obj_set_style_text_color(rate_label, lv_color_hex(0x00ff00), 0);
    lv_obj_align(rate_label, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *slider = lv_slider_create(panel);
    lv_obj_set_width(slider, 260);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);  /* Default 1.0x */

    static rate_control_data_t user_data;
    user_data.player = player;
    user_data.rate_label = rate_label;

    lv_obj_add_event_cb(slider, rate_slider_event_cb, LV_EVENT_VALUE_CHANGED, &user_data);
}
#endif
