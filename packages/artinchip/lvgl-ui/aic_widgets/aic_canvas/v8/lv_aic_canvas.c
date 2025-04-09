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
#include "lv_draw_sw.h"

#define USE_HW_IMG_BLIT

#define MY_CLASS &lv_aic_canvas_class

static void lv_aic_canvas_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_aic_canvas_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void init_fake_disp(lv_obj_t *canvas, lv_disp_t *disp, lv_disp_drv_t *drv, lv_area_t *clip_area);
static void deinit_fake_disp(lv_obj_t *canvas, lv_disp_t *disp);

const lv_obj_class_t lv_aic_canvas_class = {
    .constructor_cb = lv_aic_canvas_constructor,
    .destructor_cb = lv_aic_canvas_destructor,
    .instance_size = sizeof(lv_aic_canvas_t),
    .base_class = &lv_img_class
};

lv_obj_t *lv_aic_canvas_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_res_t lv_aic_canvas_alloc_buffer(lv_obj_t *obj, lv_coord_t w, lv_coord_t h)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    // only support argb8888 currently
    enum mpp_pixel_format fmt = MPP_FMT_ARGB_8888;

    lv_aic_canvas_t *canvas = (lv_aic_canvas_t *)obj;
    canvas->img_buf= lv_mpp_image_alloc(w, h, fmt);
    if (!canvas->img_buf) {
        LV_LOG_ERROR("lv_mpp_image_alloc failed");
        return LV_RES_INV;
    }

#ifdef USE_HW_IMG_BLIT
    canvas->dsc.header.cf = LV_IMG_CF_RESERVED_16;
    canvas->dsc.header.w  = w;
    canvas->dsc.header.h  = h;
    canvas->dsc.data      = (uint8_t *)&canvas->img_buf->buf;
#else
    lv_img_cf_t cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    canvas->dsc.header.cf = cf;
    canvas->dsc.header.w  = w;
    canvas->dsc.header.h  = h;
    canvas->dsc.data      = (void *)((unsigned long)(canvas->img_buf->buf.phy_addr[0]));
#endif
    lv_img_set_src(obj, &canvas->dsc);
    lv_img_cache_invalidate_src(&canvas->dsc);

    return LV_RES_OK;
}

void lv_aic_canvas_draw_text(lv_obj_t *obj, lv_coord_t x, lv_coord_t y, lv_coord_t max_w,
                             lv_draw_label_dsc_t * draw_dsc, const char * txt)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_aic_canvas_t *canvas = (lv_aic_canvas_t *)obj;
    lv_img_dsc_t *dsc = &canvas->dsc;

    if(dsc->header.cf >= LV_IMG_CF_INDEXED_1BIT && dsc->header.cf <= LV_IMG_CF_INDEXED_8BIT) {
        LV_LOG_WARN("lv_canvas_draw_text: can't draw to LV_IMG_CF_INDEXED canvas");
        return;
    }

    // Create a dummy display to fool the lv_draw function, It will think it draws to real screen
    lv_disp_t fake_disp;
    lv_disp_drv_t driver;
    lv_area_t clip_area;
    init_fake_disp(obj, &fake_disp, &driver, &clip_area);

    lv_disp_t * refr_ori = _lv_refr_get_disp_refreshing();
    _lv_refr_set_disp_refreshing(&fake_disp);

    lv_area_t coords;
    coords.x1 = x;
    coords.y1 = y;
    coords.x2 = x + max_w - 1;
    coords.y2 = dsc->header.h - 1;
    lv_draw_label(driver.draw_ctx, draw_dsc, &coords, txt, NULL);

    _lv_refr_set_disp_refreshing(refr_ori);

    deinit_fake_disp(obj, &fake_disp);

    if (dsc->header.cf == LV_IMG_CF_RESERVED_16) {
        lv_mpp_image_flush_cache(canvas->img_buf);
    }

    lv_obj_invalidate(obj);
}

