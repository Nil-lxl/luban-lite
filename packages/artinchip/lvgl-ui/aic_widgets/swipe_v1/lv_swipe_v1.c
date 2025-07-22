/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ZeQuan Liang <zequan.liang@artinchip.com>
 */

#include "lv_swipe_v1.h"
#if LV_SWIPE_V1
#if LVGL_VERSION_MAJOR == 8
#define GET_IMG_WIDTH(x)  ((lv_img_t *)(x))->w
#define GET_IMG_HEIGHT(x)  ((lv_img_t *)(x))->h
#define GET_STYLE_TRANSFORM lv_obj_get_style_transform_zoom
#define OBJ_SEND_EVENT lv_event_send
#else
#define GET_IMG_WIDTH(x)  ((lv_image_t *)(x))->w
#define GET_IMG_HEIGHT(x)  ((lv_image_t *)(x))->h
#define GET_STYLE_TRANSFORM lv_obj_get_style_transform_scale_x
#define OBJ_SEND_EVENT lv_obj_send_event
#endif

#define MAX_U32 0xFFFFFFFF
#define MY_CLASS (&lv_swipe_v1_class)

struct lv_swipe_v1_child {
    lv_obj_t *obj;
    uint8_t index;
    uint8_t x;
    uint8_t y;
    void **active;
    void **deactivate;
    uint8_t count;
    uint8_t current;
    void *user_data;
};

typedef struct {
    lv_obj_t *obj;
    uint8_t index;
    int32_t start_x;
    int32_t end_x;
    int32_t start_y;
    int32_t end_y;
    int32_t start_scale;
    int32_t end_scale;
    bool has_scale;
    bool set_state;
    bool state;
} child_anim_params;

static struct lv_swipe_v1_child *get_child(lv_obj_t *obj, uint16_t index);
static void on_child_click(lv_event_t *e);
static void apply_child_animation(lv_swipe_v1_t *swipe_v1, child_anim_params *params,
                                    lv_anim_enable_t enable);
static void update_draw_order(lv_obj_t *obj);
static void send_anim_event(lv_obj_t *obj, uint32_t duration);
static void set_child_state(struct lv_swipe_v1_child *child, uint32_t duration, bool show);
static void move_x_anim(lv_obj_t *obj, uint32_t duration, lv_anim_path_cb_t path, int32_t start, int32_t end);
static void move_y_anim(lv_obj_t *obj, uint32_t duration, lv_anim_path_cb_t path, int32_t start, int32_t end);
static void scale_anim(lv_obj_t *obj, uint32_t duration, lv_anim_path_cb_t path, int32_t start, int32_t end);
static void move_node_head_to_tail(lv_obj_t *obj);
static void move_node_tail_to_head(lv_obj_t *obj);
static void lv_swipe_v1_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_swipe_v1_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

const lv_obj_class_t lv_swipe_v1_class = {
    .constructor_cb = lv_swipe_v1_constructor,
    .destructor_cb = lv_swipe_v1_destructor,
    .width_def = LV_PCT(100),
    .height_def = LV_PCT(100),
    .base_class = &lv_obj_class,
    .instance_size = sizeof(lv_swipe_v1_t),
#if LVGL_VERSION_MAJOR > 8
    .name = "swipe_v1",
#endif
};

lv_obj_t *lv_swipe_v1_create(lv_obj_t *parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_swipe_v1_child_add_state_src(lv_obj_t *obj, uint16_t index, void *active, void *deactivate)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)obj;
    struct lv_swipe_v1_child *child = get_child(obj, index);

    if (child == NULL) {
        child = _lv_ll_ins_tail(&swipe_v1->child_ll);
        memset(child, 0, sizeof(struct lv_swipe_v1_child));

        lv_obj_t *image = lv_img_create(obj);
        lv_img_set_src(image, active);
        lv_obj_add_flag(image, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(image, on_child_click, LV_EVENT_CLICKED, NULL);

        child->obj = image;
        child->index = swipe_v1->child_count;
        swipe_v1->child_count++;
    }

    child->count++;
    child->active = lv_mem_realloc(child->active, sizeof(void *) * child->count);
    child->deactivate = lv_mem_realloc(child->deactivate, sizeof(void *) * child->count);
    child->active[child->count - 1] = active;
    child->deactivate[child->count - 1] = deactivate;

    lv_obj_update_layout(child->obj);
}

