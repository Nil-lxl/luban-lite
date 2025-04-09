/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <jun.ma@artinchip.com>
 * Desc: aic audio render device
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <rtthread.h>
#include <rtdevice.h>

#include "artinchip_fb.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "aic_audio_render_device.h"

#if defined(AIC_USING_I2S0)
#define SOUND_DEVICE_NAME    "i2s0_sound"
#elif  defined(AIC_USING_I2S1)
#define SOUND_DEVICE_NAME    "i2s1_sound"
#else
#define SOUND_DEVICE_NAME    "sound0"
#endif

#define AIC_AUDIO_STATUS_PLAY 0
#define AIC_AUDIO_STATUS_PAUSE 1
#define AIC_AUDIO_STATUS_STOP 2


struct aic_rt_audio_render{
    struct aic_audio_render_dev base;
    rt_device_t snd_dev;
    struct aic_audio_render_attr attr;
    int status;
};

s32 rt_audio_render_init(struct aic_audio_render_dev *render,s32 dev_id)
{
    long ret;
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;

    rt_render->snd_dev = rt_device_find(SOUND_DEVICE_NAME);
    if (!rt_render->snd_dev) {
        loge("rt_device_find error!\n");
        return -1;
    }
    ret = rt_device_open(rt_render->snd_dev, RT_DEVICE_OFLAG_WRONLY);
    if (ret != 0) {
        loge("rt_device_open error!\n");
        return -1;
    }
    rt_render->status = AIC_AUDIO_STATUS_PLAY;
    return 0;
}

s32 rt_audio_render_destroy(struct aic_audio_render_dev *render)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;

    if (!rt_render || !rt_render->snd_dev)
        return -1;

    rt_device_close(rt_render->snd_dev);
    mpp_free(rt_render);

    return 0;
}


s32 rt_audio_render_set_attr(struct aic_audio_render_dev *render,struct aic_audio_render_attr *attr)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    struct rt_audio_caps caps = {0};
    int stream, reinit_flag = 0, channels = 0, transfer_mode = 0;

    if (!rt_render || !rt_render->snd_dev)
        return -1;

    /*Get current config and check if need do set config again*/
    caps.main_type               = AUDIO_TYPE_OUTPUT;
    caps.sub_type                = AUDIO_DSP_PARAM;
    if (rt_device_control(rt_render->snd_dev, AUDIO_CTL_GETCAPS, &caps) != RT_EOK) {
        loge("AUDIO_CTL_GETCAPS failed!\n");
        return -1;
    }
    if (caps.udata.config.samplerate == attr->sample_rate &&
        caps.udata.config.channels  == attr->channels) {
         return 0;
    }
    channels = caps.udata.config.channels;

    caps.main_type               = AUDIO_TYPE_MIXER;
    caps.sub_type                = AUDIO_MIXER_EXTEND;
    if (rt_device_control(rt_render->snd_dev, AUDIO_CTL_GETCAPS, &caps) != RT_EOK) {
        loge("AUDIO_CTL_GETCAPS failed!\n");
        return -1;
    }
    transfer_mode = caps.udata.value;

    /*Channels changed need to re initial snd device*/
    if (channels != attr->channels || transfer_mode != attr->transfer_mode) {
        rt_device_close(rt_render->snd_dev);
        if (rt_device_open(rt_render->snd_dev, RT_DEVICE_OFLAG_WRONLY) != 0) {
            loge("rt_device_open error!\n");
            return -1;
        }
        reinit_flag = 1;
        rt_thread_mdelay(50);
        printf("recreate snd dev channels %d, transfer_mode %d\n!",
               attr->channels, attr->transfer_mode);
    }

    caps.main_type               = AUDIO_TYPE_OUTPUT;
    caps.sub_type                = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = attr->sample_rate;
    caps.udata.config.channels   = attr->channels;
    caps.udata.config.samplebits = 16;
    rt_device_control(rt_render->snd_dev, AUDIO_CTL_CONFIGURE, &caps);

    caps.main_type               = AUDIO_TYPE_MIXER;
    caps.sub_type                = AUDIO_MIXER_EXTEND;
    caps.udata.value = attr->transfer_mode;
    rt_device_control(rt_render->snd_dev, AUDIO_CTL_CONFIGURE, &caps);

    stream = AUDIO_STREAM_REPLAY;
    rt_device_control(rt_render->snd_dev, AUDIO_CTL_START, (void *)&stream);
    rt_thread_mdelay(200);

    if (reinit_flag) {
        rt_render->status = AIC_AUDIO_STATUS_PLAY;
    }

    return 0;
}


s32 rt_audio_render_get_attr(struct aic_audio_render_dev *render,struct aic_audio_render_attr *attr)
{
    s32 ret = 0;
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    struct rt_audio_caps caps = {0};

    if (!rt_render || !rt_render->snd_dev)
        return -1;

    caps.main_type               = AUDIO_TYPE_OUTPUT;
    caps.sub_type                = AUDIO_DSP_PARAM;
    ret = rt_device_control(rt_render->snd_dev, AUDIO_CTL_GETCAPS, &caps);
    if (ret != RT_EOK) {
        loge("AUDIO_CTL_GETCAPS failed:%d!\n", ret);
        return -1;
    }
    attr->sample_rate = caps.udata.config.samplerate;
    attr->channels = caps.udata.config.channels;

    return 0;
}

