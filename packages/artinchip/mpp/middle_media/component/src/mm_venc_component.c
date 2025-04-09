/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <jun.ma@artinchip.com>
 * Desc: middle media venc component
 */

#include "mm_venc_component.h"
#include <malloc.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "aic_message.h"
#include "aic_muxer.h"
#include "dma_allocator.h"
#include "mpp_encoder.h"
#include "mpp_list.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_encoder.h"
#include "mpp_dec_type.h"

#define VENC_CAPTURE_PATH_LEN  128
#define VENC_PACKET_ONE_TIME_CREATE_NUM 16
#define VENC_PACKET_NUM_MAX 64
#define VENC_FRAME_ONE_TIME_CREATE_NUM 3
#define VENC_FRAME_NUM_MAX 32

typedef struct mm_venc_in_frame {
    struct mpp_frame frame;
    struct mpp_list list;
} mm_venc_in_frame;

typedef struct mm_venc_out_packet {
    struct aic_av_packet packet;
    u32 phy_addr;
    s32 size;
    s32 quality;
    struct mpp_list list;
} mm_venc_out_packet;

typedef struct mm_venc_coder_config {
    struct aic_av_video_stream video_stream;
    s32 quality; // 0- 100
} mm_mm_venc_coder_config;

typedef struct mm_venc_data {
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

    struct aic_av_video_stream video_stream;
    struct mpp_encoder *p_encoder;
    struct encode_config encoder_config;

    MM_BOOL stream_end_flag;
    MM_BOOL decode_end_flag;
    MM_BOOL frame_end_flag;
    MM_BOOL flags;
    enum mpp_codec_type code_type;
    s8 p_snapshot_file_path[VENC_CAPTURE_PATH_LEN];
    MM_BOOL capture_enable;

    u32 in_frame_node_num;
    struct mpp_list in_empty_frame;
    struct mpp_list in_ready_frame;
    struct mpp_list in_processed_frame;
    pthread_mutex_t in_frame_lock;

    s32 out_pkt_node_buffer;
    struct mpp_list out_empty_pkt;
    struct mpp_list out_ready_pkt;
    struct mpp_list out_processing_pkt;
    pthread_mutex_t out_pkt_lock;

    u32 receive_frame_ok_num;
    u32 receive_frame_fail_num;
    u32 giveback_frame_ok_num;
    u32 giveback_frame_fail_num;
    u32 encoder_frame_ok_num;
    u32 send_pkt_ok_num;
    u32 send_pkt_fail_num;
    u32 giveback_pkt_ok_num;
    u32 giveback_pkt_fail_num;
} mm_venc_data;

#define mm_venc_list_empty(list, mutex) \
    ({                                  \
        int ret = 0;                    \
        pthread_mutex_lock(&mutex);     \
        ret = mpp_list_empty(list);     \
        pthread_mutex_unlock(&mutex);   \
        (ret);                          \
    })

static void *mm_venc_component_thread(void *p_thread_data);
static void mm_venc_component_count_print(mm_venc_data *p_venc_data);


