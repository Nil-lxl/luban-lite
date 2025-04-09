/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: middle media vin component, support dvp\usb video input source
 */

#include "mm_vin_component.h"
#include <malloc.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "aic_message.h"
#include "mpp_vin_dev.h"
#include "mpp_vin.h"
#include "mpp_list.h"
#include "mpp_log.h"
#include "mpp_mem.h"


#include "mpp_vin.h"
#ifdef AIC_USING_CAMERA
#include "drv_camera.h"
#endif

struct mm_dvp_data {
    s32 w;
    s32 h;
    s32 frame_size;
    s32 frame_cnt;
    s32 rotation;
    struct mpp_video_fmt src_fmt;
    struct dvp_out_fmt out_fmt;
    uint32_t num_buffers;
    struct vin_video_buf binfo;
};

typedef struct mm_vin_frame {
    struct mpp_list list;
    struct mpp_frame frame;
    unsigned char *vir_addr[3];
    s32 size[3];
    s32 frame_index;
}mm_vin_frame;

typedef struct mm_vin_data {
    MM_STATE_TYPE state;
    pthread_mutex_t state_lock;
    mm_callback *p_callback;
    void *p_app_data;
    mm_handle h_self;
    mm_port_param port_param;

    mm_param_port_def in_port_def;
    mm_param_port_def out_port_def;

    mm_bind_info in_port_bind;
    mm_bind_info out_port_bind;
    pthread_t thread_id;
    struct aic_message_queue msg;

    MM_VIDEO_IN_SOURCE_TYPE vin_source_type;
    s32 framerate;
    s8 vin_source_change;
    s8 vin_dvp_init;
    s8 vin_usb_init;
    s8 vin_file_init;
    mm_param_content_uri *p_contenturi;
    FILE *p_vin_fp;
    s32 eos;
    struct mm_dvp_data dvp_data;
    struct mpp_frame frame;

    mm_vin_frame *p_frame_node;
    struct mpp_list vin_idle_list;
    struct mpp_list vin_ready_list;
    struct mpp_list vin_processing_list;
    pthread_cond_t vin_empty_cond;
    int wait_vin_empty_flag;

    pthread_mutex_t vin_lock;
    u32 caputure_frame_ok_num;
    u32 caputure_frame_fail_num;
    u32 send_frame_ok_num;
    u32 send_frame_fail_num;
    u32 giveback_frame_ok_num;
    u32 giveback_frame_fail_num;

} mm_vin_data;

#define MM_VID_BUF_NUM             3
#define MM_VID_BUF_PLANE_NUM       2

#define mm_vin_list_empty(list, mutex) \
    ({                                  \
        int ret = 0;                    \
        pthread_mutex_lock(&mutex);     \
        ret = mpp_list_empty(list);     \
        pthread_mutex_unlock(&mutex);   \
        (ret);                          \
    })

static void *mm_vin_component_thread(void *p_thread_data);
static void mm_vin_component_count_print(mm_vin_data *p_vin_data);

static s32 mm_vin_dvp_init(mm_vin_data *p_vin_data)
{
    s32 ret = MM_ERROR_NONE;
    s32 i = 0;
    struct mpp_video_fmt video_fmt = {0};
    struct dvp_out_fmt out_fmt = {0};
    mm_vin_frame *p_frame_node = NULL;

#ifdef AIC_USING_CAMERA
    /*initial vin*/
    if (mpp_vin_init(CAMERA_DEV_NAME))
        return MM_ERROR_INSUFFICIENT_RESOURCES;
#endif
    /*get sensor fmt*/
    ret = mpp_dvp_ioctl(DVP_IN_G_FMT, &video_fmt);
    if (ret < 0) {
        loge("ioctl(DVP_IN_G_FMT) failed! err -%d\n", -ret);
        // return -1;
    }
    /*set dvp subdev fmt*/
    p_vin_data->dvp_data.src_fmt = video_fmt;
    p_vin_data->dvp_data.w = p_vin_data->dvp_data.src_fmt.width;
    p_vin_data->dvp_data.h = p_vin_data->dvp_data.src_fmt.height;

    ret = mpp_dvp_ioctl(DVP_IN_S_FMT, &p_vin_data->dvp_data.src_fmt);
    if (ret < 0) {
        loge("ioctl(DVP_IN_S_FMT) failed! err -%d\n", -ret);
        return MM_ERROR_INSUFFICIENT_RESOURCES;
    }

    /*dvp config*/
    if (p_vin_data->frame.buf.size.width > 0 &&
        p_vin_data->frame.buf.size.width < p_vin_data->dvp_data.src_fmt.width) {
        out_fmt.width = p_vin_data->frame.buf.size.width;
    } else {
        out_fmt.width = p_vin_data->dvp_data.src_fmt.width;
    }
    if (p_vin_data->frame.buf.size.height > 0 &&
        p_vin_data->frame.buf.size.height < p_vin_data->dvp_data.src_fmt.height) {
        out_fmt.height = p_vin_data->frame.buf.size.height;
    } else {
        out_fmt.height = p_vin_data->dvp_data.src_fmt.height;
    }

    /*dvp not support user config fmt, should be update this config to frame info*/
    out_fmt.pixelformat = MPP_FMT_NV12;
    out_fmt.num_planes = MM_VID_BUF_PLANE_NUM;
    ret = mpp_dvp_ioctl(DVP_OUT_S_FMT, &out_fmt);
    if (ret < 0) {
        loge("ioctl(DVP_OUT_S_FMT) failed! err -%d\n", -ret);
        return MM_ERROR_INSUFFICIENT_RESOURCES;
    }
    p_vin_data->dvp_data.out_fmt = out_fmt;
    /*request dvp buffer*/
    if (mpp_dvp_ioctl(DVP_REQ_BUF, (void *)&p_vin_data->dvp_data.binfo) < 0) {
        loge("ioctl(DVP_REQ_BUF) failed!\n");
        return MM_ERROR_INSUFFICIENT_RESOURCES;
    }
    for (i = 0; i < MM_VID_BUF_NUM; i++) {
        if (mpp_dvp_ioctl(DVP_Q_BUF, (void *)(ptr_t)i) < 0) {
            loge("ioctl(DVP_Q_BUF) %d failed!\n", i);
            return MM_ERROR_INSUFFICIENT_RESOURCES;
        }
    }

    /*start dvp*/
    ret = mpp_dvp_ioctl(DVP_STREAM_ON, NULL);
    if (ret < 0) {
        loge("ioctl(DVP_STREAM_ON) failed! err -%d\n", -ret);
        return MM_ERROR_INSUFFICIENT_RESOURCES;
    }

    /*add empty frame node*/
    p_frame_node = p_vin_data->p_frame_node;

    for (i = 0; i < MM_VID_BUF_NUM; i++) {
        mpp_list_add_tail(&p_frame_node[i].list, &p_vin_data->vin_idle_list);
    }

    logi("%s, src_fmt: w(%d), h(%d); out_fmt(%d): w(%d), h(%d)\n", __func__,
        p_vin_data->dvp_data.src_fmt.width, p_vin_data->dvp_data.src_fmt.height,
        p_vin_data->dvp_data.out_fmt.pixelformat, p_vin_data->dvp_data.out_fmt.width,
        p_vin_data->dvp_data.out_fmt.height);
    return ret;
}