#ifdef RT_AUDIO_REPLAY_MP_BLOCK_SIZE
    #define BLOCK_SIZE (RT_AUDIO_REPLAY_MP_BLOCK_SIZE/2)
#else
    #define BLOCK_SIZE (2*1024)
#endif

s32 rt_audio_render_rend(struct aic_audio_render_dev *render, void* pData, s32 nDataSize)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    s32 w_len;
    s32 count = 0;
    s32 pos = 0;

    if (!rt_render || !rt_render->snd_dev)
        return -1;

    count = nDataSize;
    while(count > 0) {
        w_len = rt_device_write(rt_render->snd_dev,0, pData+pos,(count > BLOCK_SIZE)?(BLOCK_SIZE):(count));
        if (w_len <= 0) {
            loge("rt_device_write w_len:%d,nDataSize:%d!\n",w_len,nDataSize);
            return -1;
        }
        count -= w_len;
        pos += w_len;
    }

    return 0;
}

s32 rt_audio_render_sync_rend(struct aic_audio_render_dev *render, void* data, s32 size)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;

    if (!rt_render || !rt_render->snd_dev)
        return -1;

    /*sync transfer data to codec*/
    rt_device_control(rt_render->snd_dev, AUDIO_CTL_DIRECT_TRANSFER, NULL);
    return 0;
}

s64 rt_audio_render_get_cached_time(struct aic_audio_render_dev *render)
{
    uint32_t  delay = 0;
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;

    if (!rt_render || !rt_render->snd_dev)
        return -1;

    rt_device_control(rt_render->snd_dev, AUDIO_CTL_GETAVAIL, &delay);
    return delay;
}

s32 rt_audio_render_pause(struct aic_audio_render_dev *render)
{
    int pause;
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;

    if (!rt_render || !rt_render->snd_dev)
        return -1;

    if (rt_render->status == AIC_AUDIO_STATUS_PLAY) {
        pause = 1;
        rt_device_control(rt_render->snd_dev, AUDIO_CTL_PAUSE, &pause);
        rt_render->status = AIC_AUDIO_STATUS_PAUSE;
    } else if (rt_render->status == AIC_AUDIO_STATUS_PAUSE) {
        pause = 0;
        rt_device_control(rt_render->snd_dev, AUDIO_CTL_PAUSE, &pause);
        rt_render->status = AIC_AUDIO_STATUS_PLAY;
    } else {
        return -1;
    }
    return 0;
}

s32 rt_audio_render_get_volume(struct aic_audio_render_dev *render)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    struct rt_audio_caps caps = {0};
    s32 vol = 0;

    if (!rt_render || !rt_render->snd_dev)
        return -1;

    caps.main_type               = AUDIO_TYPE_MIXER;
    caps.sub_type                = AUDIO_MIXER_VOLUME;
    caps.udata.value = 0;
    rt_device_control(rt_render->snd_dev, AUDIO_CTL_GETCAPS, &caps);
    vol = caps.udata.value;
    return vol;
}

s32 rt_audio_render_set_volume(struct aic_audio_render_dev *render,s32 vol)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    struct rt_audio_caps caps = {0};

    if (!rt_render || !rt_render->snd_dev)
        return -1;

    caps.main_type               = AUDIO_TYPE_MIXER;
    caps.sub_type                = AUDIO_MIXER_VOLUME;
    caps.udata.value = vol;
    rt_device_control(rt_render->snd_dev, AUDIO_CTL_CONFIGURE, &caps);
    return 0;
}

s32 rt_audio_render_clear_cache(struct aic_audio_render_dev *render)
{
	return 0;
}


s32 rt_audio_render_get_transfer_buffer(struct aic_audio_render_dev *render,
                                        struct aic_audio_render_transfer_buffer *transfer_buffer)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    if (transfer_buffer == NULL || rt_render == NULL)
        return -1;

    struct rt_audio_device *audio_dev = (struct rt_audio_device *)rt_render->snd_dev;
    if (audio_dev == NULL || audio_dev->replay == NULL)
        return -1;

    transfer_buffer->buffer = audio_dev->replay->buf_info.buffer;
    transfer_buffer->buffer_len = audio_dev->replay->buf_info.block_size;

    return 0;
}


s32 aic_audio_render_dev_create(struct aic_audio_render_dev **render)
{
    struct aic_rt_audio_render * rt_render;
    rt_render = mpp_alloc(sizeof(struct aic_rt_audio_render));
    if (rt_render == NULL) {
        loge("mpp_alloc alsa_render fail!!!\n");
        *render = NULL;
        return -1;
    }
    rt_render->status = AIC_AUDIO_STATUS_STOP;
    rt_render->base.init = rt_audio_render_init;
    rt_render->base.destroy = rt_audio_render_destroy;
    rt_render->base.set_attr = rt_audio_render_set_attr;
    rt_render->base.get_attr = rt_audio_render_get_attr;
    rt_render->base.rend = rt_audio_render_rend;
    rt_render->base.sync_rend = rt_audio_render_sync_rend;
    rt_render->base.get_cached_time = rt_audio_render_get_cached_time;
    rt_render->base.pause = rt_audio_render_pause;
    rt_render->base.set_volume = rt_audio_render_set_volume;
    rt_render->base.get_volume = rt_audio_render_get_volume;
    rt_render->base.clear_cache = rt_audio_render_clear_cache;
    rt_render->base.get_transfer_buffer = rt_audio_render_get_transfer_buffer;
    *render = &rt_render->base;
    return 0;
}

