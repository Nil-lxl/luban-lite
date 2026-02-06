/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  zequan liang <zequan liang@artinchip.com>
 */

#include "aic_ui.h"
#include "lv_aic_spi.h"
#include "lv_aic_player.h"

/*  the relationship between display, screen, and obj
┌───────────────────────────────────────────────────────────────┐
│                   Physical Displays (disp)                    │
│   ┌─────────────────────┐      ┌─────────────────────┐        │
│   │      SPI Display    │      │      DE Display     │        │
│   └──────────┬──────────┘      └──────────┬──────────┘        │
│              │                            │                   │
│   ┌──────────▼──────────┐      ┌──────────▼──────────┐        │
│   │     Screen 1        │      │     Screen 2        │        │
│   │  (root container)   │      │  (root container)   │        │
│   └──────────┬──────────┘      └──────────┬──────────┘        │
│              │                            │                   │
│   ┌──────────▼──────────┐      ┌──────────▼──────────┐        │
│   │  APNG Player Object │      │  APNG Player Object │        │
│   │ (child widget)      │      │ (child widget)      │        │
│   └─────────────────────┘      └─────────────────────┘        │
└───────────────────────────────────────────────────────────────┘
*/

typedef void (*lv_disp_operation_t)(void* user_data);

static lv_obj_t *lv_aic_apng_create(lv_obj_t *obj, void *src);
static void lv_obj_with_disp(lv_disp_t* disp, lv_disp_operation_t operation, void* user_data);
static void init_screen(void *user_data);

void ui_init(void)
{
    lv_disp_t *spi_disp = lv_disp_get_next(NULL); /* get the head display */
    lv_disp_t *de_disp = lv_disp_get_next(spi_disp);

    if (de_disp == NULL) {
        LV_LOG_ERROR("No DE display found!");
        return;
    }

    lv_obj_with_disp(spi_disp, init_screen, LVGL_PATH(A1_b0.png));

    lv_obj_with_disp(de_disp, init_screen, LVGL_PATH(A2_b0.png));
}

static lv_obj_t *lv_aic_apng_create(lv_obj_t *obj, void *src)
{
    lv_obj_t *player;
    player = lv_aic_player_create(obj);
    if (player == NULL)
        return NULL;
    lv_aic_player_set_src(player, src);
    lv_aic_player_set_auto_restart(player, true);
    lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
    return player;
}

static void lv_obj_with_disp(lv_disp_t* disp, lv_disp_operation_t operation, void* user_data)
{
    if(!disp || !operation) return;

    lv_disp_t* old_disp = lv_disp_get_default();
    lv_disp_set_default(disp);

    operation(user_data);

    lv_disp_set_default(old_disp);
}

static void init_screen(void *user_data)
{
    char *image_path = (char *)user_data;

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_t *player = lv_aic_apng_create(screen, image_path);
    if (player == NULL)
        return;
    lv_obj_align(player, LV_ALIGN_CENTER, 0, 0);
    lv_scr_load(screen);
}