static s32 mm_vin_dvp_deinit(mm_vin_data *p_vin_data)
{
    s32 ret = MM_ERROR_NONE;
    ret = mpp_dvp_ioctl(DVP_STREAM_OFF, NULL);
    if (ret < 0) {
        loge("ioctl(DVP_STREAM_OFF) failed! err -%d\n", -ret);
        return MM_ERROR_UNDEFINED;
    }

    mpp_vin_deinit();

    return MM_ERROR_NONE;
}


static s32 mm_vin_file_init(mm_vin_data *p_vin_data)
{
    mm_vin_frame *p_frame_node;
    s32 width = 0;
    s32 height = 0;
    s32 size[3];
    s32 stride[3];
    s32 i, j;

    if (p_vin_data->frame.buf.size.width <= 0 ||
        p_vin_data->frame.buf.size.height <= 0) {
        loge("wrong file size:%d x %d", p_vin_data->frame.buf.size.width,
            p_vin_data->frame.buf.size.height);
        return MM_ERROR_BAD_PARAMETER;
    }
    width = p_vin_data->frame.buf.size.width;
    height = p_vin_data->frame.buf.size.height;


    for (i = 0; i < 3; i++) {
        size[i] = (i == 0) ? (width * height) : (width * height / 4);
        stride[i] = (i == 0) ? (width) : (width / 2);
    }
    p_frame_node = p_vin_data->p_frame_node;

    for (i = 0; i < MM_VID_BUF_NUM; i++) {
        for (j = 0; j < 3; j++) {
            p_frame_node[i].frame.buf.phy_addr[j] = mpp_phy_alloc(size[j]);
            p_frame_node[i].vir_addr[j] =
                (unsigned char *)(unsigned long)p_frame_node[i].frame.buf.phy_addr[j];
            p_frame_node[i].size[j] = size[j];
            p_frame_node[i].frame.buf.stride[j] = stride[j];
        }
        p_frame_node[i].frame.buf.buf_type = MPP_PHY_ADDR;
        p_frame_node[i].frame.buf.size.width = width;
        p_frame_node[i].frame.buf.size.height = height;
        p_frame_node[i].frame.buf.format = p_vin_data->frame.buf.format;
        mpp_list_add_tail(&p_frame_node[i].list, &p_vin_data->vin_idle_list);
    }

    if (p_vin_data->p_vin_fp) {
        fclose(p_vin_data->p_vin_fp);
        p_vin_data->p_vin_fp = NULL;
        p_vin_data->eos = 0;
    }

    p_vin_data->p_vin_fp = fopen((const char*)p_vin_data->p_contenturi->content_uri, "rb");
    if (!p_vin_data->p_vin_fp) {
        loge("fopen error");
        return MM_ERROR_NULL_POINTER;
    }

    return MM_ERROR_NONE;
}

static s32 mm_vin_file_deinit(mm_vin_data *p_vin_data)
{
    s32 i, j;
    if (!p_vin_data->p_frame_node) {
        return MM_ERROR_INSUFFICIENT_RESOURCES;
    }
    mm_vin_frame * p_frame_node = p_vin_data->p_frame_node;
    for (i = 0; i < MM_VID_BUF_NUM; i++) {
        for (j = 0; j < 3; j++) {
            if (p_frame_node[i].frame.buf.phy_addr[j]) {
                mpp_phy_free(p_frame_node[i].frame.buf.phy_addr[j]);
                p_frame_node[i].frame.buf.phy_addr[j] = 0;
            }
        }
    }

    return MM_ERROR_NONE;
}

static s32 mm_vin_send_command(mm_handle h_component, MM_COMMAND_TYPE cmd,
                               u32 param1, void *p_cmd_data)
{
    mm_vin_data *p_vin_data;
    s32 ret = MM_ERROR_NONE;
    struct aic_message msg;
    p_vin_data = (mm_vin_data *)(((mm_component *)h_component)->p_comp_private);
    msg.message_id = cmd;
    msg.param = param1;
    msg.data_size = 0;

    // now not use always NULL
    if (p_cmd_data != NULL) {
        msg.data = p_cmd_data;
        msg.data_size = strlen((char *)p_cmd_data);
    }

    aic_msg_put(&p_vin_data->msg, &msg);
    return ret;
}

static s32 mm_vin_get_parameter(mm_handle h_component, MM_INDEX_TYPE index, void *p_param)
{
    mm_vin_data *p_vin_data;
    s32 ret = MM_ERROR_NONE;

    p_vin_data = (mm_vin_data *)(((mm_component *)h_component)->p_comp_private);

    switch (index) {
    case MM_INDEX_PARAM_PORT_DEFINITION: {
        mm_param_port_def *port = (mm_param_port_def *)p_param;
        if (port->port_index == VDEC_PORT_IN_INDEX) {
            memcpy(port, &p_vin_data->in_port_def, sizeof(mm_param_port_def));
        } else if (port->port_index == VDEC_PORT_OUT_INDEX) {
            memcpy(port, &p_vin_data->out_port_def, sizeof(mm_param_port_def));
        } else {
            ret = MM_ERROR_BAD_PARAMETER;
        }
        break;
    }

    case MM_INDEX_PARAM_CONTENT_URI:
        memcpy(p_param, &p_vin_data->p_contenturi, ((mm_param_content_uri *)p_param)->size);
        break;

    default:
        ret = MM_ERROR_UNSUPPORT;
        break;
    }

    return ret;
}


static s32 mm_vin_index_param_contenturi(mm_vin_data *p_vin_data,
                                         mm_param_content_uri *p_contenturi)
{
    int ret = MM_ERROR_NONE;

    if (p_vin_data->p_contenturi == NULL) {
        p_vin_data->p_contenturi = (mm_param_content_uri *)mpp_alloc(
            sizeof(mm_param_content_uri) + MM_MAX_STRINGNAME_SIZE);
        if (p_vin_data->p_contenturi == NULL) {
            loge("alloc for content uri failed\n");
            return MM_ERROR_NULL_POINTER;
        }
    }
    memcpy(p_vin_data->p_contenturi, p_contenturi, p_contenturi->size);

    return ret;
}

static s32 mm_vin_set_parameter(mm_handle h_component, MM_INDEX_TYPE index, void *p_param)
{
    mm_vin_data *p_vin_data;
    s32 ret = MM_ERROR_NONE;

    p_vin_data = (mm_vin_data *)(((mm_component *)h_component)->p_comp_private);

    switch (index) {
    case MM_INDEX_PARAM_PORT_DEFINITION: { // width height
        mm_param_port_def *port = (mm_param_port_def *)p_param;
        p_vin_data->frame.buf.size.width = port->format.video.frame_width;
        p_vin_data->frame.buf.size.height = port->format.video.frame_height;
        p_vin_data->framerate = port->format.video.framerate;
        if (port->format.video.color_format == MM_COLOR_FORMAT_YUV420P) {
            p_vin_data->frame.buf.format = MPP_FMT_YUV420P;
        } else {
            loge("unsupport color_format %d", port->format.video.color_format);
            return MM_ERROR_UNSUPPORT;
        }
        break;
    }

    case MM_INDEX_PARAM_CONTENT_URI:
        ret = mm_vin_index_param_contenturi(p_vin_data, (mm_param_content_uri *)p_param);
        break;

    case MM_INDEX_PARAM_VIDEO_INPUT_SOURCE:
        p_vin_data->vin_source_type = ((mm_param_u32 *)p_param)->u32;
        break;

    case MM_INDEX_PARAM_PRINT_DEBUG_INFO:
        mm_vin_component_count_print(p_vin_data);
        break;
    default:
        break;
    }
    return ret;
}

static s32 mm_vin_get_config(mm_handle h_component, MM_INDEX_TYPE index, void *p_config)
{
    s32 ret = MM_ERROR_NONE;
    return ret;
}

static s32 mm_vin_set_config(mm_handle h_component, MM_INDEX_TYPE index, void *p_config)
{
    s32 ret = MM_ERROR_NONE;

    return ret;
}

static s32 mm_vin_get_state(mm_handle h_component, MM_STATE_TYPE *p_state)
{
    s32 ret = MM_ERROR_NONE;
    mm_vin_data *p_vin_data;
    p_vin_data = (mm_vin_data *)(((mm_component *)h_component)->p_comp_private);

    pthread_mutex_lock(&p_vin_data->state_lock);
    *p_state = p_vin_data->state;
    pthread_mutex_unlock(&p_vin_data->state_lock);

    return ret;
}

