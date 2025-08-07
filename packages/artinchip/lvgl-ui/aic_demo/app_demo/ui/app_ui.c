#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "lv_port_disp.h"
#include "mpp_fb.h"
#include "app_ui.h"

/**
 * Create a button with a label and react on click event.
 */

#define LCD_HOR_RES PANEL_HACTIVE
#define LCD_VER_RES PANEL_VACTIVE

static lv_obj_t *main_scr;
static lv_obj_t *container;
void app_ui_init() {
    main_scr = lv_scr_act();

    container = lv_obj_create(main_scr);
    lv_obj_align(container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(container, lv_pct(80), lv_pct(60));
    lv_obj_set_scroll_dir(container, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(container, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *item[10];
    for (int i = 0;i < 10;i++) {
        item[i] = lv_btn_create(container);
        lv_obj_set_align(item[i], LV_ALIGN_CENTER);
        lv_obj_set_size(item[i], 150, 150);
        lv_obj_set_pos(item[i], 300 * i, 0);

    }
    lv_obj_scroll_to_view(item[0], LV_ANIM_OFF);

}
void ui_init(void) {
    app_ui_init();
}
