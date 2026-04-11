/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <xiaodong.zhao@artinchip.com>
 * Desc: aic audio mix
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <rtthread.h>
#include <rtdevice.h>

#include "aic_mix.h"

#include "artinchip_fb.h"
#include "aic_audio_render_manager.h"
#include "aic_audio_render_device.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mm_core.h"
#include "aic_core.h"
#include "aic_osal.h"

#define MIX_SUM_BUF_SIZE (4096)      // 2048 samples
#define MIX_RESAMPLE_BUF_SIZE (16384)

struct aic_pcm_mix_manager *g_mix_manager = NULL;

static inline unsigned long buf_readable(struct aic_pcm_mix *mix)
{
    if (mix->wt >= mix->rd) {
        return mix->wt - mix->rd;
    } else {
        return mix->size - mix->rd + mix->wt;
    }
}

static inline unsigned long buf_writable(struct aic_pcm_mix *mix)
{
    return mix->size - buf_readable(mix);
}

// Audio mixing function
static void generic_mix_areas_16_native(unsigned int size,
                                        volatile int16_t *dst,
                                        int16_t *src,
                                        volatile int32_t *sum,
                                        size_t dst_step,
                                        size_t src_step,
                                        size_t sum_step)
{
    register int32_t sample;
    for (;;) {
        sample = *src;
        if (!*dst) {
            // Initialize accumulator and destination
            *sum = sample;
            *dst = *src;
        } else {
            // Mix samples with clipping protection
            sample += *sum;
            *sum = sample;
            if (sample > 0x7fff)
                sample = 0x7fff;
            else if (sample < -0x8000)
                sample = -0x8000;
            *dst = sample;
        }
        if (!--size)
            return;
        // Move pointers using byte-based stepping
        src = (int16_t *)((char *)src + src_step);
        dst = (int16_t *)((char *)dst + dst_step);
        sum = (int32_t *)((char *)sum + sum_step);
    }
}

struct aic_pcm_mix_manager *aic_pcm_mix_manager_create(void)
{
    struct aic_pcm_mix_manager *mix_manager;

    mix_manager = malloc(sizeof(struct aic_pcm_mix_manager));
    if (NULL == mix_manager) {
        loge("mix_manager malloc fail!\n");
        return NULL;
    }
    memset(mix_manager, 0, sizeof(struct aic_pcm_mix_manager));

    if (pthread_mutex_init(&mix_manager->mutex, NULL)) {
        loge("pthread_mutex_init mix mutex fail!\n");
        goto mutex_err;
    }

    mix_manager->mix_buf = malloc(MIX_SUM_BUF_SIZE * sizeof(int16_t));
    if (NULL == mix_manager->mix_buf) {
        loge("mix_manager->mix_buf malloc fail!\n");
        goto mix_buf_err;
    }

    mix_manager->sum_buf = malloc(MIX_SUM_BUF_SIZE * sizeof(int32_t));
    if (NULL == mix_manager->sum_buf) {
        loge("mix_manager->sum_buf malloc fail!\n");
        goto sum_buf_err;
    }

    mix_manager->size = MIX_SUM_BUF_SIZE;
    mix_manager->mix = NULL;
    mix_manager->mix_cnt = 0;

    mix_manager->resample_buf = malloc(MIX_RESAMPLE_BUF_SIZE);
    if (NULL == mix_manager->resample_buf) {
        loge("malloc resample_buf fail!\n");
        goto resample_buf_err;
    }
    mix_manager->resample_buf_size = MIX_RESAMPLE_BUF_SIZE;

    return mix_manager;

resample_buf_err:
    free(mix_manager->sum_buf);
sum_buf_err:
    free(mix_manager->mix_buf);
mix_buf_err:
    pthread_mutex_destroy(&mix_manager->mutex);
mutex_err:
    free(mix_manager);
    return NULL;
}