static s32 mm_vin_bind_request(mm_handle h_comp, u32 port, mm_handle h_bind_comp, u32 bind_port)
{
    s32 ret = MM_ERROR_NONE;
    mm_param_port_def *p_port;
    mm_bind_info *p_bind_info;
    mm_vin_data *p_vin_data;
    p_vin_data = (mm_vin_data *)(((mm_component *)h_comp)->p_comp_private);
    if (p_vin_data->state != MM_STATE_LOADED) {
        loge("Component is not in MM_STATE_LOADED,it is in%d,it can not tunnel\n", p_vin_data->state);
        return MM_ERROR_INVALID_STATE;
    }
    if (port == VIN_PORT_IN_INDEX) {
        p_port = &p_vin_data->in_port_def;
        p_bind_info = &p_vin_data->in_port_bind;
    } else if (port == VIN_PORT_OUT_INDEX) {
        p_port = &p_vin_data->out_port_def;
        p_bind_info = &p_vin_data->out_port_bind;
    } else {
        loge("component can not find port :%d\n", port);
        return MM_ERROR_BAD_PARAMETER;
    }

    // cancle setup tunnel
    if (NULL == h_bind_comp && 0 == bind_port) {
        p_bind_info->flag = MM_FALSE;
        p_bind_info->port_index = bind_port;
        p_bind_info->p_bind_comp = h_bind_comp;
        return MM_ERROR_NONE;
    }

    if (p_port->dir == MM_DIR_OUTPUT) {
        p_bind_info->port_index = bind_port;
        p_bind_info->p_bind_comp = h_bind_comp;
        p_bind_info->flag = MM_TRUE;
    } else if (p_port->dir == MM_DIR_INPUT) {
        mm_param_port_def port_def;
        port_def.port_index = bind_port;
        mm_get_parameter(h_bind_comp, MM_INDEX_PARAM_PORT_DEFINITION, &port_def);
        if (port_def.dir != MM_DIR_OUTPUT) {
            loge("both ports are input.\n");
            return MM_ERROR_PORT_NOT_COMPATIBLE;
        }
        p_bind_info->port_index = bind_port;
        p_bind_info->p_bind_comp = h_bind_comp;
        p_bind_info->flag = MM_TRUE;
    } else {
        loge("port is neither output nor input.\n");
        return MM_ERROR_PORT_NOT_COMPATIBLE;
    }
    return ret;
}

static s32 mm_vin_send_buffer(mm_handle h_component, mm_buffer *p_buffer)
{
    return MM_ERROR_NONE;
}

static s32 mm_vin_giveback_buffer(mm_handle h_component, mm_buffer *p_buffer)
{
    mm_vin_data *p_vin_data;
    mm_vin_frame *p_frame_node;

    p_vin_data = (mm_vin_data *)(((mm_component *)h_component)->p_comp_private);

    pthread_mutex_lock(&p_vin_data->vin_lock);
    if (mpp_list_empty(&p_vin_data->vin_processing_list)) {
        loge("giveback buffer to vin error");
        pthread_mutex_unlock(&p_vin_data->vin_lock);
        return MM_ERROR_MB_ERRORS_IN_FRAME;
    }
    p_frame_node = mpp_list_first_entry(&p_vin_data->vin_processing_list,
                                        struct mm_vin_frame, list);
    if (p_vin_data->vin_dvp_init) {
        if (mpp_dvp_ioctl(DVP_Q_BUF, (void *)(ptr_t)p_frame_node->frame_index) < 0) {
            loge("ioctl() failed frame_index %d!\n", p_frame_node->frame_index);
            return -1;
        }
    }

    mpp_list_del(&p_frame_node->list);
    mpp_list_add_tail(&p_frame_node->list, &p_vin_data->vin_idle_list);

    mm_vin_send_command(h_component, MM_COMMAND_NOPS, 0, NULL);
    p_vin_data->giveback_frame_ok_num++;
    if (p_vin_data->wait_vin_empty_flag == 1) {
        p_vin_data->wait_vin_empty_flag = 0;
        pthread_cond_signal(&p_vin_data->vin_empty_cond);
    }
    pthread_mutex_unlock(&p_vin_data->vin_lock);


    return MM_ERROR_NONE;
}

static s32 mm_vin_set_callback(mm_handle h_component, mm_callback *p_callback, void *p_app_data)
{
    s32 ret = MM_ERROR_NONE;
    mm_vin_data *p_vin_data;
    p_vin_data = (mm_vin_data *)(((mm_component *)h_component)->p_comp_private);
    p_vin_data->p_callback = p_callback;
    p_vin_data->p_app_data = p_app_data;
    return ret;
}