void lv_swipe_v1_set_next(lv_obj_t *obj, lv_anim_enable_t enable)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)obj;

    if (swipe_v1->child_count != 4) {
        LV_LOG_ERROR("child count is != 4, %d", swipe_v1->child_count);
        return;
    }
    if (swipe_v1->switching) {
        LV_LOG_WARN("switching");
        return;
    }

    struct lv_swipe_v1_child *children[4], *child;
    uint8_t count = 0;
    _LV_LL_READ(&swipe_v1->child_ll, child) {
        children[count] = child;
        count++;
    }

    // Temporary values for the last child
    int32_t temp_move_x = lv_obj_get_x_aligned(children[0]->obj);
    int32_t temp_move_y = lv_obj_get_y_aligned(children[0]->obj);
    int32_t temp_scale = GET_STYLE_TRANSFORM(children[0]->obj, 0);

    // Configure animation parameters for each child
    child_anim_params params[4] = {
        // Child 0
        {
            .obj = children[0]->obj,
            .index = children[0]->index,
            .start_x = lv_obj_get_x_aligned(children[0]->obj),
            .end_x = lv_obj_get_x_aligned(children[1]->obj),
            .start_y = lv_obj_get_y_aligned(children[0]->obj),
            .end_y = lv_obj_get_y_aligned(children[1]->obj),
            .has_scale = false,
            .set_state = false
        },
        // Child 1
        {
            .obj = children[1]->obj,
            .index = children[1]->index,
            .start_x = lv_obj_get_x_aligned(children[1]->obj),
            .end_x = lv_obj_get_x_aligned(children[2]->obj),
            .start_y = lv_obj_get_y_aligned(children[1]->obj),
            .end_y = lv_obj_get_y_aligned(children[2]->obj),
            .has_scale = false,
            .set_state = false
        },
        // Child 2
        {
            .obj = children[2]->obj,
            .index = children[2]->index,
            .start_x = lv_obj_get_x_aligned(children[2]->obj),
            .end_x = lv_obj_get_x_aligned(children[3]->obj),
            .start_y = lv_obj_get_y_aligned(children[2]->obj),
            .end_y = lv_obj_get_y_aligned(children[3]->obj),
            .start_scale = GET_STYLE_TRANSFORM(children[2]->obj, 0),
            .end_scale = GET_STYLE_TRANSFORM(children[3]->obj, 0),
            .has_scale = true,
            .set_state = true,
            .state = true
        },
        // Child 3
        {
            .obj = children[3]->obj,
            .index = children[3]->index,
            .start_x = lv_obj_get_x_aligned(children[3]->obj),
            .end_x = temp_move_x,
            .start_y = lv_obj_get_y_aligned(children[3]->obj),
            .end_y = temp_move_y,
            .start_scale = GET_STYLE_TRANSFORM(children[3]->obj, 0),
            .end_scale = temp_scale,
            .has_scale = true,
            .set_state = true,
            .state = false
        }
    };

    // Apply animations
    for (int i = 0; i < 4; i++) {
        apply_child_animation(swipe_v1, &params[i], enable);
    }

    send_anim_event(obj, swipe_v1->anim_time);
    move_node_tail_to_head(obj);
    update_draw_order(obj);
}