void lv_aic_canvas_draw_text_to_center(lv_obj_t *obj, lv_draw_label_dsc_t *label_dsc, const char *txt)
{
    lv_aic_canvas_t *canvas = (lv_aic_canvas_t *)obj;
    lv_ge_fill(&canvas->img_buf->buf, GE_NO_GRADIENT, 0x00000000, 0x00000000, 0);

    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, txt, label_dsc->font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

    lv_coord_t x = (canvas->dsc.header.w - txt_size.x) / 2;
    lv_coord_t y = (canvas->dsc.header.h  - txt_size.y) / 2;

    lv_aic_canvas_draw_text(obj, x, y, canvas->dsc.header.w, label_dsc, txt);
    lv_mpp_image_flush_cache(canvas->img_buf);
    lv_obj_invalidate(obj);

    return;
}

static void lv_aic_canvas_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_aic_canvas_t * canvas = (lv_aic_canvas_t *)obj;

    canvas->dsc.header.always_zero = 0;
    canvas->dsc.header.cf          = LV_IMG_CF_TRUE_COLOR;
    canvas->dsc.header.h           = 0;
    canvas->dsc.header.w           = 0;
    canvas->dsc.data_size          = 0;
    canvas->dsc.data               = NULL;

    lv_img_set_src(obj, &canvas->dsc);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_aic_canvas_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_aic_canvas_t * canvas = (lv_aic_canvas_t *)obj;
    lv_img_cache_invalidate_src(&canvas->dsc);
    if (canvas->img_buf)
        lv_mpp_image_free(canvas->img_buf);
}

static inline lv_color32_t lv_pixel_mix(lv_color32_t c1, lv_color32_t c2, uint8_t mix)
{
    lv_color32_t ret;

    ret.ch.red = LV_UDIV255(c1.ch.red *mix + c2.ch.red * (255 - mix) + LV_COLOR_MIX_ROUND_OFS);
    ret.ch.green = LV_UDIV255(c1.ch.green *mix + c2.ch.green * (255 - mix) + LV_COLOR_MIX_ROUND_OFS);
    ret.ch.blue = LV_UDIV255(c1.ch.blue *mix + c2.ch.blue * (255 - mix) + LV_COLOR_MIX_ROUND_OFS);
    ret.ch.alpha = 0xff;
    return ret;
}

static inline void lv_pixel_mix_with_alpha(lv_color32_t bg_color, lv_opa_t bg_opa,
                                           lv_color32_t fg_color, lv_opa_t fg_opa,
                                           lv_color32_t *res_color, lv_opa_t *res_opa)
{
    if(fg_opa >= LV_OPA_MAX || bg_opa <= LV_OPA_MIN) {
        res_color->full = fg_color.full;
        *res_opa = fg_opa;
    } else if(fg_opa <= LV_OPA_MIN) {
        res_color->full = bg_color.full;
        *res_opa = bg_opa;
    } else if(bg_opa >= LV_OPA_MAX) {
        *res_color = lv_pixel_mix(fg_color, bg_color, fg_opa);
        *res_opa = LV_OPA_COVER;
    } else {
        static lv_opa_t fg_opa_save     = 0;
        static lv_opa_t bg_opa_save     = 0;
        static lv_color32_t fg_color_save = _LV_COLOR_ZERO_INITIALIZER32;
        static lv_color32_t bg_color_save = _LV_COLOR_ZERO_INITIALIZER32;
        static lv_color32_t res_color_saved = _LV_COLOR_ZERO_INITIALIZER32;
        static lv_opa_t res_opa_saved = 0;

        if(fg_opa != fg_opa_save || bg_opa != bg_opa_save || fg_color.full != fg_color_save.full ||
           bg_color.full != bg_color_save.full) {
            fg_opa_save        = fg_opa;
            bg_opa_save        = bg_opa;
            fg_color_save.full = fg_color.full;
            bg_color_save.full = bg_color.full;
            res_opa_saved = 255 - ((uint16_t)((uint16_t)(255 - fg_opa) * (255 - bg_opa)) >> 8);
            LV_ASSERT(res_opa_saved != 0);
            lv_opa_t ratio = (uint16_t)((uint16_t)fg_opa * 255) / res_opa_saved;
            res_color_saved = lv_pixel_mix(fg_color, bg_color, ratio);

        }

        res_color->full = res_color_saved.full;
        *res_opa = res_opa_saved;
    }
}

