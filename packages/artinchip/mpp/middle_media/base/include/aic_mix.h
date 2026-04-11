/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <xiaodong.zhao@artinchip.com>
 * Desc: aic audio mix
 */

#ifndef __AIC_MIX_H__
#define __AIC_MIX_H__

struct aic_pcm_mix_manager;
struct aic_pcm_mix {
    int id;

    int sample_num;
    int ch_num;
    int sample_rate_org;
    int sample_rate;
    int byte_per_sample;
    int volume; // 0-100

    int16_t *data;
    unsigned long rd;
    unsigned long wt;
    unsigned long size;       // data buffer can store samples
    struct aic_pcm_mix *prev, *next;
    struct aic_pcm_mix_manager *mix_manager;
};

struct aic_pcm_mix_manager {
    int16_t *mix_buf;
    int32_t *sum_buf;
    int size;               // mix and sum buffer can store samples
    char *resample_buf;
    int resample_buf_size;

    struct aic_pcm_mix *mix;
    int mix_cnt;

    pthread_mutex_t mutex;
};

struct aic_pcm_mix_manager *aic_pcm_mix_manager_create(void);
int aic_pcm_mix_manager_destroy(struct aic_pcm_mix_manager *mix_manager);

struct aic_pcm_mix *aic_pcm_mix_create(struct aic_pcm_mix_manager *mix_manager, int id, int size);
int aic_pcm_mix_destroy(struct aic_pcm_mix *mix);
int aic_pcm_mix_set_attr(struct aic_pcm_mix *mix, int sn, int ch, int sr, int bps);

int aic_pcm_mix_write_data(struct aic_pcm_mix *mix, char *data, int size);
int do_mix(struct aic_pcm_mix_manager *mix_manager, int samples);
unsigned long get_mix_reamin_data(struct aic_pcm_mix *mix);
int aic_set_track_volume(struct aic_pcm_mix *mix, int volume);

#endif