static s32 mm_venc_buffer_init(mm_venc_data *p_venc_data)
{
    if (p_venc_data->video_stream.width <=0 ||
        p_venc_data->video_stream.height <= 0 ||
        p_venc_data->encoder_config.quality <= 0) {
        loge("wrong params width %d, height %d, quality %d", p_venc_data->video_stream.width,
            p_venc_data->video_stream.height, p_venc_data->encoder_config.quality);
        return MM_ERROR_BAD_PARAMETER;
    }

    s32 error = MM_ERROR_NONE;
    s32 i;
    s32 width = p_venc_data->video_stream.width;
    s32 height = p_venc_data->video_stream.height;
    s32 quality = p_venc_data->encoder_config.quality;
    s32 size = width * height * 4 / 5 * quality / 100;

    for (i = 0; i < VENC_FRAME_ONE_TIME_CREATE_NUM; i++) {
        mm_venc_out_packet *p_pkt_node =
            (mm_venc_out_packet *)mpp_alloc(sizeof(mm_venc_out_packet));
        if (NULL == p_pkt_node) {
            break;
        }
        memset(p_pkt_node, 0x00, sizeof(mm_venc_out_packet));
        p_pkt_node->size = size;
        p_pkt_node->phy_addr = mpp_phy_alloc(p_pkt_node->size);
        if (p_pkt_node->phy_addr == 0) {
            loge("mpp_phy_alloc size %d failed", p_pkt_node->size);
            error = MM_ERROR_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        mpp_list_add_tail(&p_pkt_node->list, &p_venc_data->out_empty_pkt);
        p_venc_data->out_pkt_node_buffer++;
    }
    if (p_venc_data->out_pkt_node_buffer == 0) {
        loge("mpp_alloc out packet video node fail\n");
        error = MM_ERROR_INSUFFICIENT_RESOURCES;
    }

exit:
    return error;
}

static s32 mm_venc_send_command(mm_handle h_component, MM_COMMAND_TYPE cmd,
                                u32 param1, void *p_cmd_data)
{
    mm_venc_data *p_venc_data;
    s32 error = MM_ERROR_NONE;
    struct aic_message msg;
    p_venc_data = (mm_venc_data *)(((mm_component *)h_component)->p_comp_private);
    msg.message_id = cmd;
    msg.param = param1;
    msg.data_size = 0;

    // now not use always NULL
    if (p_cmd_data != NULL) {
        msg.data = p_cmd_data;
        msg.data_size = strlen((char *)p_cmd_data);
    }

    aic_msg_put(&p_venc_data->msg, &msg);
    return error;
}

static s32 mm_venc_get_parameter(mm_handle h_component, MM_INDEX_TYPE index, void *p_param)
{
    mm_venc_data *p_venc_data;
    s32 error = MM_ERROR_NONE;

    p_venc_data = (mm_venc_data *)(((mm_component *)h_component)->p_comp_private);

    switch (index) {
    case MM_INDEX_PARAM_PORT_DEFINITION: {
        mm_param_port_def *port = (mm_param_port_def *)p_param;
        if (port->port_index == VDEC_PORT_IN_INDEX) {
            memcpy(port, &p_venc_data->in_port_def, sizeof(mm_param_port_def));
        } else if (port->port_index == VDEC_PORT_OUT_INDEX) {
            memcpy(port, &p_venc_data->out_port_def, sizeof(mm_param_port_def));
        } else {
            error = MM_ERROR_BAD_PARAMETER;
        }
        break;
    }

    default:
        error = MM_ERROR_UNSUPPORT;
        break;
    }

    return error;
}

static s32 mm_venc_set_parameter(mm_handle h_component, MM_INDEX_TYPE index, void *p_param)
{
    mm_venc_data *p_venc_data;
    s32 error = MM_ERROR_NONE;

    p_venc_data = (mm_venc_data *)(((mm_component *)h_component)->p_comp_private);
    switch (index) {
    case MM_INDEX_PARAM_PORT_DEFINITION: { // width height
        mm_param_port_def *port = (mm_param_port_def *)p_param;
        p_venc_data->video_stream.width = port->format.video.frame_width;
        p_venc_data->video_stream.height = port->format.video.frame_height;
        logd("width:%d,height:%d\n", p_venc_data->video_stream.width,
                p_venc_data->video_stream.height);
        break;
    }
    case MM_INDEX_PARAM_QFACTOR: { // quality
        mm_image_param_qfactor *q = (mm_image_param_qfactor *)p_param;
        p_venc_data->encoder_config.quality = q->q_factor;
        logd("quality:%d\n", p_venc_data->encoder_config.quality);
        break;
    }
    case MM_INDEX_PARAM_PRINT_DEBUG_INFO:
        mm_venc_component_count_print(p_venc_data);
        break;

    default:
        break;
    }
    return error;
}

static s32 mm_venc_get_config(mm_handle h_component, MM_INDEX_TYPE index, void *p_config)
{
    return MM_ERROR_NONE;
}

static s32 mm_venc_set_config(mm_handle h_component, MM_INDEX_TYPE index, void *p_config)
{
    mm_venc_data *p_venc_data;
    s32 error = MM_ERROR_NONE;

    p_venc_data = (mm_venc_data *)(((mm_component *)h_component)->p_comp_private);
    switch (index) {
    case MM_INDEX_VENDOR_VIDEO_ENC_CAPTURE: {
        mm_param_video_capture *p_capture = (mm_param_video_capture *)p_config;
        s32 len = strlen((char *)p_capture->p_file_path);
        memset(p_venc_data->p_snapshot_file_path, 0,  VENC_CAPTURE_PATH_LEN);
        if(len < VENC_CAPTURE_PATH_LEN) {
            memcpy((char *)p_venc_data->p_snapshot_file_path, p_capture->p_file_path, len);
            p_venc_data->capture_enable = MM_TRUE;
        } else {
            loge("path %s len (%d) > %d\n", p_capture->p_file_path,
                len, VENC_CAPTURE_PATH_LEN);
            error = MM_ERROR_BAD_PARAMETER;
        }
        break;
    }
    default:
        break;
    }

    return error;
}

static s32 mm_venc_get_state(mm_handle h_component, MM_STATE_TYPE *p_state)
{
    s32 error = MM_ERROR_NONE;
    mm_venc_data *p_venc_data;
    p_venc_data = (mm_venc_data *)(((mm_component *)h_component)->p_comp_private);

    pthread_mutex_lock(&p_venc_data->state_lock);
    *p_state = p_venc_data->state;
    pthread_mutex_unlock(&p_venc_data->state_lock);

    return error;
}

static s32 mm_venc_bind_request(mm_handle h_comp, u32 port,
                                mm_handle h_bind_comp, u32 bind_port)
{
    s32 error = MM_ERROR_NONE;
    mm_param_port_def *p_port;
    mm_bind_info *p_bind_info;
    mm_venc_data *p_venc_data;
    p_venc_data = (mm_venc_data *)(((mm_component *)h_comp)->p_comp_private);
    if (p_venc_data->state != MM_STATE_LOADED) {
        loge("Component is not in MM_STATE_LOADED,it is in%d,it can not tunnel\n",
             p_venc_data->state);
        return MM_ERROR_INVALID_STATE;
    }
    if (port == VENC_PORT_IN_INDEX) {
        p_port = &p_venc_data->in_port_def;
        p_bind_info = &p_venc_data->in_port_bind;
    } else if (port == VENC_PORT_OUT_INDEX) {
        p_port = &p_venc_data->out_port_def;
        p_bind_info = &p_venc_data->out_port_bind;
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
    return error;
}

static s32 mm_venc_send_buffer(mm_handle h_component, mm_buffer *p_buffer)
{
    s32 error = MM_ERROR_NONE;
    mm_venc_data *p_venc_data;
    mm_venc_in_frame *p_frame_node;
    struct aic_message msg;
    struct mpp_frame *p_frame = NULL;
    p_venc_data = (mm_venc_data *)(((mm_component *)h_component)->p_comp_private);

    pthread_mutex_lock(&p_venc_data->state_lock);
    if (p_venc_data->state != MM_STATE_EXECUTING) {
        loge("component is not in MM_STATE_EXECUTING,it is in [%d]!!!\n",
             p_venc_data->state);
        pthread_mutex_unlock(&p_venc_data->state_lock);
        return MM_ERROR_INVALID_STATE;
    }

    if (mm_venc_list_empty(&p_venc_data->in_empty_frame,
                           p_venc_data->in_frame_lock)) {
        mm_venc_in_frame *p_frame_node =
            (mm_venc_in_frame *)mpp_alloc(sizeof(mm_venc_in_frame));
        if (NULL == p_frame_node) {
            loge("MM_ERROR_INSUFFICIENT_RESOURCES\n");
            pthread_mutex_unlock(&p_venc_data->state_lock);
            return MM_ERROR_INSUFFICIENT_RESOURCES;
        }
        memset(p_frame_node, 0x00, sizeof(mm_venc_in_frame));
        pthread_mutex_lock(&p_venc_data->in_frame_lock);
        mpp_list_add_tail(&p_frame_node->list, &p_venc_data->in_empty_frame);
        pthread_mutex_unlock(&p_venc_data->in_frame_lock);
        p_venc_data->in_frame_node_num++;
    }

    pthread_mutex_lock(&p_venc_data->in_frame_lock);
    p_frame_node = mpp_list_first_entry(&p_venc_data->in_empty_frame,
                                        mm_venc_in_frame, list);
    p_frame = (struct mpp_frame *)p_buffer->p_buffer;
    memcpy(&p_frame_node->frame, p_frame, sizeof(struct mpp_frame));
    mpp_list_del(&p_frame_node->list);
    mpp_list_add_tail(&p_frame_node->list, &p_venc_data->in_ready_frame);
    pthread_mutex_unlock(&p_venc_data->in_frame_lock);

    msg.message_id = MM_COMMAND_NOPS;
    msg.data_size = 0;
    aic_msg_put(&p_venc_data->msg, &msg);

    p_venc_data->receive_frame_ok_num++;

    pthread_mutex_unlock(&p_venc_data->state_lock);
    return error;
}

static s32 mm_venc_giveback_buffer(mm_handle h_component, mm_buffer *p_buffer)
{
    s32 error = MM_ERROR_NONE;
    mm_venc_data *p_venc_data;
    struct aic_av_packet *p_packet;
    mm_venc_out_packet *p_pkt_node1 = NULL, *p_pkt_node2 = NULL;
    MM_BOOL b_match = MM_FALSE;
    p_venc_data = (mm_venc_data *)(((mm_component *)h_component)->p_comp_private);

    if (p_buffer->output_port_index != VENC_PORT_OUT_INDEX) {
        loge("port not match\n");
        return MM_ERROR_BAD_PARAMETER;
    }

    if (!mm_venc_list_empty(&p_venc_data->out_processing_pkt, p_venc_data->out_pkt_lock)) {
        p_packet = (struct aic_av_packet *)p_buffer->p_buffer;
        pthread_mutex_lock(&p_venc_data->out_pkt_lock);
        mpp_list_for_each_entry_safe(p_pkt_node1, p_pkt_node2,
                                     &p_venc_data->out_processing_pkt, list)
        {
            if (p_pkt_node1->packet.data == p_packet->data) {
                b_match = MM_TRUE;
                break;
            }
        }
        logi("b_match:%d\n", b_match);
        if (b_match) { // give frame back to decoder
            struct aic_message msg;

            mpp_list_del(&p_pkt_node1->list);
            mpp_list_add_tail(&p_pkt_node1->list, &p_venc_data->out_empty_pkt);

            msg.message_id = MM_COMMAND_NOPS;
            msg.data_size = 0;
            aic_msg_put(&p_venc_data->msg, &msg);
            p_venc_data->giveback_pkt_ok_num++;
        } else {
            loge("frame not match!!!\n");
            error = MM_ERROR_BAD_PARAMETER;
            p_venc_data->giveback_pkt_fail_num++;
        }
        logi("p_venc_data->giveback_frame_ok_num:%d,p_venc_data->giveback_frame_fail_num:%d",
             p_venc_data->giveback_pkt_ok_num, p_venc_data->giveback_pkt_fail_num);
        pthread_mutex_unlock(&p_venc_data->out_pkt_lock);
    } else {
        logw("no frame need to back \n");
        error = MM_ERROR_BAD_PARAMETER;
    }

    return error;
}

static s32 mm_venc_set_callback(mm_handle h_component, mm_callback *p_callback, void *p_app_data)
{
    s32 error = MM_ERROR_NONE;
    mm_venc_data *p_venc_data;
    p_venc_data = (mm_venc_data *)(((mm_component *)h_component)->p_comp_private);
    p_venc_data->p_callback = p_callback;
    p_venc_data->p_app_data = p_app_data;
    return error;
}

s32 mm_venc_component_deinit(mm_handle h_component)
{
    s32 error = MM_ERROR_NONE;
    mm_component *p_comp;
    mm_venc_data *p_venc_data;
    mm_venc_out_packet *p_pkt_node = NULL, *p_pkt_node1 = NULL;
    mm_venc_in_frame *p_frame_node = NULL, *p_frame_node1 = NULL;
    p_comp = (mm_component *)h_component;
    struct aic_message msg;
    p_venc_data = (mm_venc_data *)p_comp->p_comp_private;

    pthread_mutex_lock(&p_venc_data->state_lock);
    if (p_venc_data->state != MM_STATE_LOADED) {
        logw("compoent is in %d,but not in MM_STATE_LOADED(1),can not FreeHandle.\n",
             p_venc_data->state);
        pthread_mutex_unlock(&p_venc_data->state_lock);
        return MM_ERROR_INVALID_STATE;
    }
    pthread_mutex_unlock(&p_venc_data->state_lock);

    msg.message_id = MM_COMMAND_STOP;
    msg.data_size = 0;
    aic_msg_put(&p_venc_data->msg, &msg);
    pthread_join(p_venc_data->thread_id, (void *)&error);

    pthread_mutex_lock(&p_venc_data->in_frame_lock);
    if (!mpp_list_empty(&p_venc_data->in_empty_frame)) {
        mpp_list_for_each_entry_safe(p_frame_node, p_frame_node1,
                                     &p_venc_data->in_empty_frame, list)
        {
            mpp_list_del(&p_frame_node->list);
            mpp_free(p_frame_node);
        }
    }

    if (!mpp_list_empty(&p_venc_data->in_ready_frame)) {
        mpp_list_for_each_entry_safe(p_frame_node, p_frame_node1,
                                     &p_venc_data->in_ready_frame, list)
        {
            mpp_list_del(&p_frame_node->list);
            mpp_free(p_pkt_node);
        }
    }

    pthread_mutex_unlock(&p_venc_data->in_frame_lock);

    pthread_mutex_lock(&p_venc_data->out_pkt_lock);
    if (!mpp_list_empty(&p_venc_data->out_empty_pkt)) {
        mpp_list_for_each_entry_safe(p_pkt_node, p_pkt_node1,
                                     &p_venc_data->out_empty_pkt, list)
        {
            mpp_list_del(&p_pkt_node->list);
            if (p_pkt_node->phy_addr) {
                mpp_phy_free(p_pkt_node->phy_addr);
                p_pkt_node->phy_addr = 0;
            }
            mpp_free(p_pkt_node);
        }
    }
    if (!mpp_list_empty(&p_venc_data->out_ready_pkt)) {
        mpp_list_for_each_entry_safe(p_pkt_node, p_pkt_node1,
                                     &p_venc_data->out_ready_pkt, list)
        {
            mpp_list_del(&p_pkt_node->list);
            if (p_pkt_node->phy_addr) {
                mpp_phy_free(p_pkt_node->phy_addr);
                p_pkt_node->phy_addr = 0;
            }
            mpp_free(p_pkt_node);
        }
    }

    if (!mpp_list_empty(&p_venc_data->out_processing_pkt)) {
        mpp_list_for_each_entry_safe(p_pkt_node, p_pkt_node1,
                                     &p_venc_data->out_processing_pkt, list)
        {
            mpp_list_del(&p_pkt_node->list);
            if (p_pkt_node->phy_addr) {
                mpp_phy_free(p_pkt_node->phy_addr);
                p_pkt_node->phy_addr = 0;
            }
            mpp_free(p_pkt_node);
        }
    }
    pthread_mutex_unlock(&p_venc_data->out_pkt_lock);

    pthread_mutex_destroy(&p_venc_data->in_frame_lock);
    pthread_mutex_destroy(&p_venc_data->out_pkt_lock);
    pthread_mutex_destroy(&p_venc_data->state_lock);

    aic_msg_destroy(&p_venc_data->msg);

    if (p_venc_data->p_encoder) {
        mpp_encoder_destory(p_venc_data->p_encoder);
        p_venc_data->p_encoder = NULL;
    }

    mpp_free(p_venc_data);
    p_venc_data = NULL;

    logd("mm_venc_component_deinit\n");
    return error;
}

static int venc_thread_attr_init(pthread_attr_t *attr)
{
    if (attr == NULL) {
        return EINVAL;
    }
    pthread_attr_init(attr);
    attr->stacksize = 8 * 1024;
    attr->schedparam.sched_priority = MM_COMPONENT_VENC_THREAD_PRIORITY;
    return 0;
}

s32 mm_venc_component_init(mm_handle h_component)
{
    mm_component *p_comp;
    mm_venc_data *p_venc_data;
    s32 error = MM_ERROR_NONE;
    u32 err;
    u32 i;
    s8 msg_create = 0;
    s8 in_frame_lock_init = 0;
    s8 out_pkt_lock_init = 0;
    s8 state_lock_init = 0;

    logi("mm_venc_component_init....");

    pthread_attr_t attr;
    venc_thread_attr_init(&attr);

    p_comp = (mm_component *)h_component;

    p_venc_data = (mm_venc_data *)mpp_alloc(sizeof(mm_venc_data));

    if (NULL == p_venc_data) {
        loge("mpp_alloc(sizeof(mm_venc_data) fail!");
        return MM_ERROR_INSUFFICIENT_RESOURCES;
    }

    memset(p_venc_data, 0x0, sizeof(mm_venc_data));
    p_comp->p_comp_private = (void *)p_venc_data;
    p_venc_data->state = MM_STATE_LOADED;
    p_venc_data->h_self = p_comp;

    p_comp->set_callback = mm_venc_set_callback;
    p_comp->send_command = mm_venc_send_command;
    p_comp->get_state = mm_venc_get_state;
    p_comp->get_parameter = mm_venc_get_parameter;
    p_comp->set_parameter = mm_venc_set_parameter;
    p_comp->get_config = mm_venc_get_config;
    p_comp->set_config = mm_venc_set_config;
    p_comp->bind_request = mm_venc_bind_request;
    p_comp->deinit = mm_venc_component_deinit;
    p_comp->giveback_buffer = mm_venc_giveback_buffer;
    p_comp->send_buffer = mm_venc_send_buffer;

    p_venc_data->in_port_def.port_index = VENC_PORT_IN_INDEX;
    p_venc_data->in_port_def.enable = MM_TRUE;
    p_venc_data->in_port_def.dir = MM_DIR_INPUT;

    p_venc_data->out_port_def.port_index = VENC_PORT_OUT_INDEX;
    p_venc_data->out_port_def.enable = MM_TRUE;
    p_venc_data->out_port_def.dir = MM_DIR_OUTPUT;

    p_venc_data->in_port_bind.port_index = VENC_PORT_IN_INDEX;
    p_venc_data->in_port_bind.p_self_comp = h_component;
    p_venc_data->out_port_bind.port_index = VENC_PORT_OUT_INDEX;
    p_venc_data->out_port_bind.p_self_comp = h_component;

    p_venc_data->in_frame_node_num = 0;
    mpp_list_init(&p_venc_data->in_empty_frame);
    mpp_list_init(&p_venc_data->in_ready_frame);
    mpp_list_init(&p_venc_data->in_processed_frame);
    for (i = 0; i < VENC_PACKET_ONE_TIME_CREATE_NUM; i++) {
        mm_venc_in_frame *p_frame_node =
            (mm_venc_in_frame *)mpp_alloc(sizeof(mm_venc_in_frame));
        if (NULL == p_frame_node) {
            break;
        }
        memset(p_frame_node, 0x00, sizeof(mm_venc_in_frame));
        mpp_list_add_tail(&p_frame_node->list, &p_venc_data->in_empty_frame);
        p_venc_data->in_frame_node_num++;
    }
    if (p_venc_data->in_frame_node_num == 0) {
        loge("mpp_alloc in frame video node fail\n");
        error = MM_ERROR_INSUFFICIENT_RESOURCES;
        goto _EXIT;
    }

    p_venc_data->out_pkt_node_buffer = 0;
    mpp_list_init(&p_venc_data->out_empty_pkt);
    mpp_list_init(&p_venc_data->out_ready_pkt);
    mpp_list_init(&p_venc_data->out_processing_pkt);

    if (aic_msg_create(&p_venc_data->msg) < 0) {
        loge("aic_msg_create fail!\n");
        error = MM_ERROR_INSUFFICIENT_RESOURCES;
        goto _EXIT;
    }
    msg_create = 1;
    if (pthread_mutex_init(&p_venc_data->in_frame_lock, NULL)) {
        loge("pthread_mutex_init fail!\n");
        goto _EXIT;
    }
    in_frame_lock_init = 1;
    if (pthread_mutex_init(&p_venc_data->out_pkt_lock, NULL)) {
        loge("pthread_mutex_init fail!\n");
        goto _EXIT;
    }
    out_pkt_lock_init = 1;
    if (pthread_mutex_init(&p_venc_data->state_lock, NULL)) {
        loge("pthread_mutex_init fail!\n");
        goto _EXIT;
    }
    state_lock_init = 1;

    // Create the component thread
    err = pthread_create(&p_venc_data->thread_id, &attr, mm_venc_component_thread, p_venc_data);
    if (err) {
        loge("pthread_create venc component fail!");
        error = MM_ERROR_INSUFFICIENT_RESOURCES;
        goto _EXIT;
    }

    return error;

_EXIT:
    if (state_lock_init) {
        pthread_mutex_destroy(&p_venc_data->state_lock);
    }
    if (out_pkt_lock_init) {
        pthread_mutex_destroy(&p_venc_data->out_pkt_lock);
    }
    if (in_frame_lock_init) {
        pthread_mutex_destroy(&p_venc_data->in_frame_lock);
    }
    if (msg_create) {
        aic_msg_destroy(&p_venc_data->msg);
    }
    if (!mpp_list_empty(&p_venc_data->out_empty_pkt)) {
        mm_venc_out_packet *p_pkt_node = NULL, *p_pkt_node1 = NULL;
        mpp_list_for_each_entry_safe(p_pkt_node, p_pkt_node1,
                                     &p_venc_data->out_empty_pkt, list)
        {
            mpp_list_del(&p_pkt_node->list);
            mpp_free(p_pkt_node);
        }
    }
    if (!mpp_list_empty(&p_venc_data->in_empty_frame)) {
        mm_venc_in_frame *p_frame_node = NULL, *p_frame_node1 = NULL;
        mpp_list_for_each_entry_safe(p_frame_node, p_frame_node1,
                                     &p_venc_data->in_empty_frame, list)
        {
            mpp_list_del(&p_frame_node->list);
            mpp_free(p_frame_node);
        }
    }
    if (p_venc_data) {
        mpp_free(p_venc_data);
        p_venc_data = NULL;
    }
    return error;
}

static void mm_venc_event_notify(mm_venc_data *p_venc_data, MM_EVENT_TYPE event,
                                 u32 data1, u32 data2, void *p_event_data)
{
    if (p_venc_data && p_venc_data->p_callback &&
        p_venc_data->p_callback->event_handler) {
        p_venc_data->p_callback->event_handler(
            p_venc_data->h_self,
            p_venc_data->p_app_data, event,
            data1, data2, p_event_data);
    }
}

static void mm_venc_state_change_to_invalid(mm_venc_data *p_venc_data)
{
    p_venc_data->state = MM_STATE_INVALID;
    mm_venc_event_notify(p_venc_data, MM_EVENT_ERROR,
                         MM_ERROR_INVALID_STATE, 0, NULL);
    mm_venc_event_notify(p_venc_data, MM_EVENT_CMD_COMPLETE,
                         MM_COMMAND_STATE_SET, p_venc_data->state, NULL);
}

static void mm_venc_state_change_to_idle(mm_venc_data *p_venc_data)
{
    if (MM_STATE_LOADED == p_venc_data->state) {
        if(mm_venc_buffer_init(p_venc_data)) {
            mm_venc_event_notify(p_venc_data, MM_EVENT_ERROR,
                                MM_ERROR_INCORRECT_STATE_TRANSITION,
                                p_venc_data->state, NULL);
            loge("mm_venc_buffer_init failed\n");
            return;
        }
        // create encoder
        if (p_venc_data->p_encoder == NULL) {
            p_venc_data->p_encoder = mpp_encoder_create(MPP_CODEC_VIDEO_ENCODER_MJPEG);
            if (p_venc_data->p_encoder == NULL) {
                loge("mpp_encoder_create failed!\n ");
                mm_venc_event_notify(p_venc_data, MM_EVENT_ERROR,
                                     MM_ERROR_INCORRECT_STATE_TRANSITION,
                                     p_venc_data->state, NULL);
                return;
            }

            if (mpp_encoder_init(p_venc_data->p_encoder, &p_venc_data->encoder_config)) {
                loge("mpp_encoder_init failed, then destroy encoder!\n ");
                mpp_encoder_destory(p_venc_data->p_encoder);
                p_venc_data->p_encoder = NULL;
                mm_venc_event_notify(p_venc_data, MM_EVENT_ERROR,
                                     MM_ERROR_INCORRECT_STATE_TRANSITION,
                                     p_venc_data->state, NULL);
                return;
            }
        }
    } else if (MM_STATE_EXECUTING == p_venc_data->state) {
    } else if (MM_STATE_PAUSE == p_venc_data->state) {
    } else {
        mm_venc_event_notify(p_venc_data, MM_EVENT_ERROR,
                             MM_ERROR_INCORRECT_STATE_TRANSITION,
                             p_venc_data->state, NULL);
        loge("MM_ERROR_INCORRECT_STATE_TRANSITION\n");
        return;
    }

    p_venc_data->state = MM_STATE_IDLE;
    mm_venc_event_notify(p_venc_data, MM_EVENT_CMD_COMPLETE,
                         MM_COMMAND_STATE_SET, p_venc_data->state, NULL);
}

static void mm_venc_state_change_to_loaded(mm_venc_data *p_venc_data)
{
    if (MM_STATE_IDLE == p_venc_data->state) {
        // wait for	all out port packet from other component or app to back.
        logi("mm_venc_state_change_to_loaded\n");

        p_venc_data->state = MM_STATE_LOADED;
        mm_venc_event_notify(p_venc_data, MM_EVENT_CMD_COMPLETE,
                             MM_COMMAND_STATE_SET, p_venc_data->state, NULL);
    } else {
        mm_venc_event_notify(p_venc_data, MM_EVENT_ERROR,
                             MM_ERROR_INCORRECT_STATE_TRANSITION,
                             p_venc_data->state, NULL);
        loge("MM_ERROR_INCORRECT_STATE_TRANSITION\n");
    }
}

static void mm_venc_state_change_to_executing(mm_venc_data *p_venc_data)
{
    if ((MM_STATE_IDLE != p_venc_data->state) &&
        (MM_STATE_PAUSE != p_venc_data->state)) {
        mm_venc_event_notify(p_venc_data, MM_EVENT_ERROR,
                             MM_ERROR_INCORRECT_STATE_TRANSITION,
                             p_venc_data->state, NULL);
        loge("MM_ERROR_INCORRECT_STATE_TRANSITION\n");
        return;
    }
    p_venc_data->state = MM_STATE_EXECUTING;
}

static void mm_venc_state_change_to_pause(mm_venc_data *p_venc_data)
{
    if (MM_STATE_EXECUTING != p_venc_data->state) {
        mm_venc_event_notify(p_venc_data, MM_EVENT_ERROR,
                             MM_ERROR_INCORRECT_STATE_TRANSITION,
                             p_venc_data->state, NULL);
        logd("MM_ERROR_INCORRECT_STATE_TRANSITION\n");
        return;
    }
    p_venc_data->state = MM_STATE_PAUSE;
}

static int mm_venc_component_process_cmd(mm_venc_data *p_venc_data)
{
    s32 cmd = MM_COMMAND_UNKNOWN;
    s32 cmd_data;
    struct aic_message message;

    if (aic_msg_get(&p_venc_data->msg, &message) == 0) {
        cmd = message.message_id;
        cmd_data = message.param;
        logi("cmd:%d, cmd_data:%d\n", cmd, cmd_data);
        if (MM_COMMAND_STATE_SET == cmd) {
            pthread_mutex_lock(&p_venc_data->state_lock);
            if (p_venc_data->state == (MM_STATE_TYPE)(cmd_data)) {
                mm_venc_event_notify(p_venc_data, MM_EVENT_ERROR,
                                     MM_ERROR_SAME_STATE, 0, NULL);
                pthread_mutex_unlock(&p_venc_data->state_lock);
                goto CMD_EXIT;
            }
            switch ((MM_STATE_TYPE)(cmd_data)) {
            case MM_STATE_INVALID:
                mm_venc_state_change_to_invalid(p_venc_data);
                break;
            case MM_STATE_LOADED: // idel->loaded means stop
                mm_venc_state_change_to_loaded(p_venc_data);
                break;
            case MM_STATE_IDLE:
                mm_venc_state_change_to_idle(p_venc_data);
                break;
            case MM_STATE_EXECUTING:
                mm_venc_state_change_to_executing(p_venc_data);
                break;
            case MM_STATE_PAUSE:
                mm_venc_state_change_to_pause(p_venc_data);
                break;
            default:
                break;
            }
            pthread_mutex_unlock(&p_venc_data->state_lock);
        } else if (MM_COMMAND_STOP == cmd) {
            logi("mm_venc_component_thread ready to exit!!!\n");
            goto CMD_EXIT;
        }
    }

CMD_EXIT:
    return cmd;
}

void mm_venc_component_count_print(mm_venc_data *p_venc_data)
{
    printf("[%s:%d]receive_frame_ok_num:%u,receive_frame_fail_num:%u,"
           "giveback_frame_ok_num:%u,giveback_frame_fail_num:%u,"
           "encoder_frame_ok_num:%u,send_pkt_ok_num:%u,send_pkt_fail_num:%u,"
           "giveback_pkt_ok_num:%u,giveback_pkt_fail_num:%u\n",
           __FUNCTION__, __LINE__,
           p_venc_data->receive_frame_ok_num,
           p_venc_data->receive_frame_fail_num,
           p_venc_data->giveback_frame_ok_num,
           p_venc_data->giveback_frame_fail_num,
           p_venc_data->encoder_frame_ok_num,
           p_venc_data->send_pkt_ok_num,
           p_venc_data->send_pkt_fail_num,
           p_venc_data->giveback_pkt_ok_num,
           p_venc_data->giveback_pkt_fail_num);
}


static int mm_venc_component_process_snapshot(mm_venc_data *p_venc_data, mm_venc_out_packet *p_pkt_node)
{
    if (MM_FALSE == p_venc_data->capture_enable) {
        return 0;
    }

    FILE *fp_save = fopen((char *)p_venc_data->p_snapshot_file_path, "wb");
    if (fp_save == NULL) {
        loge("fopen %s error\n", p_venc_data->p_snapshot_file_path);
        return MM_ERROR_NULL_POINTER;
    }

    fwrite(p_pkt_node->packet.data, 1, p_pkt_node->packet.size, fp_save);

    fclose(fp_save);
    p_venc_data->capture_enable = MM_FALSE;

    printf("%s,snapshot one frame: %s\n", __func__, p_venc_data->p_snapshot_file_path);

    return MM_ERROR_NONE;
}

static void *mm_venc_component_thread(void *p_thread_data)
{
    s32 ret = MM_ERROR_NONE;
    s32 cmd = MM_COMMAND_UNKNOWN;
    mm_venc_data *p_venc_data = (mm_venc_data *)p_thread_data;

    mm_venc_in_frame *p_frame_node = NULL;
    mm_venc_out_packet *p_pkt_node = NULL;
    mm_buffer venc_buffer;
    struct mpp_packet packet;

    while (1) {
    _AIC_MSG_GET_:
        /* process cmd and change state*/
        cmd = mm_venc_component_process_cmd(p_venc_data);
        if (MM_COMMAND_STATE_SET == cmd) {
            continue;
        } else if (MM_COMMAND_STOP == cmd) {
            goto _EXIT;
        }

        if (p_venc_data->state != MM_STATE_EXECUTING) {
            aic_msg_wait_new_msg(&p_venc_data->msg, 0);
            continue;
        }

        /* giveback frame*/
        while (!mm_venc_list_empty(&p_venc_data->in_processed_frame,
                                   p_venc_data->in_frame_lock)) {
            pthread_mutex_lock(&p_venc_data->in_frame_lock);
            p_frame_node = mpp_list_first_entry(&p_venc_data->in_processed_frame,
                                                mm_venc_in_frame, list);
            pthread_mutex_unlock(&p_venc_data->in_frame_lock);
            venc_buffer.p_buffer = (u8 *)&p_frame_node->frame;
            ret = mm_giveback_buffer(p_venc_data->in_port_bind.p_bind_comp, &venc_buffer);
            if (ret != 0) {
                loge("give back frame to vin fail\n");
                p_venc_data->giveback_frame_fail_num++;
                break;
            }
            p_venc_data->giveback_frame_ok_num++;
            pthread_mutex_lock(&p_venc_data->in_frame_lock);
            mpp_list_del(&p_frame_node->list);
            mpp_list_add_tail(&p_frame_node->list, &p_venc_data->in_empty_frame);
            pthread_mutex_unlock(&p_venc_data->in_frame_lock);
        }

        /*sleep wait for put frame*/
        if (mm_venc_list_empty(&p_venc_data->in_ready_frame, p_venc_data->in_frame_lock)) {
            aic_msg_wait_new_msg(&p_venc_data->msg, 0);
            continue;
        }

        while (!mm_venc_list_empty(&p_venc_data->in_ready_frame, p_venc_data->in_frame_lock)) {
            if (mm_venc_list_empty(&p_venc_data->out_empty_pkt, p_venc_data->out_pkt_lock)) {
                mm_send_command(p_venc_data->out_port_bind.p_bind_comp, MM_COMMAND_NOPS, 0, NULL);
                aic_msg_wait_new_msg(&p_venc_data->msg, 0);
            }
            p_frame_node = mpp_list_first_entry(&p_venc_data->in_ready_frame, mm_venc_in_frame, list);
            p_pkt_node = mpp_list_first_entry(&p_venc_data->out_empty_pkt, mm_venc_out_packet, list);
            if (p_pkt_node->phy_addr == 0) {
                logw("Empty packet, then drop one frame!!!");
                venc_buffer.p_buffer = (u8 *)&p_frame_node->frame;
                ret = mm_giveback_buffer(p_venc_data->in_port_bind.p_bind_comp, &venc_buffer);
                if (ret != 0) {
                    logw("give back frame to vin fail\n");
                    p_venc_data->giveback_frame_fail_num++;
                } else {
                    p_venc_data->giveback_frame_ok_num++;
                }
                pthread_mutex_lock(&p_venc_data->in_frame_lock);
                mpp_list_del(&p_frame_node->list);
                mpp_list_add_tail(&p_frame_node->list, &p_venc_data->in_empty_frame);
                pthread_mutex_unlock(&p_venc_data->in_frame_lock);
                mm_send_command(p_venc_data->out_port_bind.p_bind_comp, MM_COMMAND_NOPS, 0, NULL);
                aic_msg_wait_new_msg(&p_venc_data->msg, 0);
                goto _AIC_MSG_GET_;
            }

            /*do jpeg encode*/
            memset(&packet, 0, sizeof(struct mpp_packet));
            packet.phy_addr = p_pkt_node->phy_addr;
            packet.size = p_pkt_node->size;
            if (mpp_encoder_encode(p_venc_data->p_encoder, &p_frame_node->frame, &packet) < 0) {
                loge("encode failed");
                goto _AIC_MSG_GET_;
            }

            p_venc_data->encoder_frame_ok_num++;
            p_pkt_node->packet.size = packet.len;
            p_pkt_node->packet.pts = p_frame_node->frame.pts;
            p_pkt_node->packet.dts = p_frame_node->frame.pts;
            p_pkt_node->packet.data = (void *)(unsigned long)p_pkt_node->phy_addr;
            venc_buffer.p_buffer = (u8 *)&p_pkt_node->packet;
            venc_buffer.output_port_index = MUX_PORT_VIDEO_INDEX;
            venc_buffer.input_port_index = p_venc_data->out_port_bind.port_index;

            /*do jpeg snapshot*/
            mm_venc_component_process_snapshot(p_venc_data, p_pkt_node);

            ret = mm_send_buffer(p_venc_data->out_port_bind.p_bind_comp, &venc_buffer);
            if (ret != MM_ERROR_NONE) {
                logw("mm_send_buffer error %d", ret);
                p_venc_data->send_pkt_fail_num++;
                goto _AIC_MSG_GET_;
            }
            p_venc_data->send_pkt_ok_num++;
            /*push in rame to processed list*/
            pthread_mutex_lock(&p_venc_data->in_frame_lock);
            mpp_list_del(&p_frame_node->list);
            mpp_list_add_tail(&p_frame_node->list, &p_venc_data->in_processed_frame);
            pthread_mutex_unlock(&p_venc_data->in_frame_lock);

            /*push out pkt to processing list*/
            pthread_mutex_lock(&p_venc_data->out_pkt_lock);
            mpp_list_del(&p_pkt_node->list);
            mpp_list_add_tail(&p_pkt_node->list, &p_venc_data->out_processing_pkt);
            pthread_mutex_unlock(&p_venc_data->out_pkt_lock);
        }
    }
_EXIT:
    mm_venc_component_count_print(p_venc_data);

    printf("mm_venc_component_thread exit\n");

    return (void *)MM_ERROR_NONE;
}
