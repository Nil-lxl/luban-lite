/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include "lv_aic_canvas.h"

#include "lv_assert.h"
#include "lv_math.h"
#include "lv_draw.h"
#include "lv_refr.h"
#include "lv_display.h"
#include "lv_draw_sw.h"
#include "lv_string.h"
#include "lv_cache.h"

void lv_aic_canvas_init_layer(lv_obj_t *obj, lv_layer_t *layer);

void lv_aic_canvas_finish_layer(lv_obj_t *canvas, lv_layer_t *layer);

#define MY_CLASS (&lv_aic_canvas_class)

static void lv_aic_canvas_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_aic_canvas_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

const lv_obj_class_t lv_aic_canvas_class = {
    .constructor_cb = lv_aic_canvas_constructor,
    .destructor_cb = lv_aic_canvas_destructor,
    .instance_size = sizeof(lv_aic_canvas_t),
    .base_class = &lv_image_class,
    .name = "aic_canvas",
};

lv_obj_t * lv_aic_canvas_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_res_t lv_aic_canvas_alloc_buffer(lv_obj_t *obj, lv_coord_t w, lv_coord_t h)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_color_format_t cf = LV_COLOR_FORMAT_ARGB8888;
    enum mpp_pixel_format fmt = MPP_FMT_ARGB_8888;
    lv_aic_canvas_t *canvas = (lv_aic_canvas_t *)obj;

    canvas->img_buf= lv_mpp_image_alloc(w, h, fmt);
    if (!canvas->img_buf) {
        LV_LOG_ERROR("lv_mpp_image_alloc failed");
        return LV_RES_INV;
    }

    uint32_t stride = canvas->img_buf->buf.stride[0];
    void *data = (void *)(ulong)canvas->img_buf->buf.phy_addr[0];
    lv_draw_buf_init(&canvas->static_buf, w, h, cf, stride, data, stride * h);
    canvas->draw_buf = &canvas->static_buf;

    const void * src = lv_image_get_src(obj);
    if(src) {
        lv_image_cache_drop(src);
    }

    lv_image_set_src(obj, canvas->draw_buf);
    lv_image_cache_drop(canvas->draw_buf);
    return LV_RES_OK;
}

void lv_aic_canvas_init_layer(lv_obj_t *obj, lv_layer_t *layer)
{
    LV_ASSERT_NULL(obj);
    LV_ASSERT_NULL(layer);
    lv_aic_canvas_t * canvas = (lv_aic_canvas_t *)obj;
    if(canvas->draw_buf == NULL) return;

    lv_image_header_t * header = &canvas->draw_buf->header;
    lv_area_t canvas_area = {0, 0, header->w - 1,  header->h - 1};
    lv_memzero(layer, sizeof(*layer));

    layer->draw_buf = canvas->draw_buf;
    layer->color_format = header->cf;
    layer->buf_area = canvas_area;
    layer->_clip_area = canvas_area;
}

void lv_aic_canvas_finish_layer(lv_obj_t *canvas, lv_layer_t *layer)
{
    while(layer->draw_task_head) {
        lv_draw_dispatch_wait_for_request();
        lv_draw_dispatch_layer(lv_obj_get_display(canvas), layer);
    }
}

void lv_aic_canvas_draw_text(lv_obj_t *obj, lv_coord_t x, lv_coord_t y, lv_coord_t max_w,
                             lv_draw_label_dsc_t *draw_dsc, const char *txt)
{
    lv_aic_canvas_t *canvas = (lv_aic_canvas_t *)obj;
    lv_layer_t layer;
    lv_aic_canvas_init_layer(obj, &layer);

    lv_area_t coords;
    coords.x1 = x;
    coords.y1 = y;
    coords.x2 = x + max_w - 1;
    coords.y2 = canvas->draw_buf->header.h - 1;

    draw_dsc->text = txt;
    lv_draw_label(&layer, draw_dsc, &coords);
    lv_aic_canvas_finish_layer(obj, &layer);
}

void lv_aic_canvas_draw_text_to_center(lv_obj_t *obj, lv_draw_label_dsc_t *label_dsc, const char *txt)
{
    lv_aic_canvas_t *canvas = (lv_aic_canvas_t *)obj;

    if (canvas->img_buf) {
        lv_ge_fill(&canvas->img_buf->buf, GE_NO_GRADIENT, 0x00000000, 0x00000000, 0);

        lv_point_t txt_size;
        lv_txt_get_size(&txt_size, txt, label_dsc->font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        lv_coord_t x = (canvas->draw_buf->header.w - txt_size.x) / 2;
        lv_coord_t y = (canvas->draw_buf->header.h  - txt_size.y) / 2;

        lv_aic_canvas_draw_text(obj, x, y, canvas->draw_buf->header.w, label_dsc, txt);
        lv_mpp_image_flush_cache(canvas->img_buf);
        lv_obj_invalidate(obj);
    }
}

static void lv_aic_canvas_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_UNUSED(obj);
}

static void lv_aic_canvas_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_aic_canvas_t *canvas = (lv_aic_canvas_t *)obj;
    if(canvas->draw_buf == NULL) return;

    lv_image_cache_drop(&canvas->draw_buf);
    if (canvas->img_buf)
        lv_mpp_image_free(canvas->img_buf);
}