static inline void rgb565_to_argb(uint16_t src_pixel, uint32_t* dst_color)
{
    unsigned int a, r, g, b;

    a = 0xff;
    r = ((src_pixel & 0xf800) >> 8);
    g = (src_pixel & 0x07e0) >> 3;
    b = (src_pixel & 0x1f) << 3;

    *dst_color = (a << 24) | ((r | (r >> 5)) << 16) |
        ((g | (g >> 6)) << 8) | (b | (b >> 5));

}

static void set_pixel_true_color_alpha(lv_disp_drv_t * disp_drv, uint8_t * buf, lv_coord_t buf_w,
                                       lv_coord_t x, lv_coord_t y,
                                       lv_color_t color, lv_opa_t opa)
{
    (void) disp_drv; /*Unused*/

    uint8_t *buf_px = buf + (buf_w * y * 4 + x * 4);
    lv_color32_t bg_color;
    lv_color32_t fg_color;
    lv_color32_t res_color;
    lv_opa_t bg_opa = buf_px[3];

#if LV_COLOR_DEPTH == 16
    rgb565_to_argb((uint16_t)color.full, &fg_color.full);
#else
    fg_color.full = color.full;
#endif

    bg_color = *((lv_color32_t *)buf_px);
    lv_pixel_mix_with_alpha(bg_color, bg_opa, fg_color, opa, &res_color, &buf_px[3]);
    if(buf_px[3] <= LV_OPA_MIN) return;
    buf_px[0] = res_color.ch.blue;
    buf_px[1] = res_color.ch.green;
    buf_px[2] = res_color.ch.red;
}

static void lv_disp_drv_set_px_cb(lv_disp_drv_t * disp_drv, lv_img_cf_t cf)
{
    switch(cf) {
        case LV_IMG_CF_TRUE_COLOR_ALPHA:
            disp_drv->set_px_cb = set_pixel_true_color_alpha;
            break;
        default:
            LV_LOG_ERROR("unknown cf:%d", cf);
            disp_drv->set_px_cb = NULL;
    }
}

static void init_fake_disp(lv_obj_t *canvas, lv_disp_t *disp, lv_disp_drv_t *drv, lv_area_t *clip_area)
{
    lv_aic_canvas_t *aic_canvas = (lv_aic_canvas_t *)canvas;
    lv_img_dsc_t * dsc = &aic_canvas->dsc;
    lv_img_cf_t cf = LV_IMG_CF_TRUE_COLOR_ALPHA;

    clip_area->x1 = 0;
    clip_area->x2 = dsc->header.w - 1;
    clip_area->y1 = 0;
    clip_area->y2 = dsc->header.h - 1;

    lv_memset_00(disp, sizeof(lv_disp_t));
    disp->driver = drv;

    lv_disp_drv_init(disp->driver);
    disp->driver->hor_res = dsc->header.w;
    disp->driver->ver_res = dsc->header.h;

    lv_draw_ctx_t * draw_ctx = lv_mem_alloc(sizeof(lv_draw_sw_ctx_t));
    LV_ASSERT_MALLOC(draw_ctx);
    if(draw_ctx == NULL)  return;
    lv_draw_sw_init_ctx(drv, draw_ctx);
    disp->driver->draw_ctx = draw_ctx;
    draw_ctx->clip_area = clip_area;
    draw_ctx->buf_area = clip_area;

    if (aic_canvas->img_buf) {
        if (dsc->header.cf == LV_IMG_CF_RESERVED_16)
            draw_ctx->buf = (void *)((unsigned long)(aic_canvas->img_buf->buf.phy_addr[0]));
        else
            draw_ctx->buf = (void *)dsc->data;
    } else {
        return;
    }

    cf = aic_canvas->img.cf;
    lv_disp_drv_set_px_cb(disp->driver, cf);
}

static void deinit_fake_disp(lv_obj_t *canvas, lv_disp_t *disp)
{
    LV_UNUSED(canvas);
    lv_draw_sw_deinit_ctx(disp->driver, disp->driver->draw_ctx);
    lv_mem_free(disp->driver->draw_ctx);
}
