#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mpp_fb.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "aic_ui.h"
#include "app_ui.h"
/**
 * Create a button with a label and react on click event.
 */

#ifdef AIC_PANEL_CUSTOM_RESOLUTION
#define LCD_HOR_RES PANEL_HACTIVE
#define LCD_VER_RES PANEL_VACTIVE
#else
#define LCD_HOR_RES PANEL_HACTIVE_RES
#define LCD_VER_RES PANEL_VACTIVE_RES
#endif

#define APP_NUM 8

static lv_obj_t *app_home_init();
extern lv_obj_t *app_wifi_init();
extern lv_obj_t *app_player_init();

static lv_obj_t *player;
static lv_obj_t *app_list[APP_NUM];  //app list
static lv_obj_t *app_page[APP_NUM];
static lv_obj_t *back_home_btn;

static char current_scr[64];
const char *app_name[] = { "WIFI", "VIDEO", "app3", "app4", "app5", "app6", "app7", "app8" };

lv_obj_t *app_home_init(void) {
    lv_obj_t *ui_home = lv_obj_create(NULL);
    lv_obj_set_style_bg_img_src(ui_home, LVGL_IMAGE_PATH(main_bg.jpg), 0);

    lv_obj_t *container = lv_obj_create(ui_home);
    lv_obj_align(container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(container, lv_pct(100), lv_pct(60));
    lv_obj_set_scroll_dir(container, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(container, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(container, LV_OPA_0, 0);
    lv_obj_set_style_border_width(container, LV_BORDER_SIDE_NONE, 0);

    lv_obj_t *app_label[APP_NUM];
    for (int i = 0;i < APP_NUM;i++) {
        app_list[i] = lv_btn_create(container);
        lv_obj_set_align(app_list[i], LV_ALIGN_CENTER);
        lv_obj_set_size(app_list[i], 150, 200);
        lv_obj_set_pos(app_list[i], 300 * i, 0);

        app_label[i] = lv_label_create(app_list[i]);
        lv_label_set_text(app_label[i], app_name[i]);
        lv_obj_set_align(app_label[i], LV_ALIGN_BOTTOM_MID);
    }
    lv_obj_scroll_to_view(app_list[0], LV_ANIM_OFF);
    lv_obj_add_event_cb(app_list[0], app_wifi_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(app_list[1], app_player_cb, LV_EVENT_CLICKED, NULL);

    return ui_home;
}

static void app_wifi_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        application_entrance(APP_WIFI, true);
    }
}
static void app_player_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        application_entrance(APP_PLAYER, true);
    }
}
static void back_home_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        application_entrance(APP_HOME, true);
    }
}

void hide_home_btn(void) {
    lv_obj_add_flag(back_home_btn, LV_OBJ_FLAG_HIDDEN);     //隐藏home按钮
}
void show_home_btn(void) {
    lv_obj_clear_flag(back_home_btn, LV_OBJ_FLAG_HIDDEN);   //显示home按钮
}

void application_entrance(app_index_t index, bool auto_del) {
    lv_obj_t *scr = NULL;
    switch (index) {
        case APP_HOME:
            hide_home_btn();
            scr = app_home_init();
            strncpy(current_scr, "app_home", sizeof(current_scr));
            break;
        case APP_WIFI:
            show_home_btn();
            scr = app_wifi_init();
            strncpy(current_scr, "app_wifi", sizeof(current_scr));
            break;
        case APP_PLAYER:
            show_home_btn();
            scr = app_player_init();
            strncpy(current_scr, "app_player", sizeof(current_scr));
            break;
        default:
            break;
    }
    if (scr && auto_del == true) {
        lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_NONE, 100, 5, true);
    } else if (scr && auto_del == false) {
        lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_NONE, 100, 5, false);
    }
}
void app_ui_init() {
    aicos_msleep(1000);//等待sdcard挂载成功

    back_home_btn = lv_btn_create(lv_layer_top());  //创建home按钮
    lv_obj_set_size(back_home_btn, 66, 66);
    lv_obj_set_style_bg_opa(back_home_btn, LV_OPA_0, 0);
    lv_obj_set_style_border_width(back_home_btn, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_shadow_width(back_home_btn, 0, 0);
    lv_obj_set_style_radius(back_home_btn, 0, 0);
    lv_obj_set_style_bg_img_src(back_home_btn, LVGL_IMAGE_PATH(home_released.png), 0);
    lv_obj_set_style_bg_img_src(back_home_btn, LVGL_IMAGE_PATH(home_pressed.png), LV_STATE_PRESSED);
    lv_obj_align(back_home_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(back_home_btn, back_home_cb, LV_EVENT_CLICKED, NULL);
    hide_home_btn();

    application_entrance(APP_HOME, false);          //默认加载主界面

}
app_index_t app_running(void) {

}

void ui_init(void) {
    app_ui_init();
}
