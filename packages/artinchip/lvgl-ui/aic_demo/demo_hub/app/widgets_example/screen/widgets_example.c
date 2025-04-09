#include "widgets_example.h"
#include "widget_btn_ui.h"
#include "demo_hub.h"
#include "../app_entrance.h"

#define PAGE_WIDGET         0
#define PAGE_WIDGET_CHILD   1

typedef struct _btn_widget_list {
    const char *img_path;
    void (*event_cb)(lv_event_t * e);
} btn_widget_list;

/* complex menu */
enum {
    LV_MENU_ITEM_BUILDER_VARIANT_1,
    LV_MENU_ITEM_BUILDER_VARIANT_2
};
typedef uint8_t lv_menu_builder_variant_t;

static lv_obj_t *widget_btn_init(lv_obj_t *parent);
static lv_obj_t *complex_menu_init(lv_obj_t *parent);
static lv_obj_t *keyboard_init(lv_obj_t *parent);
static lv_obj_t *calendar_init(lv_obj_t *parent);
static lv_obj_t *table_init(lv_obj_t *parent);

#if LVGL_VERSION_MAJOR == 8
/* complex menu */
static lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt,
                              lv_menu_builder_variant_t builder_variant);
static lv_obj_t * create_slider(lv_obj_t * parent,
                                const char * icon, const char * txt, int32_t min, int32_t max, int32_t val);
static lv_obj_t * create_switch(lv_obj_t * parent,
                                const char * icon, const char * txt, bool chk);
#endif

static void ui_widgets_cb(lv_event_t *e);
static void ui_complex_menu_cb(lv_event_t *e);
static void ui_keyboard_cb(lv_event_t *e);
static void ui_calendar_cb(lv_event_t *e);
static void ui_table_cb(lv_event_t *e);
static void exit_cb(lv_event_t *e);

#if LVGL_VERSION_MAJOR == 8
/* complex menu */
static void back_event_handler(lv_event_t *e);
static void switch_handler(lv_event_t *e);
#endif

/* keyboard */
static void ta_event_cb(lv_event_t *e);

/* calder */
static void calender_event_handler(lv_event_t *e);

/* table */
static void draw_part_event_cb(lv_event_t *e);

static void select_disp_obj(lv_obj_t *obj);

static lv_obj_t *widget_contain_ui;
/* complex menu */
static lv_obj_t *complex_menu_ui;
#if LVGL_VERSION_MAJOR == 8
static lv_obj_t *root_page;
#endif
/* keyboard */
static lv_obj_t *keyboard_ui;
static lv_obj_t *kb;

/* calendar */
static lv_obj_t *calendar_ui;

/* table */
static lv_obj_t *table_ui;

static int now_page = 0;

