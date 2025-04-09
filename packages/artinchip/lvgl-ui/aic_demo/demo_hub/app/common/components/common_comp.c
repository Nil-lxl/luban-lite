/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "common_comp.h"

#define MAX_FREETYPE_STYLE 32

#if LVGL_VERSION_MAJOR == 8
#define lv_img_header_t         lv_img_header_t
#else
#define lv_img_header_t         lv_image_header_t
#endif

typedef enum {
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

typedef struct com_imgbtn_switch {
    int status;
    void *img_src_0;
    void *img_src_1;
} com_imgbtn_switch_t;

#if LV_USE_FREETYPE
typedef struct com_freetype_style {
    lv_ft_info_t ft_info;
    int reference;
} com_freetype_style_t;

static com_freetype_style_t freetype_style_list[MAX_FREETYPE_STYLE];

static void freetype_style_del_and_remove_list(lv_event_t * e);
static lv_ft_info_t *freetype_style_get_and_add_list(uint16_t weight, uint16_t style, const char *path);
#endif

static void switch_imgbtn_cb(lv_event_t * e);
static void lv_mem_free_cb(lv_event_t * e);

lv_obj_t *com_imgbtn_switch_comp(lv_obj_t *parent, void *img_src_0, void *img_src_1)
{
    disp_size_t disp_size = 0;
    if(LV_HOR_RES <= 480) disp_size = DISP_SMALL;
    else if(LV_HOR_RES <= 800) disp_size = DISP_MEDIUM;
    else disp_size = DISP_LARGE;

    com_imgbtn_switch_t *com_imgbtn;

    lv_obj_t *app_icon = lv_img_create(parent);
    lv_img_set_src(app_icon, img_src_0);
    lv_obj_add_flag(app_icon, LV_OBJ_FLAG_CLICKABLE);
    if (disp_size == DISP_SMALL)
        lv_obj_set_ext_click_area(app_icon, 10);

    com_imgbtn = (com_imgbtn_switch_t *)lv_mem_alloc(sizeof(com_imgbtn_switch_t));
    lv_memset(com_imgbtn, 0, sizeof(com_imgbtn_switch_t));;
    com_imgbtn->img_src_0 = img_src_0;
    com_imgbtn->img_src_1 = img_src_1;

    lv_obj_add_event_cb(app_icon, switch_imgbtn_cb, LV_EVENT_ALL, com_imgbtn);
    lv_obj_add_event_cb(app_icon, lv_mem_free_cb, LV_EVENT_ALL, com_imgbtn);

    return app_icon;
}

lv_obj_t *com_imgbtn_comp(lv_obj_t *parent, void *img_src, void *pressed_img_src)
{
    disp_size_t disp_size = 0;
    int offset_size = 2;

    if(LV_HOR_RES <= 480) disp_size = DISP_SMALL;
    else if(LV_HOR_RES <= 800) disp_size = DISP_MEDIUM;
    else disp_size = DISP_LARGE;

    LV_UNUSED(disp_size);
    lv_obj_t *app_icon = lv_img_create(parent);
    lv_img_set_src(app_icon, img_src);
    lv_obj_add_flag(app_icon, LV_OBJ_FLAG_CLICKABLE);

    /* reduce the click area and minimize accidental touches */
    if (disp_size != DISP_SMALL)
        lv_obj_set_ext_click_area(app_icon, 10);
    else
        disp_size = 4;

    if (pressed_img_src) {
        lv_obj_set_style_bg_img_src(app_icon, pressed_img_src, LV_STATE_PRESSED | LV_PART_MAIN);
        lv_obj_set_style_bg_img_opa(app_icon, LV_OPA_100, LV_STATE_PRESSED | LV_PART_MAIN);
        lv_obj_set_style_img_opa(app_icon, LV_OPA_0, LV_STATE_PRESSED | LV_PART_MAIN);
    } else {
        lv_obj_set_style_translate_x(app_icon, offset_size, LV_STATE_PRESSED | LV_PART_MAIN);
        lv_obj_set_style_translate_y(app_icon, offset_size, LV_STATE_PRESSED | LV_PART_MAIN);
    }

    return app_icon;
}

lv_obj_t *commom_label_ui_create(lv_obj_t *parent, int font_size, int font_type)
{
    lv_obj_t *common_label = lv_label_create(parent);
    lv_obj_set_width(common_label, LV_SIZE_CONTENT);
    lv_obj_set_height(common_label, LV_SIZE_CONTENT);
    lv_obj_align(common_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(common_label, 0, 0);
    lv_label_set_text(common_label, "ft label");
    lv_obj_set_style_text_color(common_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(common_label, LV_OPA_100, 0);

    return common_label;
}

static void switch_imgbtn_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *img = lv_event_get_target(e);
    com_imgbtn_switch_t *com_imgbtn = (com_imgbtn_switch_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED)
    {
        if (com_imgbtn->status == 0) {
            com_imgbtn->status = 1;
            lv_img_set_src(img, com_imgbtn->img_src_1);
        } else {
            com_imgbtn->status = 0;
            lv_img_set_src(img, com_imgbtn->img_src_0);
        }
    }
}

static void lv_mem_free_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    void *usr_data = lv_event_get_user_data(e);

    if (code == LV_EVENT_DELETE)
    {
        lv_mem_free(usr_data);
    }
}

lv_obj_t *com_empty_imgbtn_comp(lv_obj_t *parent, void *pressed_img_src)
{
    lv_img_header_t header;
    lv_img_decoder_get_info(pressed_img_src, &header);

    lv_obj_t *app_icon = lv_obj_create(parent);
    lv_obj_remove_style_all(app_icon);
    lv_obj_set_size(app_icon, header.w, header.h);
    lv_obj_add_flag(app_icon, LV_OBJ_FLAG_CLICKABLE);

    if (pressed_img_src) {
        lv_obj_set_style_bg_img_src(app_icon, pressed_img_src, LV_STATE_PRESSED | LV_PART_MAIN);
        lv_obj_set_style_bg_img_opa(app_icon, LV_OPA_100, LV_STATE_PRESSED | LV_PART_MAIN);
    }

    return app_icon;
}

static void switch_empty_imgbtn_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *img = lv_event_get_target(e);
    int *status = (int *)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED)
    {
        if (*status == 0) {
            *status = 1;
            lv_obj_set_style_img_opa(img, LV_OPA_100, LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            *status = 0;
            lv_obj_set_style_img_opa(img, LV_OPA_0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

lv_obj_t *com_empty_imgbtn_switch_comp(lv_obj_t *parent, void *img_src)
{
    lv_img_header_t header;
    lv_img_decoder_get_info(img_src, &header);

    lv_obj_t *app_icon = lv_img_create(parent);
    lv_img_set_src(app_icon, img_src);
    lv_obj_set_size(app_icon, header.w, header.h);
    lv_obj_add_flag(app_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_img_opa(app_icon, LV_OPA_0, LV_PART_MAIN | LV_STATE_DEFAULT);

    int *img_status = (int *)lv_mem_alloc(sizeof(int));
    *img_status = 0;

    lv_obj_add_event_cb(app_icon, switch_empty_imgbtn_cb, LV_EVENT_ALL, img_status);
    lv_obj_add_event_cb(app_icon, lv_mem_free_cb, LV_EVENT_ALL, img_status);

    return app_icon;
}

#if LV_USE_FREETYPE
lv_obj_t * ft_label_comp(lv_obj_t *parent, uint16_t weight, uint16_t style, const char *path)
{
    /*Create a font*/
    lv_ft_info_t *ft_info = freetype_style_get_and_add_list(weight, style, path);

    if (ft_info == NULL)
        return NULL;

    /*Create a label with the new style*/
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_recolor(label, true);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);

    lv_obj_set_style_text_font(label, ft_info->font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(label, freetype_style_del_and_remove_list, LV_EVENT_ALL, ft_info);

    return label;
}

static void freetype_style_del_and_remove_list(lv_event_t * e)
{
    int i;
    lv_ft_info_t *ft_info = (lv_ft_info_t *)lv_event_get_user_data(e);

    if (lv_event_get_code(e) != LV_EVENT_DELETE) {
        return;
    }

    for (i = 0; i < MAX_FREETYPE_STYLE; i++) {
        if (freetype_style_list[i].ft_info.style == ft_info->style &&
            freetype_style_list[i].ft_info.weight == ft_info->weight &&
            strcmp(freetype_style_list[i].ft_info.name, ft_info->name) == 0) {
            freetype_style_list[i].reference--;
        }
    }

    for (i = 0; i < MAX_FREETYPE_STYLE; i++) {
        if (freetype_style_list[i].ft_info.name != NULL && freetype_style_list[i].reference == 0) {
            lv_ft_font_destroy(freetype_style_list[i].ft_info.font);
            lv_memset_00(&freetype_style_list[i], sizeof(com_freetype_style_t));
        }
    }
}

static lv_ft_info_t *freetype_style_get_and_add_list(uint16_t weight, uint16_t style, const char *path)
{
    int i;
    com_freetype_style_t *freetype_style;

    /* get initialized */
    for (i = 0; i < MAX_FREETYPE_STYLE; i++) {
        if (freetype_style_list[i].ft_info.style == style &&
            freetype_style_list[i].ft_info.weight == weight &&
            strcmp(freetype_style_list[i].ft_info.name, path) == 0)
            break;
    }

    if (i != MAX_FREETYPE_STYLE) {
        freetype_style = &freetype_style_list[i];
        freetype_style->reference++;
        return &freetype_style->ft_info;
    /* initialize and join list */
    } else {
        /* find a lv_mem_free location */
        for (i = 0; i < MAX_FREETYPE_STYLE; i++) {
            if (freetype_style_list[i].ft_info.name == NULL)
                break;
        }

        if (i == MAX_FREETYPE_STYLE)
            return NULL;

        freetype_style = &freetype_style_list[i];
        freetype_style->reference++;
        freetype_style->ft_info.name = path;
        freetype_style->ft_info.weight = weight;
        freetype_style->ft_info.style = style;
        freetype_style->ft_info.mem = NULL;

        if(!lv_ft_font_init(&freetype_style->ft_info)) {
            return NULL;
        }

        return &freetype_style->ft_info;
    }

    return NULL;
}
#endif
