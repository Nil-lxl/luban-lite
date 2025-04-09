/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: aic audio render manager
 */

#include "aic_audio_render_manager.h"
#include "aic_audio_render_device.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

struct aic_audio_render_scene {
    enum audio_render_scene_type scene_type;
    int priority;
    int flag;
};

struct aic_audio_render_dev_manager {
    struct aic_audio_render_dev *render_dev;
    struct aic_audio_render_scene render_scene[AUDIO_RENDER_SCENE_MAX];
    int dev_id;
    int dev_ref_cnt;
    pthread_mutex_t dev_lock;
};

struct aic_audio_render_manager {
    struct aic_audio_render base;
    struct aic_audio_render_dev *render_dev;
    enum audio_render_scene_type scene_type;
    struct aic_audio_render_attr attr;
    int dev_id;
    int scene_id;
    void (*event_handler)(int event);
};

struct aic_audio_render_dev_manager g_audio_render_device[AIC_AUDIO_RENDER_DEVICE_NUM] = {0};

s32 aic_audio_render_manager_init(struct aic_audio_render *render)
{
    struct aic_audio_render_manager *audio_render = (struct aic_audio_render_manager *)render;

    if (!audio_render || !audio_render->render_dev) {
        loge("audio_render(%p) or render_dev is null", audio_render);
        return -1;
    }
    /*device has been initialed ,should be return */
    if (audio_render->scene_id == 0) {
        if (aic_audio_render_dev_init(audio_render->render_dev, audio_render->dev_id)) {
            loge("aic_audio_render_dev_init failed");
            return -1;
        }
    }

    return 0;
}

s32 aic_audio_render_manager_destroy(struct aic_audio_render *render)
{
    if (!render)
        return -1;

    struct aic_audio_render_manager *audio_render =
        (struct aic_audio_render_manager *)render;
    struct aic_audio_render_dev_manager *render_device =
        &g_audio_render_device[audio_render->dev_id];
    struct aic_audio_render_scene *render_scene =
        &render_device->render_scene[audio_render->scene_id];

    render_scene->flag = 0;
    render_scene->scene_type = AUDIO_RENDER_SCENE_INVALID;
    render_scene->priority = AUDIO_RENDER_SCENE_LOWEST_PRIORITY - 1;
    render_device->dev_ref_cnt--;

    printf("[%s]dev_ref_cnt %d, scene_type %d, scene_id %d success.\n", __func__,
           render_device->dev_ref_cnt, audio_render->scene_type, audio_render->scene_id);

    if (render_device->dev_ref_cnt == 0) {
        pthread_mutex_destroy(&render_device->dev_lock);
        aic_audio_render_dev_destroy(audio_render->render_dev);
        audio_render->render_dev = NULL;
        g_audio_render_device[audio_render->dev_id].render_dev = NULL;
    }
    mpp_free(audio_render);

    return 0;
}

s32 aic_audio_render_manager_check_bypass(struct aic_audio_render *render)
{
    if (!render)
        return -1;

    int i = 0, highest_priority = 0;
    struct aic_audio_render_manager *audio_render = (struct aic_audio_render_manager *)render;
    struct aic_audio_render_dev_manager *render_device = &g_audio_render_device[audio_render->dev_id];
    struct aic_audio_render_scene *render_scene = NULL;

    /*Find the highest priority scene */
    highest_priority = AUDIO_RENDER_SCENE_LOWEST_PRIORITY - 1;
    for (i = 0; i < AUDIO_RENDER_SCENE_MAX; i++) {
        if (render_device->render_scene[i].scene_type == AUDIO_RENDER_SCENE_INVALID ||
            render_device->render_scene[i].flag == 0) {
            continue;
        }
        if (highest_priority < render_device->render_scene[i].priority) {
            render_scene = &render_device->render_scene[i];
        }
    }
    if (!render_scene) {
        return 1;
    }

    if (render_scene->scene_type != audio_render->scene_type) {
        return 1;
    }

    return 0;
}

s32 aic_audio_render_manager_control(struct aic_audio_render *render,
                                     enum audio_render_cmd_type cmd, void *params)
{
    if (!render)
        return -1;

    int ret = 0, priority = 0;
    struct aic_audio_render_manager *audio_render = (struct aic_audio_render_manager *)render;
    struct aic_audio_render_dev_manager *render_device = &g_audio_render_device[audio_render->dev_id];