lv_obj_t *widgets_ui_init(void)
{
    static lv_style_t pressed_default;
    lv_obj_t *ui_widgets = lv_obj_create(NULL);
    lv_obj_set_size(ui_widgets, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(ui_widgets, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bg_color = lv_obj_create(ui_widgets);
    lv_obj_remove_style_all(bg_color);
    lv_obj_set_style_bg_opa(bg_color, LV_OPA_100, 0);
    lv_obj_set_style_bg_color(bg_color, lv_theme_get_color_primary(NULL), 0);
    lv_obj_set_size(bg_color, lv_pct(100), lv_pct(100));
    lv_obj_set_align(bg_color, LV_ALIGN_TOP_LEFT);

    widget_contain_ui = widget_btn_init(ui_widgets);
    complex_menu_ui = complex_menu_init(ui_widgets);
    keyboard_ui = keyboard_init(ui_widgets);
    calendar_ui = calendar_init(ui_widgets);
    table_ui = table_init(ui_widgets);

    lv_obj_t *exit = lv_img_create(ui_widgets);
    lv_img_set_src(exit, LVGL_PATH(widgets_example/exit.png));
    lv_obj_add_flag(exit, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(exit, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_pos(exit, -LV_HOR_RES * 0.05, -LV_VER_RES * 0.1);
    lv_obj_set_ext_click_area(exit, LV_HOR_RES * 0.02);

    if (pressed_default.prop_cnt > 1) {
        lv_style_reset(&pressed_default);
    } else {
        lv_style_init(&pressed_default);
    }

    lv_style_set_translate_x(&pressed_default, 2);
    lv_style_set_translate_y(&pressed_default, 2);

    lv_obj_add_event_cb(ui_widgets, ui_widgets_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(exit, exit_cb, LV_EVENT_ALL, NULL);

    select_disp_obj(widget_contain_ui);

    now_page = PAGE_WIDGET;

    return ui_widgets;
}

static lv_obj_t *widget_btn_init(lv_obj_t *parent)
{
    btn_widget_list list[] = {
        {LVGL_PATH(widgets_example/complex_menu.png), ui_complex_menu_cb},
        {LVGL_PATH(widgets_example/keyboard.png), ui_keyboard_cb},
        {LVGL_PATH(widgets_example/calendar.png), ui_calendar_cb},
        {LVGL_PATH(widgets_example/table.png), ui_table_cb},
        {NULL, NULL}
    };

    lv_obj_t *btn_contain = lv_obj_create(parent);
    lv_obj_remove_style_all(btn_contain);
    lv_obj_set_align(btn_contain, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_contain, LV_OPA_0, 0);
    lv_obj_set_size(btn_contain, lv_pct(60), lv_pct(80));
    lv_obj_clear_flag(btn_contain, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(btn_contain, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(btn_contain, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN);

    for (int i = 0; i < sizeof(list) / sizeof(list[0]) - 1; i++) {
        lv_obj_t *btn = widget_btn_ui_create(btn_contain);
        lv_obj_t *btn_label = lv_img_create(btn);
        lv_img_set_src(btn_label, list[i].img_path);
        lv_obj_set_align(btn_label, LV_ALIGN_CENTER);
        lv_obj_add_event_cb(btn, list[i].event_cb, LV_EVENT_ALL, NULL);
    }

    return btn_contain;
}

lv_obj_t *complex_menu_init(lv_obj_t *parent)
{
#if LVGL_VERSION_MAJOR == 8
    lv_obj_t * menu = lv_menu_create(parent);
    lv_obj_set_align(menu, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(menu, lv_theme_get_color_primary(NULL), 0);

    lv_color_t bg_color = lv_obj_get_style_bg_color(menu, 0);
    if(lv_color_brightness(bg_color) > 127) {
        lv_obj_set_style_bg_color(menu, lv_color_darken(lv_obj_get_style_bg_color(menu, 0), 10), 0);
    } else {
        lv_obj_set_style_bg_color(menu, lv_color_darken(lv_obj_get_style_bg_color(menu, 0), 50), 0);
    }
    lv_menu_set_mode_root_back_btn(menu, LV_MENU_ROOT_BACK_BTN_ENABLED);
    lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);
    lv_obj_set_size(menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_center(menu);

    lv_obj_t * cont;
    lv_obj_t * section;

    /*Create sub pages*/
    lv_obj_t * sub_mechanics_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_mechanics_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_mechanics_page);
    section = lv_menu_section_create(sub_mechanics_page);
    create_slider(section, LV_SYMBOL_SETTINGS, "Velocity", 0, 150, 120);
    create_slider(section, LV_SYMBOL_SETTINGS, "Acceleration", 0, 150, 50);
    create_slider(section, LV_SYMBOL_SETTINGS, "Weight limit", 0, 150, 80);

    lv_obj_t * sub_sound_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_sound_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_sound_page);
    section = lv_menu_section_create(sub_sound_page);
    create_switch(section, LV_SYMBOL_AUDIO, "Sound", false);

    lv_obj_t * sub_display_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_display_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_display_page);
    section = lv_menu_section_create(sub_display_page);
    create_slider(section, LV_SYMBOL_SETTINGS, "Brightness", 0, 150, 100);

    lv_obj_t * sub_software_info_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_software_info_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    section = lv_menu_section_create(sub_software_info_page);
    create_text(section, NULL, "Version 1.0", LV_MENU_ITEM_BUILDER_VARIANT_1);

    lv_obj_t * sub_legal_info_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_legal_info_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    section = lv_menu_section_create(sub_legal_info_page);
    for(uint32_t i = 0; i < 15; i++) {
        create_text(section, NULL,
                    "This is a long long long long long long long long long text, if it is long enough it may scroll.",
                    LV_MENU_ITEM_BUILDER_VARIANT_1);
    }

    lv_obj_t * sub_about_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_about_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_about_page);
    section = lv_menu_section_create(sub_about_page);
    cont = create_text(section, NULL, "Software information", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_software_info_page);
    cont = create_text(section, NULL, "Legal information", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_legal_info_page);

    lv_obj_t * sub_menu_mode_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_menu_mode_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_menu_mode_page);
    section = lv_menu_section_create(sub_menu_mode_page);
    cont = create_switch(section, LV_SYMBOL_AUDIO, "Sidebar enable", true);
    lv_obj_add_event_cb(lv_obj_get_child(cont, 2), switch_handler, LV_EVENT_VALUE_CHANGED, menu);

    /*Create a root page*/
    root_page = lv_menu_page_create(menu, "Settings");
    lv_obj_set_style_text_color(menu, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_style_pad_hor(root_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_obj_set_style_text_color(root_page, lv_theme_get_color_emphasize(NULL), 0);
    section = lv_menu_section_create(root_page);
    cont = create_text(section, LV_SYMBOL_SETTINGS, "Mechanics", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_mechanics_page);
    cont = create_text(section, LV_SYMBOL_AUDIO, "Sound", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_sound_page);
    cont = create_text(section, LV_SYMBOL_SETTINGS, "Display", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_display_page);

    lv_obj_t *other_cont = create_text(root_page, NULL, "Others", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_obj_t *other_label = lv_obj_get_child(other_cont, 0);
    lv_obj_set_style_text_color(other_label, lv_theme_get_color_emphasize(NULL), 0);

    section = lv_menu_section_create(root_page);
    cont = create_text(section, NULL, "About", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_about_page);
    cont = create_text(section, LV_SYMBOL_SETTINGS, "Menu mode", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_menu_mode_page);

    lv_menu_set_sidebar_page(menu, root_page);

    lv_event_send(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0), 0), LV_EVENT_CLICKED, NULL);
    return menu;
#else
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, "Did't support menu example!!!");
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    return label;
#endif
}

lv_obj_t *keyboard_init(lv_obj_t *parent)
{
    lv_obj_t *keyboard_contain = lv_obj_create(parent);
    lv_obj_set_size(keyboard_contain, lv_pct(80), lv_pct(80));
    lv_obj_center(keyboard_contain);

    /*Create the password box*/
    lv_obj_t * pwd_ta = lv_textarea_create(keyboard_contain);
    lv_textarea_set_text(pwd_ta, "");
    lv_textarea_set_password_mode(pwd_ta, true);
    lv_textarea_set_one_line(pwd_ta, true);
    lv_obj_set_width(pwd_ta, lv_pct(40));
    lv_obj_set_pos(pwd_ta, LV_HOR_RES * 0.01, LV_VER_RES * 0.01);
    lv_obj_add_event_cb(pwd_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    /*Create a label and position it above the text box*/
    lv_obj_t * pwd_label = lv_label_create(keyboard_contain);
    lv_label_set_text(pwd_label, "Password:");
    lv_obj_align_to(pwd_label, pwd_ta, LV_ALIGN_OUT_TOP_LEFT, 0, 0);

    /*Create the one-line mode text area*/
    lv_obj_t * text_ta = lv_textarea_create(keyboard_contain);
    lv_textarea_set_one_line(text_ta, true);
    lv_textarea_set_password_mode(text_ta, false);
    lv_obj_set_width(text_ta, lv_pct(40));
    lv_obj_add_event_cb(text_ta, ta_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_align(text_ta, LV_ALIGN_TOP_RIGHT, -LV_HOR_RES * 0.01, LV_VER_RES * 0.01);


    /*Create a label and position it above the text box*/
    lv_obj_t * oneline_label = lv_label_create(keyboard_contain);
    lv_label_set_text(oneline_label, "Text:");
    lv_obj_align_to(oneline_label, text_ta, LV_ALIGN_OUT_TOP_LEFT, 0, 0);

    /*Create a keyboard*/
    kb = lv_keyboard_create(keyboard_contain);
    lv_obj_set_align(kb, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_size(kb, lv_pct(100), lv_pct(70));

    lv_keyboard_set_textarea(kb, pwd_ta); /*Focus it on one of the text areas to start*/

    return keyboard_contain;
}


lv_obj_t *calendar_init(lv_obj_t *parent)
{
    lv_obj_t  * calendar = lv_calendar_create(parent);
    lv_obj_set_size(calendar, LV_HOR_RES * 0.6, LV_VER_RES * 0.6);
    lv_obj_align(calendar, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(calendar, calender_event_handler, LV_EVENT_ALL, NULL);

    lv_calendar_set_today_date(calendar, 2021, 02, 23);
    lv_calendar_set_showed_date(calendar, 2021, 02);

    /*Highlight a few days*/
    static lv_calendar_date_t highlighted_days[3];       /*Only its pointer will be saved so should be static*/
    highlighted_days[0].year = 2021;
    highlighted_days[0].month = 02;
    highlighted_days[0].day = 6;

    highlighted_days[1].year = 2021;
    highlighted_days[1].month = 02;
    highlighted_days[1].day = 11;

    highlighted_days[2].year = 2026;
    highlighted_days[2].month = 02;
    highlighted_days[2].day = 22;

    lv_calendar_set_highlighted_dates(calendar, highlighted_days, 3);

#if LV_USE_CALENDAR_HEADER_DROPDOWN
    lv_calendar_header_dropdown_create(calendar);
#elif LV_USE_CALENDAR_HEADER_ARROW
    lv_calendar_header_arrow_create(calendar);
#endif
    lv_calendar_set_showed_date(calendar, 2021, 10);

    return calendar;
}

lv_obj_t *table_init(lv_obj_t *parent)
{
    lv_obj_t * table = lv_table_create(parent);
    lv_obj_set_style_bg_color(table, lv_theme_get_color_primary(NULL), 0);

    /*Fill the first column*/
    lv_table_set_cell_value(table, 0, 0, "Name");
    lv_table_set_cell_value(table, 1, 0, "Apple");
    lv_table_set_cell_value(table, 2, 0, "Banana");
    lv_table_set_cell_value(table, 3, 0, "Lemon");
    lv_table_set_cell_value(table, 4, 0, "Grape");
    lv_table_set_cell_value(table, 5, 0, "Melon");
    lv_table_set_cell_value(table, 6, 0, "Peach");
    lv_table_set_cell_value(table, 7, 0, "Nuts");

    /*Fill the second column*/
    lv_table_set_cell_value(table, 0, 1, "Price");
    lv_table_set_cell_value(table, 1, 1, "$7");
    lv_table_set_cell_value(table, 2, 1, "$4");
    lv_table_set_cell_value(table, 3, 1, "$6");
    lv_table_set_cell_value(table, 4, 1, "$2");
    lv_table_set_cell_value(table, 5, 1, "$5");
    lv_table_set_cell_value(table, 6, 1, "$1");
    lv_table_set_cell_value(table, 7, 1, "$9");

    /*Set a smaller height to the table. It'll make it scrollable*/
    lv_obj_center(table);
    lv_obj_set_size(table, LV_HOR_RES * 0.6, LV_VER_RES * 0.6);
    lv_table_set_col_width(table, 0, LV_HOR_RES * 0.3);
    lv_table_set_col_width(table, 1, LV_HOR_RES * 0.3);

    /*Add an event callback to to apply some custom drawing*/
#if LVGL_VERSION_MAJOR == 8
    lv_obj_add_event_cb(table, draw_part_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
#else
    lv_obj_add_event_cb(table, draw_part_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(table, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
#endif
    return table;
}
#if LVGL_VERSION_MAJOR == 8
static lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt,
                              lv_menu_builder_variant_t builder_variant)
{
    lv_obj_t * obj = lv_menu_cont_create(parent);

    lv_obj_t * img = NULL;
    lv_obj_t * label = NULL;

    if(icon) {
        img = lv_img_create(obj);
        lv_img_set_src(img, icon);
    }

    if(txt) {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
    }

    if(builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt) {
        lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_swap(img, label);
    }

    return obj;
}

static lv_obj_t * create_slider(lv_obj_t * parent, const char * icon, const char * txt, int32_t min, int32_t max,
                                int32_t val)
{
    lv_obj_t * obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);

    lv_obj_t * slider = lv_slider_create(obj);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);

    if(icon == NULL) {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }

    return obj;
}

static lv_obj_t * create_switch(lv_obj_t * parent, const char * icon, const char * txt, bool chk)
{
    lv_obj_t * obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

    lv_obj_t * sw = lv_switch_create(obj);
    lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : 0);

    return obj;
}
#endif

static void ui_complex_menu_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        select_disp_obj(complex_menu_ui);
        now_page = PAGE_WIDGET_CHILD;
    }
}

static void ui_keyboard_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        select_disp_obj(keyboard_ui);
        now_page = PAGE_WIDGET_CHILD;
    }
}

static void ui_calendar_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        select_disp_obj(calendar_ui);
        now_page = PAGE_WIDGET_CHILD;
    }
}