s32 mm_vin_component_deinit(mm_handle h_component)
{
    s32 ret = MM_ERROR_NONE;
    mm_component *p_comp;
    mm_vin_data *p_vin_data;
    p_comp = (mm_component *)h_component;
    struct aic_message msg;
    p_vin_data = (mm_vin_data *)p_comp->p_comp_private;

    pthread_mutex_lock(&p_vin_data->state_lock);
    if (p_vin_data->state != MM_STATE_LOADED) {
        logw("compoent is in %d,but not in MM_STATE_LOADED(1), can not FreeHandle.\n",
             p_vin_data->state);
        pthread_mutex_unlock(&p_vin_data->state_lock);
        return MM_ERROR_INVALID_STATE;
    }
    pthread_mutex_unlock(&p_vin_data->state_lock);

    msg.message_id = MM_COMMAND_STOP;
    msg.data_size = 0;
    aic_msg_put(&p_vin_data->msg, &msg);
    pthread_join(p_vin_data->thread_id, (void *)&ret);

    pthread_mutex_destroy(&p_vin_data->state_lock);
    pthread_mutex_destroy(&p_vin_data->vin_lock);
    pthread_cond_destroy(&p_vin_data->vin_empty_cond);

    aic_msg_destroy(&p_vin_data->msg);

    if (p_vin_data->vin_dvp_init) {
        mm_vin_dvp_deinit(p_vin_data);
    } else if (p_vin_data->vin_file_init && p_vin_data->p_vin_fp) {
        fclose(p_vin_data->p_vin_fp);
        p_vin_data->p_vin_fp = NULL;
        mm_vin_file_deinit(p_vin_data);
    }
    if (p_vin_data->p_contenturi) {
        mpp_free(p_vin_data->p_contenturi);
        p_vin_data->p_contenturi = NULL;
    }
    if (p_vin_data->p_frame_node) {
        mpp_free(p_vin_data->p_frame_node);
        p_vin_data->p_frame_node = NULL;
    }
    mpp_free(p_vin_data);
    p_vin_data = NULL;

    logd("mm_vin_component_deinit\n");
    return ret;
}

static int vin_thread_attr_init(pthread_attr_t *attr)
{
    // default stack size is 2K, it is not enough for decode thread
    if (attr == NULL) {
        return EINVAL;
    }
    pthread_attr_init(attr);
    attr->stacksize = 8 * 1024;
    attr->schedparam.sched_priority = MM_COMPONENT_VIN_THREAD_PRIORITY;
    return 0;
}

s32 mm_vin_component_init(mm_handle h_component)
{
    mm_component *p_comp;
    mm_vin_data *p_vin_data;
    s32 ret = MM_ERROR_NONE;
    u32 err;
    s8 msg_create = 0;
    s8 state_lock_init = 0;
    s8 vin_lock_init = 0;

    logi("mm_vin_component_init....");

    pthread_attr_t attr;
    vin_thread_attr_init(&attr);

    p_comp = (mm_component *)h_component;

    p_vin_data = (mm_vin_data *)mpp_alloc(sizeof(mm_vin_data));

    if (NULL == p_vin_data) {
        loge("mpp_alloc(sizeof(mm_vin_data) fail!");
        return MM_ERROR_INSUFFICIENT_RESOURCES;
    }

    memset(p_vin_data, 0x0, sizeof(mm_vin_data));
    p_comp->p_comp_private = (void *)p_vin_data;
    p_vin_data->state = MM_STATE_LOADED;
    p_vin_data->h_self = p_comp;

    p_comp->set_callback = mm_vin_set_callback;
    p_comp->send_command = mm_vin_send_command;
    p_comp->get_state = mm_vin_get_state;
    p_comp->get_parameter = mm_vin_get_parameter;
    p_comp->set_parameter = mm_vin_set_parameter;
    p_comp->get_config = mm_vin_get_config;
    p_comp->set_config = mm_vin_set_config;
    p_comp->bind_request = mm_vin_bind_request;
    p_comp->deinit = mm_vin_component_deinit;
    p_comp->giveback_buffer = mm_vin_giveback_buffer;
    p_comp->send_buffer = mm_vin_send_buffer;

    p_vin_data->in_port_def.port_index = VIN_PORT_IN_INDEX;
    p_vin_data->in_port_def.enable = MM_TRUE;
    p_vin_data->in_port_def.dir = MM_DIR_INPUT;

    p_vin_data->out_port_def.port_index = VIN_PORT_OUT_INDEX;
    p_vin_data->out_port_def.enable = MM_TRUE;
    p_vin_data->out_port_def.dir = MM_DIR_OUTPUT;

    p_vin_data->in_port_bind.port_index = VIN_PORT_IN_INDEX;
    p_vin_data->in_port_bind.p_self_comp = h_component;
    p_vin_data->out_port_bind.port_index = VIN_PORT_OUT_INDEX;
    p_vin_data->out_port_bind.p_self_comp = h_component;

    pthread_cond_init(&p_vin_data->vin_empty_cond, NULL);
    p_vin_data->wait_vin_empty_flag = 0;

    mpp_list_init(&p_vin_data->vin_idle_list);
    mpp_list_init(&p_vin_data->vin_ready_list);
    mpp_list_init(&p_vin_data->vin_processing_list);

    p_vin_data->p_frame_node = mpp_alloc(MM_VID_BUF_NUM * sizeof(mm_vin_frame));
    if (!p_vin_data->p_frame_node) {
        loge("malloc for frame node failed!");
        return MM_ERROR_INSUFFICIENT_RESOURCES;
    }
    memset(p_vin_data->p_frame_node, 0, MM_VID_BUF_NUM * sizeof(mm_vin_frame));

    if (aic_msg_create(&p_vin_data->msg) < 0) {
        loge("aic_msg_create fail!\n");
        ret = MM_ERROR_INSUFFICIENT_RESOURCES;
        goto _EXIT;
    }
    msg_create = 1;

    if (pthread_mutex_init(&p_vin_data->state_lock, NULL)) {
        loge("pthread_mutex_init fail!\n");
        goto _EXIT;
    }
    state_lock_init = 1;

    if (pthread_mutex_init(&p_vin_data->vin_lock, NULL)) {
        loge("pthread_mutex_init fail!\n");
        goto _EXIT;
    }
    vin_lock_init = 1;
    // Create the component thread
    err = pthread_create(&p_vin_data->thread_id, &attr, mm_vin_component_thread, p_vin_data);
    if (err) {
        loge("pthread_create venc component fail!");
        ret = MM_ERROR_INSUFFICIENT_RESOURCES;
        goto _EXIT;
    }

    return ret;

_EXIT:
    pthread_cond_destroy(&p_vin_data->vin_empty_cond);

    if (state_lock_init) {
        pthread_mutex_destroy(&p_vin_data->state_lock);
    }
    if (vin_lock_init) {
        pthread_mutex_destroy(&p_vin_data->vin_lock);
    }
    if (msg_create) {
        aic_msg_destroy(&p_vin_data->msg);
    }
    if (p_vin_data->p_frame_node) {
        mpp_free(p_vin_data->p_frame_node);
        p_vin_data->p_frame_node = NULL;
    }
    if (p_vin_data) {
        mpp_free(p_vin_data);
        p_vin_data = NULL;
    }
    return ret;
}

