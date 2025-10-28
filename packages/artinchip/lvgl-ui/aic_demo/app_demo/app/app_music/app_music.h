#pragma once

lv_obj_t *app_music_init();

static void play_cb(lv_event_t *e);

static void anim_rotate_cb(void *var, int32_t v);

static void play_anim(lv_obj_t *obj);