static void ui_table_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        select_disp_obj(table_ui);
        now_page = PAGE_WIDGET_CHILD;
    }
}

static void ui_widgets_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_SCREEN_UNLOAD_START) {;}

    if (code == LV_EVENT_DELETE) {;}
    if (code == LV_EVENT_SCREEN_LOADED) {;}
}

static void exit_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        if (now_page == PAGE_WIDGET_CHILD) {
            select_disp_obj(widget_contain_ui);
            now_page = PAGE_WIDGET;
        } else {
            app_entrance(APP_NAVIGATION, 1);
        }
    }
}
#if LVGL_VERSION_MAJOR == 8
static void back_event_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_t * menu = lv_event_get_user_data(e);

    if(lv_menu_back_btn_is_root(menu, obj)) {
#if LVGL_VERSION_MAJOR == 8
    lv_obj_t * mbox1 = lv_msgbox_create(NULL, "Hello", "Root back btn click.", NULL, true);
#else
    lv_obj_t * mbox1 = lv_msgbox_create(NULL);
    lv_msgbox_add_title(mbox1, "Hello");
    lv_msgbox_add_text(mbox1, "Root back btn click.");
    lv_msgbox_add_close_button(mbox1);
#endif
    lv_obj_center(mbox1);
    }
}

static void switch_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * menu = lv_event_get_user_data(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        if(lv_obj_has_state(obj, LV_STATE_CHECKED)) {
            lv_menu_set_page(menu, NULL);
            lv_menu_set_sidebar_page(menu, root_page);
            lv_event_send(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0), 0), LV_EVENT_CLICKED, NULL);
        } else {
            lv_menu_set_sidebar_page(menu, NULL);
            lv_menu_clear_history(menu); /* Clear history because we will be showing the root page later */
            lv_menu_set_page(menu, root_page);
        }
    }
}
#endif
static void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED) {
        /*Focus on the clicked text area*/
        if(kb != NULL) lv_keyboard_set_textarea(kb, ta);
    } else if(code == LV_EVENT_READY) {
        LV_LOG_USER("Ready, current text: %s", lv_textarea_get_text(ta));
    }
}