static void mm_vin_event_notify(mm_vin_data *p_vin_data, MM_EVENT_TYPE event,
                                u32 data1, u32 data2, void *p_event_data)
{
    if (p_vin_data && p_vin_data->p_callback && p_vin_data->p_callback->event_handler) {
        p_vin_data->p_callback->event_handler(
            p_vin_data->h_self,
            p_vin_data->p_app_data, event,
            data1, data2, p_event_data);
    }
}

static void mm_vin_state_change_to_invalid(mm_vin_data *p_vin_data)
{
    p_vin_data->state = MM_STATE_INVALID;
    mm_vin_event_notify(p_vin_data, MM_EVENT_ERROR,
                        MM_ERROR_INVALID_STATE, 0, NULL);
    mm_vin_event_notify(p_vin_data, MM_EVENT_CMD_COMPLETE,
                        MM_COMMAND_STATE_SET, p_vin_data->state, NULL);
}

static void mm_vin_state_change_to_idle(mm_vin_data *p_vin_data)
{
    if (MM_STATE_LOADED == p_vin_data->state) {
        if (p_vin_data->vin_source_type == MM_VIDEO_IN_SOURCE_DVP) {
            if (mm_vin_dvp_init(p_vin_data)) {
                loge("mm_vin_dvp_init  fail!!!!\n");
                goto state_exit;
            }
            p_vin_data->vin_dvp_init = MM_TRUE;
        } else if (p_vin_data->vin_source_type == MM_VIDEO_IN_SOURCE_FILE) {
            if (mm_vin_file_init(p_vin_data)) {
                loge("mm_vin_file_init  fail!!!!\n");
                goto state_exit;
            }
            p_vin_data->vin_file_init = MM_TRUE;
        } else {
            loge("unknown vin source type %d", p_vin_data->vin_source_type);
            goto state_exit;
        }

        p_vin_data->state = MM_STATE_LOADED;
        mm_vin_event_notify(p_vin_data, MM_EVENT_CMD_COMPLETE,
                            MM_COMMAND_STATE_SET, p_vin_data->state, NULL);
    } else if ((MM_STATE_EXECUTING != p_vin_data->state) &&
       (MM_STATE_PAUSE == p_vin_data->state)) {
        mm_vin_event_notify(p_vin_data, MM_EVENT_ERROR,
                            MM_ERROR_INCORRECT_STATE_TRANSITION,
                            p_vin_data->state, NULL);
        loge("MM_ERROR_INCORRECT_STATE_TRANSITION\n");
        return;
    }

    p_vin_data->state = MM_STATE_IDLE;
    mm_vin_event_notify(p_vin_data, MM_EVENT_CMD_COMPLETE,
                        MM_COMMAND_STATE_SET, p_vin_data->state, NULL);
    return;

state_exit:
    mm_vin_event_notify(p_vin_data, MM_EVENT_ERROR,
                        MM_ERROR_INCORRECT_STATE_TRANSITION,
                        p_vin_data->state, NULL);
}

static void mm_vin_state_change_to_loaded(mm_vin_data *p_vin_data)
{
    if ((p_vin_data->state != MM_STATE_IDLE) &&
        (p_vin_data->state != MM_STATE_EXECUTING) &&
        (p_vin_data->state != MM_STATE_PAUSE)) {
        mm_vin_event_notify(p_vin_data, MM_EVENT_ERROR,
                            MM_ERROR_INCORRECT_STATE_TRANSITION,
                            p_vin_data->state, NULL);
        loge("MM_ERROR_INCORRECT_STATE_TRANSITION\n");
        return;
    }

    p_vin_data->state = MM_STATE_LOADED;
    mm_vin_event_notify(p_vin_data, MM_EVENT_CMD_COMPLETE,
                        MM_COMMAND_STATE_SET, p_vin_data->state, NULL);
}

static void mm_vin_state_change_to_executing(mm_vin_data *p_vin_data)
{
    if ((MM_STATE_IDLE != p_vin_data->state) &&
        (MM_STATE_PAUSE != p_vin_data->state)) {
        mm_vin_event_notify(p_vin_data, MM_EVENT_ERROR,
                            MM_ERROR_INCORRECT_STATE_TRANSITION,
                            p_vin_data->state, NULL);
        loge("MM_ERROR_INCORRECT_STATE_TRANSITION\n");
        return;
    }
    p_vin_data->state = MM_STATE_EXECUTING;
}