void lv_swipe_v1_set_prev(lv_obj_t *obj, lv_anim_enable_t enable)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)obj;

    if (swipe_v1->child_count != 4) {
        LV_LOG_ERROR("child count is != 4, %d", swipe_v1->child_count);
        return;
    }
    if (swipe_v1->switching) {
        LV_LOG_WARN("switching");
        return;
    }

    struct lv_swipe_v1_child *children[4], *child;
    uint8_t count = 0;
    _LV_LL_READ(&swipe_v1->child_ll, child) {
        children[count] = child;
        count++;
    }

    // Temporary values for the first child
    int32_t temp_move_x = lv_obj_get_x_aligned(children[2]->obj);
    int32_t temp_move_y = lv_obj_get_y_aligned(children[2]->obj);
    int32_t temp_scale = GET_STYLE_TRANSFORM(children[2]->obj, 0);

    // Configure animation parameters for each child
    child_anim_params params[4] = {
        // Child 2
        {
            .obj = children[2]->obj,
            .index = children[2]->index,
            .start_x = lv_obj_get_x_aligned(children[2]->obj),
            .end_x = lv_obj_get_x_aligned(children[1]->obj),
            .start_y = lv_obj_get_y_aligned(children[2]->obj),
            .end_y = lv_obj_get_y_aligned(children[1]->obj),
            .has_scale = false,
            .set_state = false
        },
        // Child 1
        {
            .obj = children[1]->obj,
            .index = children[1]->index,
            .start_x = lv_obj_get_x_aligned(children[1]->obj),
            .end_x = lv_obj_get_x_aligned(children[0]->obj),
            .start_y = lv_obj_get_y_aligned(children[1]->obj),
            .end_y = lv_obj_get_y_aligned(children[0]->obj),
            .has_scale = false,
            .set_state = false
        },
        // Child 0
        {
            .obj = children[0]->obj,
            .index = children[0]->index,
            .start_x = lv_obj_get_x_aligned(children[0]->obj),
            .end_x = lv_obj_get_x_aligned(children[3]->obj),
            .start_y = lv_obj_get_y_aligned(children[0]->obj),
            .end_y = lv_obj_get_y_aligned(children[3]->obj),
            .start_scale = GET_STYLE_TRANSFORM(children[0]->obj, 0),
            .end_scale = GET_STYLE_TRANSFORM(children[3]->obj, 0),
            .has_scale = true,
            .set_state = true,
            .state = true
        },
        // Child 3
        {
            .obj = children[3]->obj,
            .index = children[3]->index,
            .start_x = lv_obj_get_x_aligned(children[3]->obj),
            .end_x = temp_move_x,
            .start_y = lv_obj_get_y_aligned(children[3]->obj),
            .end_y = temp_move_y,
            .start_scale = GET_STYLE_TRANSFORM(children[3]->obj, 0),
            .end_scale = temp_scale,
            .has_scale = true,
            .set_state = true,
            .state = false
        }
    };

    // Apply animations
    for (int i = 0; i < 4; i++) {
        apply_child_animation(swipe_v1, &params[i], enable);
    }

    send_anim_event(obj, swipe_v1->anim_time);
    move_node_head_to_tail(obj);
    update_draw_order(obj);
}

void lv_swipe_v1_set_anim_params(lv_obj_t *obj, uint16_t anim_time, lv_anim_path_cb_t path)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)obj;
    swipe_v1->anim_time = anim_time;
    swipe_v1->path = path;
}

void lv_swipe_v1_set_child_activate(lv_obj_t *obj, uint16_t index, uint16_t src_index)
{
    struct lv_swipe_v1_child *child = get_child(obj, index);
    if (child == NULL || src_index > child->count)
        return;

    child->current = src_index;
    lv_img_set_src(child->obj, child->active[src_index]);
}

void lv_swipe_v1_set_child_deactive_src(lv_obj_t *obj, uint16_t index, uint16_t src_index)
{
    struct lv_swipe_v1_child *child = get_child(obj, index);
    if (child == NULL || src_index > child->count)
        return;

    child->current = src_index;
    lv_img_set_src(child->obj, child->deactivate[src_index]);
}

void lv_swipe_v1_toggle_child_active(lv_obj_t *obj)
{
    int index = lv_swipe_v1_get_active_child(obj);
    struct lv_swipe_v1_child *child = get_child(obj, index);
    if (child == NULL)
        return;

    OBJ_SEND_EVENT(child->obj, LV_EVENT_CLICKED, child->obj);
}

int16_t lv_swipe_v1_get_active_child(lv_obj_t *obj)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)obj;
    if (swipe_v1->child_count <= 0)
        return -1;

    struct lv_swipe_v1_child *child = _lv_ll_get_tail(&swipe_v1->child_ll);

    return child->index;
}

uint16_t lv_swipe_v1_get_child_count(lv_obj_t *obj)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)obj;
    return swipe_v1->child_count;
}

int16_t lv_swipe_v1_get_child_active_src(lv_obj_t *obj, uint16_t index)
{
    struct lv_swipe_v1_child *child = get_child(obj, index);
    if (child == NULL)
        return -1;
    return child->current;
}