static void calender_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        lv_calendar_date_t date;
        if(lv_calendar_get_pressed_date(obj, &date)) {
            LV_LOG_USER("Clicked date: %02d.%02d.%d", date.day, date.month, date.year);
        }
    }
}
#if LVGL_VERSION_MAJOR == 8
static void draw_part_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    /*If the cells are drawn...*/
    if(dsc->part == LV_PART_ITEMS) {
        uint32_t row = dsc->id /  lv_table_get_col_cnt(obj);
        uint32_t col = dsc->id - row * lv_table_get_col_cnt(obj);

        /*Make the texts in the first cell center aligned*/
        if(row == 0) {
            dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;
            dsc->rect_dsc->bg_color = lv_color_mix(lv_theme_get_color_primary(NULL), dsc->rect_dsc->bg_color, LV_OPA_20);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        } else if(col == 0) { /*In the first column align the texts to the right*/
            dsc->label_dsc->align = LV_TEXT_ALIGN_RIGHT;
        }

        /*MAke every 2nd row grayish*/
        if((row != 0 && row % 2) == 0) {
            dsc->rect_dsc->bg_color = lv_color_mix(lv_theme_get_color_secondary(NULL), dsc->rect_dsc->bg_color, LV_OPA_10);
            dsc->rect_dsc->bg_opa = LV_OPA_COVER;
        }
    }
}
#else
static void draw_part_event_cb(lv_event_t * e)
{
    lv_draw_task_t * draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t * base_dsc = draw_task->draw_dsc;
    /*If the cells are drawn...*/
    if(base_dsc->part == LV_PART_ITEMS) {
        uint32_t row = base_dsc->id1;
        uint32_t col = base_dsc->id2;

        /*Make the texts in the first cell center aligned*/
        if(row == 0) {
            lv_draw_label_dsc_t * label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
            if(label_draw_dsc) {
                label_draw_dsc->align = LV_TEXT_ALIGN_CENTER;
            }
            lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
            if(fill_draw_dsc) {
                fill_draw_dsc->color = lv_color_mix(lv_theme_get_color_primary(NULL), fill_draw_dsc->color, LV_OPA_20);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }
        }
        /*In the first column align the texts to the right*/
        else if(col == 0) {
            lv_draw_label_dsc_t * label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
            if(label_draw_dsc) {
                label_draw_dsc->align = LV_TEXT_ALIGN_RIGHT;
            }
        }

        /*Make every 2nd row grayish*/
        if((row != 0 && row % 2) == 0) {
            lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
            if(fill_draw_dsc) {
                fill_draw_dsc->color = lv_color_mix(lv_theme_get_color_secondary(NULL), fill_draw_dsc->color, LV_OPA_10);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }
        }
    }
}
#endif

static void select_disp_obj(lv_obj_t *obj)
{
    lv_obj_add_flag(widget_contain_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(complex_menu_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(keyboard_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(calendar_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(table_ui, LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