static void mm_vin_state_change_to_pause(mm_vin_data *p_vin_data)
{
    if (MM_STATE_EXECUTING != p_vin_data->state) {
        mm_vin_event_notify(p_vin_data, MM_EVENT_ERROR,
                            MM_ERROR_INCORRECT_STATE_TRANSITION,
                            p_vin_data->state, NULL);
        logd("MM_ERROR_INCORRECT_STATE_TRANSITION\n");
        return;
    }
    p_vin_data->state = MM_STATE_PAUSE;
}

static int mm_vin_component_process_cmd(mm_vin_data *p_vin_data)
{
    s32 cmd = MM_COMMAND_UNKNOWN;
    s32 cmd_data;
    struct aic_message message;

    if (aic_msg_get(&p_vin_data->msg, &message) == 0) {
        cmd = message.message_id;
        cmd_data = message.param;
        logi("cmd:%d, cmd_data:%d\n", cmd, cmd_data);
        if (MM_COMMAND_STATE_SET == cmd) {
            pthread_mutex_lock(&p_vin_data->state_lock);
            if (p_vin_data->state == (MM_STATE_TYPE)(cmd_data)) {
                mm_vin_event_notify(p_vin_data, MM_EVENT_ERROR,
                                    MM_ERROR_SAME_STATE, 0, NULL);
                pthread_mutex_unlock(&p_vin_data->state_lock);
                goto CMD_EXIT;
            }
            switch ((MM_STATE_TYPE)(cmd_data)) {
            case MM_STATE_INVALID:
                mm_vin_state_change_to_invalid(p_vin_data);
                break;
            case MM_STATE_LOADED: // idel->loaded means stop
                mm_vin_state_change_to_loaded(p_vin_data);
                break;
            case MM_STATE_IDLE:
                mm_vin_state_change_to_idle(p_vin_data);
                break;
            case MM_STATE_EXECUTING:
                mm_vin_state_change_to_executing(p_vin_data);
                break;
            case MM_STATE_PAUSE:
                mm_vin_state_change_to_pause(p_vin_data);
                break;
            default:
                break;
            }
            pthread_mutex_unlock(&p_vin_data->state_lock);
        } else if (MM_COMMAND_STOP == cmd) {
            logi("mm_vin_component_thread ready to exit!!!\n");
            goto CMD_EXIT;
        }
    }

CMD_EXIT:
    return cmd;
}

static s32 mm_vin_component_capture_dvp_frame(mm_vin_data *p_vin_data, mm_vin_frame *p_frame_node)
{
    if (!p_vin_data->vin_dvp_init || !p_frame_node) {
        return MM_ERROR_EMPTY_DATA;
    }

    s32 ret = MM_ERROR_NONE;
    s32 i = 0;
    s32 index = 0;
    s32 frame_duration = p_vin_data->framerate > 0 ? (1000 / p_vin_data->framerate) : 40;
    struct vin_video_buf *p_binfo = &p_vin_data->dvp_data.binfo;

    ret = mpp_dvp_ioctl(DVP_DQ_BUF, (void *)&index);
    if (ret < 0) {
        logd("ioctl(DVP_DQ_BUF) failed! err -%d\n", -ret);
        return MM_ERROR_READ_FAILED;
    }

    p_frame_node->frame_index = index;
    p_frame_node->frame.buf.buf_type = MPP_PHY_ADDR;
    p_frame_node->frame.buf.size.width = p_vin_data->dvp_data.out_fmt.width;
    p_frame_node->frame.buf.size.height = p_vin_data->dvp_data.out_fmt.height;
    p_frame_node->frame.buf.stride[0] = p_vin_data->dvp_data.out_fmt.width;
    p_frame_node->frame.buf.stride[1] = p_vin_data->dvp_data.out_fmt.width;
    p_frame_node->frame.buf.stride[2] = 0;
    p_frame_node->frame.buf.format = p_vin_data->dvp_data.out_fmt.pixelformat;
    for (i = 0; i < MM_VID_BUF_PLANE_NUM; i++) {
        p_frame_node->frame.buf.phy_addr[i] = p_binfo->planes[index * MM_VID_BUF_PLANE_NUM + i].buf;
    }
    p_frame_node->frame.pts = p_vin_data->caputure_frame_ok_num * frame_duration;
    p_vin_data->caputure_frame_ok_num++;

    return MM_ERROR_NONE;
}

static s32 mm_vin_component_capture_usb_frame(mm_vin_data *p_vin_data, mm_vin_frame *p_frame_node)
{
    return MM_ERROR_NONE;
}

static s32 mm_vin_component_capture_file_frame(mm_vin_data *p_vin_data, mm_vin_frame *p_frame_node)
{
    if (!p_vin_data->p_vin_fp) {
        loge("p_vin_fp is null, uri:%s", p_vin_data->p_contenturi->content_uri);
        return MM_ERROR_NULL_POINTER;
    }

    s32 i = 0;
    s32 ret = 0;
    s32 frame_duration = p_vin_data->framerate > 0 ? (1000 / p_vin_data->framerate) : 40;
    pthread_mutex_lock(&p_vin_data->vin_lock);
    for (i = 0; i < 3; i++) {
        ret = fread(p_frame_node->vir_addr[i], 1, p_frame_node->size[i], p_vin_data->p_vin_fp);
        if (ret < p_frame_node->size[i]) {
            p_frame_node->frame.flags = FRAME_FLAG_EOS;
            p_vin_data->eos = 1;
            pthread_mutex_unlock(&p_vin_data->vin_lock);
            return MM_ERROR_NONE;
        }
    }
    pthread_mutex_unlock(&p_vin_data->vin_lock);
    p_frame_node->frame.pts = p_vin_data->caputure_frame_ok_num * frame_duration;
    p_vin_data->caputure_frame_ok_num++;

    return MM_ERROR_NONE;
}