int aic_pcm_mix_manager_destroy(struct aic_pcm_mix_manager *mix_manager)
{
    if (NULL == mix_manager) {
        loge("<%s:%d>invalid parameter!\n", __func__, __LINE__);
        return -1;
    }

    if ((NULL != mix_manager->mix) || (0 != mix_manager->mix_cnt)) {
        return -1;
    }

    pthread_mutex_destroy(&mix_manager->mutex);

    if (mix_manager->mix_buf) {
        free(mix_manager->mix_buf);
        mix_manager->mix_buf = NULL;
    }

    if (mix_manager->sum_buf) {
        free(mix_manager->sum_buf);
        mix_manager->sum_buf = NULL;
    }

    if (mix_manager->resample_buf) {
        free(mix_manager->resample_buf);
        mix_manager->resample_buf = NULL;
    }

    if (mix_manager == g_mix_manager) {
        g_mix_manager = NULL;
    }
    free(mix_manager);

    return 0;
}

static int insert_mix_to_manager(struct aic_pcm_mix_manager *mix_manager, struct aic_pcm_mix *mix)
{
    if ((NULL == mix_manager) || (NULL == mix)) {
        loge("<%s:%d>invalid parameter!\n", __func__, __LINE__);
        return -1;
    }

    pthread_mutex_lock(&mix_manager->mutex);

    if (NULL == mix_manager->mix) {
        mix->prev = mix->next = mix;
        mix_manager->mix = mix;
    } else {
        struct aic_pcm_mix *tail = mix_manager->mix->prev;
        mix->next = mix_manager->mix;
        mix_manager->mix->prev = mix;
        mix->prev = tail;
        tail->next = mix;
    }

    mix->mix_manager = mix_manager;
    mix_manager->mix_cnt++;

    pthread_mutex_unlock(&mix_manager->mutex);

    return 0;
}

static int pop_mix_from_manager(struct aic_pcm_mix_manager *mix_manager, struct aic_pcm_mix *mix)
{
    struct aic_pcm_mix *tmp;

    if ((NULL == mix_manager) || (NULL == mix)) {
        loge("<%s:%d>invalid parameter!\n", __func__, __LINE__);
        return -1;
    }

    if (NULL == mix_manager->mix) {
        return 0;
    }

    pthread_mutex_lock(&mix_manager->mutex);

    if (mix_manager->mix == mix_manager->mix->next)
    {
        if (mix_manager->mix == mix) {
            mix_manager->mix = NULL;
            mix->prev = NULL;
            mix->next = NULL;
            mix_manager->mix_cnt--;
        } else {
            loge("<%s:%d> node:%p id:%d not found!\n", __func__, __LINE__, mix, mix->id);
        }
    } else {
        tmp = mix_manager->mix;
        do {
            if (tmp == mix) {
                struct aic_pcm_mix *prev = tmp->prev;
                struct aic_pcm_mix *next = tmp->next;
                if (prev && next) {
                    prev->next = next;
                    next->prev = prev;

                    // mix is header
                    if (mix == mix_manager->mix) {
                        mix_manager->mix = mix->next;
                    }

                    mix->prev = NULL;
                    mix->next = NULL;
                    mix_manager->mix_cnt--;
                } else {
                    loge("<%s:%d> corrupted list: NULL pointer detected!\n", __func__, __LINE__);
                }
                break;
            }
            tmp = tmp->next;
        } while (tmp != mix_manager->mix);
    }

    pthread_mutex_unlock(&mix_manager->mutex);

    return 0;
}

struct aic_pcm_mix *aic_pcm_mix_create(struct aic_pcm_mix_manager *mix_manager, int id, int size)
{
    struct aic_pcm_mix *mix = NULL;

    if (size <= 0) {
        loge("invalid parameter [%d]\n", size);
        return NULL;
    }

    if (NULL == g_mix_manager) {
        g_mix_manager = aic_pcm_mix_manager_create();
        if (NULL == g_mix_manager) {
            loge("create mix_manager fail!\n");
            return NULL;
        }
    }

    if (NULL == mix_manager) {
        mix_manager = g_mix_manager;
    }

    mix = malloc(sizeof(struct aic_pcm_mix));
    if (NULL == mix) {
        loge("malloc mix fail!\n");
        return NULL;
    }
    memset(mix, 0, sizeof(struct aic_pcm_mix));

    mix->data = malloc(size * sizeof(int16_t));
    if (NULL == mix->data) {
        free(mix);
        loge("malloc mix->data fail!\n");
        return NULL;
    }

    mix->size = size;
    mix->rd = 0;
    mix->wt = 0;
    mix->id = id;
    mix->volume = 100;

    insert_mix_to_manager(mix_manager, mix);

    return mix;
}

