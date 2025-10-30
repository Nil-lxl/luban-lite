/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <jun.ma@artinchip.com>
 * Desc: aic audio render device
 */

#ifndef __AIC_AUDIO_RENDER_DEVICE_H__
#define __AIC_AUDIO_RENDER_DEVICE_H__

#include "mpp_dec_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AIC_AUDIO_RENDER_DEVICE_NUM 1

struct aic_audio_render_attr {
    s32 channels;
    s32 sample_rate;
    s32 bits_per_sample;
    s32 smples_per_frame;
    s32 transfer_mode;
};

struct aic_audio_render_transfer_buffer {
    void *buffer;
    s32 buffer_len;
};

struct aic_audio_render_dev {
    s32 (*init)(struct aic_audio_render_dev *render, s32 dev_id);

    s32 (*destroy)(struct aic_audio_render_dev *render);

    s32 (*set_attr)(struct aic_audio_render_dev *render, struct aic_audio_render_attr *attr);

    s32 (*get_attr)(struct aic_audio_render_dev *render, struct aic_audio_render_attr *attr);

    s32 (*rend)(struct aic_audio_render_dev *render, void *pData, s32 nDataSize);
    s32 (*sync_rend)(struct aic_audio_render_dev *render, void *pData, s32 nDataSize);

    s32 (*pause)(struct aic_audio_render_dev *render);

    s64 (*get_cached_time)(struct aic_audio_render_dev *render);

    s32 (*get_volume)(struct aic_audio_render_dev *render);

    s32 (*set_volume)(struct aic_audio_render_dev *render, s32 vol);

    s32 (*clear_cache)(struct aic_audio_render_dev *render);

    s32 (*get_transfer_buffer)(struct aic_audio_render_dev *render,
                               struct aic_audio_render_transfer_buffer *transfer_buffer);
};

#define aic_audio_render_dev_init(render, dev_id) \
    ((struct aic_audio_render_dev *)render)->init(render, dev_id)

#define aic_audio_render_dev_destroy(render) \
    ((struct aic_audio_render_dev *)render)->destroy(render)

#define aic_audio_render_dev_set_attr(render, attr) \
    ((struct aic_audio_render_dev *)render)->set_attr(render, attr)

#define aic_audio_render_dev_get_attr(render, attr) \
    ((struct aic_audio_render_dev *)render)->get_attr(render, attr)

#define aic_audio_render_dev_rend(render, pData, nDataSize) \
    ((struct aic_audio_render_dev *)render)->rend(render, pData, nDataSize)

#define aic_audio_render_dev_sync_rend(render, pData, nDataSize) \
    ((struct aic_audio_render_dev *)render)->sync_rend(render, pData, nDataSize)

#define aic_audio_render_dev_get_cached_time(render) \
    ((struct aic_audio_render_dev *)render)->get_cached_time(render)

#define aic_audio_render_dev_pause(render) \
    ((struct aic_audio_render_dev *)render)->pause(render)

#define aic_audio_render_dev_set_volume(render, vol) \
    ((struct aic_audio_render_dev *)render)->set_volume(render, vol)

#define aic_audio_render_dev_get_volume(render) \
    ((struct aic_audio_render_dev *)render)->get_volume(render)

#define aic_audio_render_dev_clear_cache(render) \
    ((struct aic_audio_render_dev *)render)->clear_cache(render)

#define aic_audio_render_dev_get_transfer_buffer(render, transfer_buffer) \
    ((struct aic_audio_render_dev *)render)->get_transfer_buffer(render, transfer_buffer)

s32 aic_audio_render_dev_create(struct aic_audio_render_dev **render);

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif
