/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: aic audio render manager
 */

#ifndef __AIC_AUDIO_RENDER_H__
#define __AIC_AUDIO_RENDER_H__

#include "mpp_dec_type.h"
#include "aic_audio_render_device.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AUDIO_RENDER_SCENE_LOWEST_PRIORITY 1
#define AUDIO_RENDER_SCENE_DEFAULT_PRIORITY 5
#define AUDIO_RENDER_SCENE_HIGHEST_PRIORITY 10

enum audio_render_cmd_type {
    AUDIO_RENDER_CMD_SET_ATTR,
    AUDIO_RENDER_CMD_GET_ATTR,
    AUDIO_RENDER_CMD_SET_VOL,
    AUDIO_RENDER_CMD_GET_VOL,
    AUDIO_RENDER_CMD_PAUSE,
    AUDIO_RENDER_CMD_GET_CACHE_TIME,
    AUDIO_RENDER_CMD_CLEAR_CACHE,
    AUDIO_RENDER_CMD_SET_SCENE_PRIORITY,
    AUDIO_RENDER_CMD_GET_SCENE_PRIORITY,
    AUDIO_RENDER_CMD_SET_MIXING,
    AUDIO_RENDER_CMD_GET_TRANSFER_BUFFER,
    AUDIO_RENDER_CMD_GET_BYPASS,
    AUDIO_RENDER_CMD_SET_EVENT_HANDLER,
};


enum audio_render_scene_type {
    AUDIO_RENDER_SCENE_INVALID = 0,
    /*Local Media File*/
    AUDIO_RENDER_SCENE_DEFAULT,
    /*Warning Tone Audio*/
    AUDIO_RENDER_SCENE_WARNING_TONE,
    /*USB Audio Class*/
    AUDIO_RENDER_SCENE_UAC,
    AUDIO_RENDER_SCENE_MAX,
};

enum audio_render_event_type {
    AUDIO_RENDER_EVENT_START = 0,
    AUDIO_RENDER_EVENT_STOP,
    AUDIO_RENDER_EVENT_RECONFIG,
    AUDIO_RENDER_EVENT_MAX,
};


struct audio_render_create_params {
    int dev_id;
    enum audio_render_scene_type scene_type;
};

struct aic_audio_render {
    s32 (*init)(struct aic_audio_render *render);

    s32 (*destroy)(struct aic_audio_render *render);

    s32 (*rend)(struct aic_audio_render *render, void *data, s32 size);

    s32 (*control)(struct aic_audio_render *render, enum audio_render_cmd_type cmd, void *params);
};

#define aic_audio_render_init(render) \
    ((struct aic_audio_render *)render)->init(render)

#define aic_audio_render_destroy(render) \
    ((struct aic_audio_render *)render)->destroy(render)

#define aic_audio_render_rend(render, data, size) \
    ((struct aic_audio_render *)render)->rend(render, data, size)

#define aic_audio_render_control(render, cmd, params) \
    ((struct aic_audio_render *)render)->control(render, cmd, params)

s32 aic_audio_render_create(struct aic_audio_render **render, struct audio_render_create_params *create_params);

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif
