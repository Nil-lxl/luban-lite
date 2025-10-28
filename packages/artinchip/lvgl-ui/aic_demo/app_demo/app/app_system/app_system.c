#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "aic_ui.h"
#include "lv_port_disp.h"
#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

#include "lvgl.h"
#include "app_system.h"

static lv_obj_t *system_ui;

lv_obj_t *app_system_init(void) {

    system_ui = lv_obj_create(NULL);

    lv_obj_set_style_bg_img_src(system_ui, LVGL_IMAGE_PATH(main_bg.jpg), 0);

    lv_obj_t *tabview = lv_tabview_create(system_ui);
    // lv_obj_set_size(tabview,1024,400);
    lv_obj_set_align(tabview, LV_ALIGN_BOTTOM_MID);

    lv_tabview_set_tab_bar_position(tabview, LV_DIR_LEFT);
    lv_tabview_set_tab_bar_size(tabview, 200);
    lv_tabview_add_tab(tabview, "Display");
    lv_tabview_add_tab(tabview, "Sound");
    lv_tabview_add_tab(tabview, "tab3");
    lv_tabview_add_tab(tabview, "tab4");
    lv_tabview_add_tab(tabview, "System Info");
    lv_obj_remove_flag(tabview, LV_OBJ_FLAG_SCROLLABLE);


    static lv_style_t tab_normal_style;
    static lv_style_t tab_checked_style;

    lv_style_init(&tab_normal_style);
    lv_style_set_bg_color(&tab_normal_style, lv_color_hex(0xffffff));
    lv_style_set_bg_opa(&tab_normal_style, LV_OPA_40);
    lv_style_set_text_font(&tab_normal_style, &lv_font_montserrat_18);

    lv_style_init(&tab_checked_style);
    lv_style_set_bg_color(&tab_checked_style, lv_color_hex(0xc0c0c0));
    lv_style_set_bg_opa(&tab_checked_style, LV_OPA_100);
    lv_style_set_border_side(&tab_checked_style, LV_BORDER_SIDE_RIGHT);
    lv_style_set_border_color(&tab_checked_style, lv_color_hex(0xc0c0c0));
    lv_style_set_text_color(&tab_checked_style, lv_color_hex(0x000000));
    lv_style_set_text_font(&tab_checked_style, &lv_font_montserrat_18);

    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabview);
    uint32_t count = lv_tabview_get_tab_count(tabview);
    for (int i = 0;i < count;i++) {
        lv_obj_t *button = lv_obj_get_child(tab_bar, i);
        lv_obj_add_style(button, &tab_normal_style, LV_PART_MAIN);
        lv_obj_add_style(button, &tab_checked_style, LV_PART_MAIN | LV_STATE_CHECKED);

    }

    return system_ui;

}
