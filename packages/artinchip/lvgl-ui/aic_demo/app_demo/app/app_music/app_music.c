#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "aic_ui.h"
#include "lv_port_disp.h"

#include "lvgl.h"
#include "app_music.h"

#define MUSIC_PATH(y) CONN(LVGL_DIR"image/music/", y)

static lv_obj_t *music_ui;
static lv_obj_t *top_cont;
static lv_obj_t *mid_cont;
static lv_obj_t *bottom_cont;
static lv_obj_t *cover_img;

static lv_obj_t *top_btns[4];
static lv_obj_t *bottom_btns[5];

lv_obj_t *app_music_init(void) {

    music_ui = lv_obj_create(NULL);

    // lv_obj_set_style_bg_img_src(music_ui, LVGL_IMAGE_PATH(main_bg.jpg), 0);
    lv_obj_set_style_bg_opa(music_ui, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(music_ui, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);

    static lv_style_t normal_style;
    lv_style_init(&normal_style);
    lv_style_set_border_width(&normal_style, 0);
    lv_style_set_radius(&normal_style, 0);
    lv_style_set_bg_opa(&normal_style, LV_OPA_0);
    // lv_style_set_bg_color(&normal_style, lv_color_hex(0x008080));

    static lv_style_t flex_style;
    lv_style_init(&flex_style);
    lv_style_set_layout(&flex_style, LV_LAYOUT_FLEX);
    lv_style_set_flex_main_place(&flex_style, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_style_set_flex_flow(&flex_style, LV_FLEX_FLOW_ROW_WRAP);

    top_cont = lv_obj_create(music_ui);
    lv_obj_set_size(top_cont, lv_pct(80), 40);
    lv_obj_align(top_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_style(top_cont, &normal_style, 0);
    lv_obj_add_style(top_cont, &flex_style, 0);
    lv_obj_set_style_pad_ver(top_cont, 5, 0);

    for (int i = 0;i < 4;i++) {
        top_btns[i] = lv_btn_create(top_cont);
        lv_obj_set_size(top_btns[i], 40, 40);
        lv_obj_set_style_bg_opa(top_btns[i], LV_OPA_0, 0);
        lv_obj_set_style_radius(top_btns[i], 0, 0);
        lv_obj_set_style_border_width(top_btns[i], 0, 0);
        lv_obj_set_style_shadow_width(top_btns[i], 0, 0);
    }
    lv_obj_set_style_bg_img_src(top_btns[0], MUSIC_PATH(heart.png), 0);
    lv_obj_set_style_bg_img_src(top_btns[1], MUSIC_PATH(chart.png), 0);
    lv_obj_set_style_bg_img_src(top_btns[2], MUSIC_PATH(download.png), 0);
    lv_obj_set_style_bg_img_src(top_btns[3], MUSIC_PATH(chat.png), 0);

    mid_cont = lv_obj_create(music_ui);
    lv_obj_set_size(mid_cont, 266, 266);
    lv_obj_align_to(mid_cont, top_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 50);
    lv_obj_set_style_border_width(mid_cont, 0, 0);
    lv_obj_set_style_radius(mid_cont, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_img_src(mid_cont, MUSIC_PATH(cover.png), 0);
    lv_obj_clear_flag(mid_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_transform_pivot_x(mid_cont, lv_pct(50), 0);
    lv_obj_set_style_transform_pivot_y(mid_cont, lv_pct(50), 0);

    // cover_img = lv_img_create(mid_cont);
    // lv_obj_center(cover_img);
    // lv_img_set_src(cover_img, MUSIC_PATH(cover.png));

    bottom_cont = lv_obj_create(music_ui);
    lv_obj_set_size(bottom_cont, lv_pct(100), 150);
    lv_obj_align_to(bottom_cont, mid_cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 50);
    lv_obj_add_style(bottom_cont, &normal_style, 0);
    lv_obj_add_style(bottom_cont, &flex_style, 0);
    lv_obj_set_style_pad_ver(bottom_cont, 20, 0);


    for (int i = 0;i < 5;i++) {
        bottom_btns[i] = lv_btn_create(bottom_cont);
        lv_obj_set_size(bottom_btns[i], 100, 100);
        lv_obj_set_style_bg_opa(bottom_btns[i], LV_OPA_0, 0);
        lv_obj_set_style_radius(bottom_btns[i], 0, 0);
        lv_obj_set_style_border_width(bottom_btns[i], 0, 0);
        lv_obj_set_style_shadow_width(bottom_btns[i], 0, 0);
    }
    lv_obj_set_style_bg_img_src(bottom_btns[0], MUSIC_PATH(random.png), 0);
    lv_obj_set_style_bg_img_src(bottom_btns[1], MUSIC_PATH(prev.png), 0);
    lv_obj_set_style_bg_img_src(bottom_btns[2], MUSIC_PATH(play.png), 0);
    lv_obj_set_style_bg_img_src(bottom_btns[3], MUSIC_PATH(next.png), 0);
    lv_obj_set_style_bg_img_src(bottom_btns[4], MUSIC_PATH(loop.png), 0);

    lv_obj_add_flag(bottom_btns[2], LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_img_src(bottom_btns[2], MUSIC_PATH(pause.png), LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(bottom_btns[2], LV_OPA_0, LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(bottom_btns[2], 0, LV_STATE_CHECKED);
    lv_obj_set_style_border_width(bottom_btns[2], 0, LV_STATE_CHECKED);
    lv_obj_add_event_cb(bottom_btns[2], play_cb, LV_EVENT_ALL, NULL);

    return music_ui;

}

static void play_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
            play_anim(mid_cont);
        } else {
            lv_anim_del(mid_cont, anim_rotate_cb);
        }
    }
}
static void anim_rotate_cb(void *var, int32_t v) {
    lv_obj_set_style_transform_angle(var, v, 0);
}
static void play_anim(lv_obj_t *obj) {
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, obj);
    lv_anim_set_values(&anim, 0, 3600);
    lv_anim_set_time(&anim, 5000);
    lv_anim_set_path_cb(&anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&anim, anim_rotate_cb);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&anim);
}