lv_obj_t *lv_swipe_v1_get_child(lv_obj_t *obj, uint16_t index)
{
    struct lv_swipe_v1_child *child = get_child(obj, index);
    if (child == NULL)
        return NULL;
    return child->obj;
}

static void on_child_click(lv_event_t *e)
{
    lv_obj_t *image = lv_event_get_current_target(e);
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)lv_obj_get_parent(image);

    struct lv_swipe_v1_child *child = NULL, *head = NULL, *tail_before = NULL;
    _LV_LL_READ(&swipe_v1->child_ll, child) {
        if (child->obj == image) {
            break;
        }
    }

    if (child == NULL || swipe_v1 ->child_count < 4)
        return;

    head = _lv_ll_get_head(&swipe_v1->child_ll);
    tail_before = _lv_ll_get_prev(&swipe_v1->child_ll,
                                _lv_ll_get_tail(&swipe_v1->child_ll));

    if (head->index == child->index) {
        lv_swipe_v1_set_prev((lv_obj_t *)swipe_v1, LV_ANIM_ON);
        return;
    } else if (tail_before->index == child->index) {
        lv_swipe_v1_set_next((lv_obj_t *)swipe_v1, LV_ANIM_ON);
        return;
    }

    child->current++;
    child->current = (child->current >= child->count) ? 0 : child->current;
    lv_img_set_src(child->obj, child->active[child->current]);
}

static void apply_child_animation(lv_swipe_v1_t *swipe_v1, child_anim_params *params,
                                    lv_anim_enable_t enable)
{
    if (enable) {
        if (params->start_x != params->end_x || params->start_y != params->end_y) {
            move_x_anim(params->obj, swipe_v1->anim_time, swipe_v1->path, params->start_x, params->end_x);
            move_y_anim(params->obj, swipe_v1->anim_time, swipe_v1->path, params->start_y, params->end_y);
        }
        if (params->has_scale) {
            scale_anim(params->obj, swipe_v1->anim_time, swipe_v1->path, params->start_scale, params->end_scale);
        }
    } else {
        lv_obj_set_x(params->obj, params->end_x);
        lv_obj_set_y(params->obj, params->end_y);
        if (params->has_scale) {
            lv_obj_set_style_transform_zoom(params->obj, params->end_scale, 0);
        }
    }

    if (params->set_state) {
        struct lv_swipe_v1_child *child = get_child((lv_obj_t *)swipe_v1, params->index);
        set_child_state(child, swipe_v1->anim_time, params->state);
    }
}

static struct lv_swipe_v1_child *get_child(lv_obj_t *obj, uint16_t index)
{
    lv_swipe_v1_t * swipe_v1 = (lv_swipe_v1_t *)obj;
    if (swipe_v1->child_count == 0)
        return NULL;

    struct lv_swipe_v1_child *child;
    _LV_LL_READ(&swipe_v1->child_ll, child) {
        if (child->index == index)
            return child;
    }
    return NULL;
}

static void move_node_head_to_tail(lv_obj_t *obj)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)obj;

    struct lv_swipe_v1_child *head, *node;

    head = _lv_ll_get_head(&swipe_v1->child_ll);
    _lv_ll_remove(&swipe_v1->child_ll, head);
    node = _lv_ll_ins_tail(&swipe_v1->child_ll);
    memcpy(node, head, sizeof(struct lv_swipe_v1_child));

    lv_mem_free(head);
}

static void move_node_tail_to_head(lv_obj_t *obj)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)obj;

    struct lv_swipe_v1_child *tail, *node;

    tail = _lv_ll_get_tail(&swipe_v1->child_ll);
    _lv_ll_remove(&swipe_v1->child_ll, tail);
    node = _lv_ll_ins_head(&swipe_v1->child_ll);
    memcpy(node, tail, sizeof(struct lv_swipe_v1_child));

    lv_mem_free(tail);
}

static void anim_empty_cb(void *obj, int32_t var)
{
    (void)obj;
    (void)var;
}

static void anim_start_event(lv_anim_t *anim)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)anim->var;
    swipe_v1->switching = true;
    OBJ_SEND_EVENT(anim->var, LV_EVENT_SCROLL_BEGIN, anim->var);
}

