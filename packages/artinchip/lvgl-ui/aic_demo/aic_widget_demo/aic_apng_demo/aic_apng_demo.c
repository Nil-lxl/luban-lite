/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "aic_ui.h"
#include "lvgl.h"
#include "lv_aic_player.h"

#define PNG_DEMO_TYPE 0

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
#else
static int g_sync_player_index = 0;
static void group_sync_apng_group_set_sync(lv_obj_t *group, lv_obj_t *player);
static void group_sync_apng_src_switch_cb(lv_timer_t *timer);
static void group_sync_apng_init(void);
#endif

void aic_png_demo()
{
#if PNG_DEMO_TYPE == 0
    normal_apng_init();
#elif PNG_DEMO_TYPE == 1
    slave_apng_init(); // a single src case
#else
    group_sync_apng_init(); // multiple src case
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
#else
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
#endif