int aic_pcm_mix_destroy(struct aic_pcm_mix *mix)
{
    struct aic_pcm_mix_manager *manager = NULL;

    if (NULL == mix) {
        loge("<%s:%d>invalid parameter!\n", __func__, __LINE__);
        return -1;
    }

    manager = mix->mix_manager;

    pop_mix_from_manager(manager, mix);

    if (mix->data) {
        free(mix->data);
        mix->data = NULL;
    }
    free(mix);

    if (0 == manager->mix_cnt) {
        aic_pcm_mix_manager_destroy(manager);
    }

    return 0;
}

int aic_pcm_mix_set_attr(struct aic_pcm_mix *mix, int sn, int ch, int sr, int bps)
{
    struct aic_pcm_mix *tmp, *main_mix = NULL;

    if (NULL == mix) {
        loge("<%s:%d>invalid parameter!\n", __func__, __LINE__);
        return -1;
    }

    mix->sample_num = sn;
    mix->ch_num = ch;
    mix->sample_rate = mix->sample_rate_org = sr;
    mix->byte_per_sample = bps;

    printf("<%s:%d> id:%d sample_rate:%d\n", __func__, __LINE__, mix->id, mix->sample_rate);

    // check if need resample
    if (0 != mix->id) {
        tmp = mix->next;
        while (tmp != mix) {
            if (0 == tmp->id) {
                main_mix = tmp;
                break;
            }
            tmp = tmp->next;
        }

        if (NULL != main_mix) {
            if (main_mix->sample_rate != mix->sample_rate) {    // need resample
                mix->sample_rate = main_mix->sample_rate;
                printf("need do resample %d -> %d\n", mix->sample_rate_org, mix->sample_rate);
            }
        }
    }

    return 0;
}

int aic_pcm_mix_set_vol(struct aic_pcm_mix *mix, int vol)
{
    return 0;
}

int resample_audio(struct aic_pcm_mix *mix, void *data, int size)
{
    struct aic_pcm_mix_manager *mix_manager;

    if ((NULL == mix) || (NULL == data) || (size <= 0)) {
        loge("invalid parameter [%p/%p/%d]\n", mix, data, size);
        return -1;
    }

    mix_manager = mix->mix_manager;

    int num_samples = size / mix->ch_num / mix->byte_per_sample;

    // clca output sample number
    double ratio = (double)mix->sample_rate / mix->sample_rate_org;
    int output_samples = (int)(num_samples * ratio + 0.5);
    int out_size = output_samples * mix->ch_num * mix->byte_per_sample;
    if (out_size > mix_manager->resample_buf_size) {
        loge("resample_buf too small, need:%d actual:%d\n", out_size, mix_manager->resample_buf_size);
        return -1;
    }

    int16_t* in_data = (int16_t*)data;
    int16_t* out_data = (int16_t*)mix_manager->resample_buf;
    // Linear interpolation resampling
    for (int ch = 0; ch < mix->ch_num; ch++) {
        for (int i = 0; i < output_samples; i++) {
            double pos = (double)i / ratio;
            int idx = (int)pos;
            double frac = pos - idx;

            if (idx >= num_samples - 1) {
                out_data[i * mix->ch_num + ch] = in_data[(num_samples - 1) * mix->ch_num + ch];
            } else {
                if (2 == mix->byte_per_sample) {
                    int16_t sample1 = in_data[idx * mix->ch_num + ch];
                    int16_t sample2 = in_data[(idx + 1) * mix->ch_num + ch];
                    out_data[i * mix->ch_num + ch] = (int16_t)(sample1 * (1.0 - frac) + sample2 * frac);
                } else if (1 == mix->byte_per_sample) {
                    int8_t sample1 = in_data[idx * mix->ch_num + ch];
                    int8_t sample2 = in_data[(idx + 1) * mix->ch_num + ch];
                    out_data[i * mix->ch_num + ch] = (int8_t)(sample1 * (1.0 - frac) + sample2 * frac);
                }
            }
        }
    }

    return (output_samples * mix->ch_num);
}