    switch (cmd) {
    case AUDIO_RENDER_CMD_SET_ATTR:
        pthread_mutex_lock(&render_device->dev_lock);
        if (audio_render->scene_type == AUDIO_RENDER_SCENE_UAC)
            ((struct aic_audio_render_attr *)params)->transfer_mode = 1;
        else
            ((struct aic_audio_render_attr *)params)->transfer_mode = 0;
        ret = aic_audio_render_dev_set_attr(audio_render->render_dev,
                                            (struct aic_audio_render_attr *)params);
        memcpy(&audio_render->attr, params, sizeof(struct aic_audio_render_attr));
        pthread_mutex_unlock(&render_device->dev_lock);
        break;
    case AUDIO_RENDER_CMD_GET_ATTR:
        ret = aic_audio_render_dev_get_attr(audio_render->render_dev,
                                            (struct aic_audio_render_attr *)params);
        break;
    case AUDIO_RENDER_CMD_SET_VOL:
        ret = aic_audio_render_dev_set_volume(audio_render->render_dev, *(s32 *)params);
        break;
    case AUDIO_RENDER_CMD_GET_VOL:
        *(s32 *)params = aic_audio_render_dev_get_volume(audio_render->render_dev);
        break;
    case AUDIO_RENDER_CMD_PAUSE:
        ret = aic_audio_render_dev_pause(audio_render->render_dev);
        break;
    case AUDIO_RENDER_CMD_GET_CACHE_TIME:
        *(s64 *)params = aic_audio_render_dev_get_cached_time(audio_render->render_dev);
        break;
    case AUDIO_RENDER_CMD_CLEAR_CACHE:
        ret = aic_audio_render_dev_clear_cache(audio_render->render_dev);
        break;
    case AUDIO_RENDER_CMD_SET_SCENE_PRIORITY:
        priority = *(s32 *)params;
        if (priority < AUDIO_RENDER_SCENE_LOWEST_PRIORITY ||
            priority > AUDIO_RENDER_SCENE_HIGHEST_PRIORITY) {
            loge("the priority %d is illegal", priority);
            return -1;
        }
        render_device->render_scene[audio_render->scene_id].priority = priority;
        break;
    case AUDIO_RENDER_CMD_SET_MIXING:
        break;
    case AUDIO_RENDER_CMD_GET_TRANSFER_BUFFER:
        ret = aic_audio_render_dev_get_transfer_buffer(audio_render->render_dev,
                                                       (struct aic_audio_render_transfer_buffer *)params);
        break;
    case AUDIO_RENDER_CMD_GET_BYPASS:
        ret = aic_audio_render_manager_check_bypass(render);
        break;
    case AUDIO_RENDER_CMD_SET_EVENT_HANDLER:
        if (params) {
            audio_render->event_handler = params;
        } else {
            loge("set event_handler is null");
            return -1;
        }
        break;

    default:
        loge("unsupport audio render control cmd %d", cmd);
        return -1;
    }

    return ret;
}

s32 aic_audio_render_manager_rend(struct aic_audio_render *render, void *data, s32 size)
{
    if (!render || !data)
        return -1;

    int ret = 0;
    static enum audio_render_scene_type last_scene_type = 0;
    struct aic_audio_render_manager *audio_render = (struct aic_audio_render_manager *)render;
    struct aic_audio_render_dev_manager *render_device = &g_audio_render_device[audio_render->dev_id];

    ret = aic_audio_render_manager_check_bypass(render);
    if(ret != 0) {
        /*Bypass audio render means drop the data and return success*/
        return 0;
    }

    if (audio_render->scene_type == AUDIO_RENDER_SCENE_UAC) {
        if (last_scene_type != AUDIO_RENDER_SCENE_UAC) {
            if (audio_render->event_handler)
                audio_render->event_handler(AUDIO_RENDER_EVENT_RECONFIG);
        } else {
            ret = aic_audio_render_dev_sync_rend(audio_render->render_dev, data, size);
        }
    } else {
        if (last_scene_type == AUDIO_RENDER_SCENE_UAC) {
            aic_audio_render_manager_control(render, AUDIO_RENDER_CMD_SET_ATTR, &audio_render->attr);
        }
        pthread_mutex_lock(&render_device->dev_lock);
        ret = aic_audio_render_dev_rend(audio_render->render_dev, data, size);
        pthread_mutex_unlock(&render_device->dev_lock);
    }
    last_scene_type = audio_render->scene_type;
    return ret;
}

s32 aic_audio_render_create(struct aic_audio_render **render,
                            struct audio_render_create_params *create_params)
{
    struct aic_audio_render_manager *audio_render = NULL;
    struct aic_audio_render_dev_manager *render_device = NULL;
    if (!create_params) {
        loge("audio render create_params is null\n");
        *render = NULL;
        return -1;
    }

    audio_render = mpp_alloc(sizeof(struct aic_audio_render_manager));
    if (!audio_render) {
        loge("mpp_alloc audio_render fail!!!\n");
        *render = NULL;
        return -1;
    }
    memset(audio_render, 0, sizeof(struct aic_audio_render_manager));
    if (create_params->dev_id >= AIC_AUDIO_RENDER_DEVICE_NUM) {
        loge("dev_id %d is exceed the max audio devices num", create_params->dev_id);
        goto exit;
    }
    render_device = &g_audio_render_device[audio_render->dev_id];
    audio_render->scene_type = create_params->scene_type;
    audio_render->dev_id = create_params->dev_id;
    audio_render->scene_id = render_device->dev_ref_cnt;

    if (!render_device->render_dev) {
        if (aic_audio_render_dev_create(&render_device->render_dev)) {
            loge("aic_audio_render_dev_create failed");
            goto exit;
        }
        if (pthread_mutex_init(&render_device->dev_lock, NULL)) {
            loge("pthread_mutex_init dev_lock fail!\n");
            goto exit;
        }
    }

    audio_render->render_dev = render_device->render_dev;
    render_device->render_scene[audio_render->scene_id].scene_type = audio_render->scene_type;
    render_device->render_scene[audio_render->scene_id].priority =
        AUDIO_RENDER_SCENE_DEFAULT_PRIORITY;
    render_device->render_scene[audio_render->scene_id].flag = 1;
    render_device->dev_ref_cnt++;

    audio_render->base.init = aic_audio_render_manager_init;
    audio_render->base.destroy = aic_audio_render_manager_destroy;
    audio_render->base.control = aic_audio_render_manager_control;
    audio_render->base.rend = aic_audio_render_manager_rend;
    *render = &audio_render->base;

    printf("[%s]dev_ref_cnt %d, scene_type %d, scene_id %d success.\n", __func__,
           render_device->dev_ref_cnt, audio_render->scene_type, audio_render->scene_id);

    return 0;

exit:
    if (audio_render)
        mpp_free(audio_render);
    *render = NULL;

    return -1;
}