static void anim_end_event(lv_anim_t *anim)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)anim->var;
    swipe_v1->switching = false;
    OBJ_SEND_EVENT(anim->var, LV_EVENT_SCROLL_END, anim->var);
    OBJ_SEND_EVENT(anim->var, LV_EVENT_VALUE_CHANGED, anim->var);
}

static void scale_anim_cb(void *obj, int32_t var)
{
    lv_obj_set_style_transform_zoom(obj, var, 0);
}

static void create_anim(lv_anim_t *anim, void *var,
    lv_anim_exec_xcb_t exec_cb,
    uint32_t duration,
    lv_anim_path_cb_t path,
    int32_t start, int32_t end,
    lv_anim_start_cb_t start_cb,
    lv_anim_deleted_cb_t deleted_cb)
{
    lv_anim_init(anim);
    lv_anim_set_var(anim, var);
    lv_anim_set_values(anim, start, end);
    lv_anim_set_time(anim, duration);
    lv_anim_set_exec_cb(anim, exec_cb);
    lv_anim_set_path_cb(anim, path);
    if (start_cb) lv_anim_set_start_cb(anim, start_cb);
    if (deleted_cb) lv_anim_set_deleted_cb(anim, deleted_cb);
    lv_anim_set_user_data(anim, var);
    lv_anim_start(anim);
}

static void send_anim_event(lv_obj_t *obj, uint32_t duration)
{
    lv_anim_t anim;
    create_anim(&anim, obj, anim_empty_cb,
    duration, lv_anim_path_linear, 0, 100,
    anim_start_event,
    anim_end_event);
}

static void set_child_state(struct lv_swipe_v1_child *child,
                                            uint32_t duration, bool show)
{
    void *src = show ? child->active[child->current] : child->deactivate[child->current];

    if (src != NULL) {
        lv_img_set_src(child->obj, src);
    }
}

static void move_x_anim(lv_obj_t *obj, uint32_t duration,
                                     lv_anim_path_cb_t path, int32_t start, int32_t end)
{
    lv_anim_t anim;
    create_anim(&anim, obj, (lv_anim_exec_xcb_t)lv_obj_set_x,
                duration, path, start, end, NULL, NULL);
}

static void move_y_anim(lv_obj_t *obj, uint32_t duration,
                                     lv_anim_path_cb_t path, int32_t start, int32_t end)
{
    lv_anim_t anim;
    create_anim(&anim, obj, (lv_anim_exec_xcb_t)lv_obj_set_y,
                duration, path, start, end, NULL, NULL);
}

static void scale_anim(lv_obj_t *obj, uint32_t duration,
                                      lv_anim_path_cb_t path, int32_t start, int32_t end)
{
    lv_anim_t anim;
    create_anim(&anim, obj, scale_anim_cb,
                duration, path, start, end, NULL, NULL);
}

static void update_draw_order(lv_obj_t *obj)
{
    lv_swipe_v1_t *swipe_v1 = (lv_swipe_v1_t *)obj;
    struct lv_swipe_v1_child *child;

    if (swipe_v1->child_count <= 0)
        return;
    int count = 0;
    _LV_LL_READ(&swipe_v1->child_ll, child) {
        lv_obj_move_to_index(child->obj, count);
        count++;
    }
}

static void lv_swipe_v1_constructor(const lv_obj_class_t * class_p,
                                         lv_obj_t * obj)
{
    lv_swipe_v1_t * swipe_v1 = (lv_swipe_v1_t *)obj;

    _lv_ll_init(&swipe_v1->child_ll, sizeof(struct lv_swipe_v1_child));

    swipe_v1->child_count = 0;
    swipe_v1->anim_time = 1000;
    swipe_v1->path = lv_anim_path_linear;
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
#if LVGL_VERSION_MAJOR == 8
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
#else
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
#endif
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void lv_swipe_v1_destructor(const lv_obj_class_t * class_p,
                                         lv_obj_t * obj)
{
    LV_TRACE_OBJ_CREATE("begin");
    lv_swipe_v1_t * swipe_v1 = (lv_swipe_v1_t *)obj;

    struct lv_swipe_v1_child *child;
    _LV_LL_READ(&swipe_v1->child_ll, child) {
        lv_mem_free(child->active);
        lv_mem_free(child->deactivate);
    }
    _lv_ll_clear(&swipe_v1->child_ll);
}
#endif