int aic_pcm_mix_write_data(struct aic_pcm_mix *mix, char *data, int size)
{
    unsigned long avail;
    int data_sample = 0, bps;

    if ((NULL == mix) || (NULL == data) || (size <= 0)) {
        loge("invalid parameter [%p/%p/%d]\n", mix, data, size);
        return -1;
    }

    bps = mix->byte_per_sample;
    if (0 == bps) {
        bps = 2;
    }

    if (mix->sample_rate != mix->sample_rate_org) {
        data_sample = resample_audio(mix, data, size);
    }

    if (data_sample > 0) {  // do resample
        data = mix->mix_manager->resample_buf;
    } else {
        data_sample = size / bps;
    }

    do {
        /* wait for space */
        int n = 0;
        while (1) {
            avail = buf_writable(mix);
            if (avail > 0) {
                break;
            } else {
                usleep(10);
                if (++n >= 300) {
                    loge("no spce to write, data size:%d space:%ld\n", size, avail * bps);
                    return -1;
                }
            }
        }

        if (avail > data_sample) {
            avail = data_sample;
        }

        unsigned long wt = mix->wt % mix->size;
        unsigned long first = mix->size - wt;
        if (first > avail) {
            first = avail;
        }
        memcpy(mix->data + wt, data, first * bps);

        if (avail > first) {
            memcpy(mix->data, data + first * bps, (avail - first) * bps);
        }

        mix->wt += avail;
        data_sample -= avail;
        data += avail * bps;
    } while (data_sample > 0);

    return size;
}

static int adjust_volume(struct aic_pcm_mix *mix, int16_t *data, int samples)
{
    int s;

    if ((NULL == mix) || (NULL == data) || (samples <= 0)) {
        loge("invalid parameter [%p / %p / %d]\n", mix, data, samples);
        return -1;
    }

    if (0 == mix->volume) {
        memset(data, 0, samples * mix->byte_per_sample);
        return 0;
    } else if (100 == mix->volume) {
        return 0;
    }

    for (int i = 0; i < samples; i++) {
        s = data[i];
        s *= mix->volume;
        s /= 100;
        data[i] = (int16_t)s;
    }

    return 0;
}

int do_mix(struct aic_pcm_mix_manager *mix_manager, int samples)
{
    unsigned long rd, avail;
    struct aic_pcm_mix *mix;
    int ret = 0, src_step, dst_step, sum_step;

    if (NULL == mix_manager) {
        mix_manager = g_mix_manager;
        if (NULL == mix_manager) {
            loge("mix_manager is null!\n");
            return -1;
        }
    }

    if (samples <= 0) {
        loge("invalid samples:%d\n", samples);
        return -1;
    }

    if (samples > mix_manager->size) {
        loge("mix_buf and sum_buf too small, samples:%d buf_size:%d\n", samples, mix_manager->size);
        return -1;
    }

    if (NULL == mix_manager->mix) {
        loge("mix list is empty!\n");
        return 0;
    }

    memset(mix_manager->mix_buf, 0, mix_manager->size * sizeof(int16_t));
    memset(mix_manager->sum_buf, 0, mix_manager->size * sizeof(int32_t));

    src_step = dst_step = sizeof(int16_t);
    sum_step = sizeof(int32_t);
    mix = mix_manager->mix;

    pthread_mutex_lock(&mix_manager->mutex);
    do {
        avail = buf_readable(mix);
        if (avail >= samples) {
            rd = mix->rd % mix->size;

            unsigned long first = mix->size - rd;
            if (first > samples) {
                first = samples;
            }
            adjust_volume(mix, mix->data + rd, first);
            generic_mix_areas_16_native(first, mix_manager->mix_buf, mix->data + rd,
                        mix_manager->sum_buf, src_step, dst_step, sum_step);

            if (samples > first) {
                adjust_volume(mix, mix->data, samples - first);
                generic_mix_areas_16_native(samples - first, mix_manager->mix_buf + first, mix->data,
                        mix_manager->sum_buf + first, src_step, dst_step, sum_step);
            }

            mix->rd += samples;
            ret = samples;
        }

        mix = mix->next;
    } while (mix != mix_manager->mix);

    pthread_mutex_unlock(&mix_manager->mutex);

    return ret;
}

unsigned long get_mix_reamin_data(struct aic_pcm_mix *mix)
{
    if (NULL == mix) {
        loge("invalid parameter!\n");
        return 0;
    }

    return buf_readable(mix);
}

int aic_set_track_volume(struct aic_pcm_mix *mix, int volume)
{
    if (NULL == mix) {
        loge("invalid parameter!\n");
        return -1;
    }

    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }

    mix->volume = volume;

    return 0;
}