static void mm_vin_component_count_print(mm_vin_data *p_vin_data)
{
    logi("[%s:%d]caputure_frame_ok_num:%u,caputure_frame_fail_num:%u,"
           "send_frame_ok_num:%u,send_frame_fail_num:%u,"
           "giveback_frame_ok_num:%u,giveback_frame_fail_num:%u\n",
           __FUNCTION__, __LINE__,
           p_vin_data->caputure_frame_ok_num,
           p_vin_data->caputure_frame_fail_num,
           p_vin_data->send_frame_ok_num,
           p_vin_data->send_frame_fail_num,
           p_vin_data->giveback_frame_ok_num,
           p_vin_data->giveback_frame_fail_num);
}

static void *mm_vin_component_thread(void *p_thread_data)
{
    s32 ret = MM_ERROR_NONE;
    s32 cmd = MM_COMMAND_UNKNOWN;
    MM_BOOL b_notify_frame_end = MM_FALSE;
    mm_vin_data *p_vin_data = (mm_vin_data *)p_thread_data;
    mm_vin_frame *p_frame_node = NULL;
    mm_buffer buffer;

    while (1) {
    _AIC_MSG_GET_:
        /* process cmd and change state*/
        cmd = mm_vin_component_process_cmd(p_vin_data);
        if (MM_COMMAND_STATE_SET == cmd) {
            continue;
        } else if (MM_COMMAND_STOP == cmd) {
            goto _EXIT;
        }

        if (p_vin_data->state != MM_STATE_EXECUTING) {
            aic_msg_wait_new_msg(&p_vin_data->msg, 0);
            continue;
        }
        if (p_vin_data->vin_source_type == MM_VIDEO_IN_SOURCE_UNKNOWN) {
            aic_msg_wait_new_msg(&p_vin_data->msg, 0);
            continue;
        }

        if (p_vin_data->eos) {
            if (!b_notify_frame_end) {
                mm_vin_event_notify(p_vin_data, MM_EVENT_BUFFER_FLAG, 0, 0, NULL);
                b_notify_frame_end = MM_TRUE;
            }
            aic_msg_wait_new_msg(&p_vin_data->msg, 0);
            continue;
        }

        while (!mpp_list_empty(&p_vin_data->vin_ready_list)) {
            pthread_mutex_lock(&p_vin_data->vin_lock);
            p_frame_node = mpp_list_first_entry(&p_vin_data->vin_ready_list,
                                                struct mm_vin_frame, list);
            /*send frame to venc component*/
            buffer.p_buffer = (u8 *)&p_frame_node->frame;
            buffer.data_type = MM_BUFFER_DATA_FRAME;
            ret = mm_send_buffer(p_vin_data->out_port_bind.p_bind_comp, &buffer);
            if (ret != 0) {
                logw("mm_send_buffer ret");
                p_vin_data->send_frame_fail_num++;
                pthread_mutex_unlock(&p_vin_data->vin_lock);
                goto _AIC_MSG_GET_;
            }
            p_vin_data->send_frame_ok_num++;
            mpp_list_del(&p_frame_node->list);
            mpp_list_add_tail(&p_frame_node->list, &p_vin_data->vin_processing_list);
            pthread_mutex_unlock(&p_vin_data->vin_lock);
        }

        /*get free node*/
        if (mm_vin_list_empty(&p_vin_data->vin_idle_list, p_vin_data->vin_lock)) {
            p_vin_data->wait_vin_empty_flag = 1;
            pthread_cond_wait(&p_vin_data->vin_empty_cond, &p_vin_data->vin_lock);
        }

        pthread_mutex_lock(&p_vin_data->vin_lock);
        p_frame_node = mpp_list_first_entry(&p_vin_data->vin_idle_list,
                                          struct mm_vin_frame, list);

        /*capture frame from dvp/file/usb*/
        if (p_vin_data->vin_source_type == MM_VIDEO_IN_SOURCE_DVP) {
            ret = mm_vin_component_capture_dvp_frame(p_vin_data, p_frame_node);
        } else if (p_vin_data->vin_source_type == MM_VIDEO_IN_SOURCE_USB) {
            ret = mm_vin_component_capture_usb_frame(p_vin_data, p_frame_node);
        } else if (p_vin_data->vin_source_type == MM_VIDEO_IN_SOURCE_FILE) {
            ret = mm_vin_component_capture_file_frame(p_vin_data, p_frame_node);
        }

        if (ret != MM_ERROR_NONE) {
            pthread_mutex_unlock(&p_vin_data->vin_lock);
            aic_msg_wait_new_msg(&p_vin_data->msg, 0);
            continue;
        }

        mpp_list_del(&p_frame_node->list);
        mpp_list_add_tail(&p_frame_node->list, &p_vin_data->vin_ready_list);
        pthread_mutex_unlock(&p_vin_data->vin_lock);

        usleep(20000);
    }
_EXIT:
    mm_vin_component_count_print(p_vin_data);
    return (void *)MM_ERROR_NONE;
}
