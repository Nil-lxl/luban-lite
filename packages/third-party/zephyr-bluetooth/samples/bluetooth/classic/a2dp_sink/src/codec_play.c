/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/audio/codec.h>
#include "audio_buf.h"
#include "codec_play.h"

#include <rtthread.h>
#include <rtdevice.h>

#if IS_ENABLED(CONFIG_ZEPHYR_BLUETOOTH_SAMPLE_A2DP_SINK)

#if NOCACHE_MEMORY
#define __NOCACHE	__attribute__((__section__(".nocache")))
#elif defined(DT_DEFINED_NOCACHE)
#define __NOCACHE	__attribute__((__section__(DT_DEFINED_NOCACHE_NAME)))
#else /* NOCACHE_MEMORY */
#define __NOCACHE
#endif /* NOCACHE_MEMORY */

/* audio stream control variables */
static volatile bool audio_start;
static uint32_t audio_sample_rate;
static uint8_t *audio_data_sync_buf[A2DP_BOARD_CODEC_PLAY_COUNT];
static uint32_t audio_data_sync_buf_size[A2DP_BOARD_CODEC_PLAY_COUNT];
static uint8_t audio_data_sync_buf_w;
static uint8_t audio_data_sync_buf_r;
static __NOCACHE __aligned(4) uint8_t a2dp_silence_data[A2DP_SBC_DATA_PLAY_SIZE_48K];
#define SOUND_DEVICE_NAME   "sound0"
#define I2S_TIMEOUT (2000U)

static rt_device_t snd_dev;

static __NOCACHE __aligned(4) uint8_t mem_slab_buffer[A2DP_BOARD_CODEC_PLAY_COUNT *
						      A2DP_SBC_DATA_PLAY_SIZE_48K];
static struct k_mem_slab mem_slab;

int codec_play_init(void)
{
    snd_dev = rt_device_find(SOUND_DEVICE_NAME);
    if (!snd_dev) {
        rt_kprintf("%s not found!\n", SOUND_DEVICE_NAME);
        return -RT_ERROR;
    }

	return 0;
}

void codec_play_configure(uint32_t sample_rate, uint8_t sample_width, uint8_t channels)
{
    struct rt_audio_caps caps = { 0 };

	audio_sample_rate = sample_rate;

    rt_device_open(snd_dev, RT_DEVICE_OFLAG_WRONLY);

    caps.main_type               = AUDIO_TYPE_OUTPUT;
    caps.sub_type                = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = sample_rate;
    caps.udata.config.channels   = channels;
    caps.udata.config.samplebits = sample_width;
    rt_device_control(snd_dev, AUDIO_CTL_CONFIGURE, &caps);
}

static void codec_play_to_dev(uint8_t *data, uint32_t length)
{
    rt_device_write(snd_dev, 0, data, length);
}

static void codec_play_data(uint8_t *data, uint32_t length)
{
	audio_data_sync_buf[audio_data_sync_buf_w % A2DP_BOARD_CODEC_PLAY_COUNT] = data;
	audio_data_sync_buf_size[audio_data_sync_buf_w % A2DP_BOARD_CODEC_PLAY_COUNT] =
											length;
	audio_data_sync_buf_w++;

	if (!audio_start) {
		return;
	}

	if ((data != NULL) && (length != 0U)) {
		codec_play_to_dev(data, length);
	} else {
		codec_play_to_dev(a2dp_silence_data,
				  audio_sample_rate == 48000 ?
				  A2DP_SBC_DATA_PLAY_SIZE_48K : A2DP_SBC_DATA_PLAY_SIZE_44_1K);
	}
}

void codec_play_start(void)
{
    int stream = 0;

	if (audio_start) {
	rt_thread_mdelay(1);
		return;
	}

	for (uint8_t i = 0; i < A2DP_BOARD_CODEC_PLAY_COUNT; i++) {
		codec_play_data(a2dp_silence_data,
				audio_sample_rate == 48000 ?
				A2DP_SBC_DATA_PLAY_SIZE_48K : A2DP_SBC_DATA_PLAY_SIZE_44_1K);

		if (i == 0) {
            /* Start to playback. This step will enable Power Amplifier */
            stream = AUDIO_STREAM_REPLAY;
            rt_device_control(snd_dev, AUDIO_CTL_START, (void *)&stream);
            /* Wait power amplifier stable */
            rt_thread_mdelay(200);      //This is an experience value

	        audio_start = true;
		}
	}
}

void codec_play_stop(void)
{
	if (!audio_start) {
		return;
	}

	audio_start = false;
}

void codec_keep_play(void)
{
	uint8_t *get_data;
	uint32_t length;

	while (true) {
		if (!audio_start) {
			rt_thread_mdelay(1);
			continue;
		}

		if (audio_sample_rate == 44100) {
			length = A2DP_SBC_DATA_PLAY_SIZE_44_1K;
		} else {
			length = A2DP_SBC_DATA_PLAY_SIZE_48K;
		}
		/* play data */
		audio_get_pcm_data(&get_data, length);
		codec_play_data(get_data, length);

		/* sync the already played media data */
		audio_media_sync(audio_data_sync_buf[audio_data_sync_buf_r %
				A2DP_BOARD_CODEC_PLAY_COUNT],
				audio_data_sync_buf_size[audio_data_sync_buf_r %
				A2DP_BOARD_CODEC_PLAY_COUNT]);

		audio_data_sync_buf_r++;
	}
}

#else

void codec_play_configure(uint32_t sample_rate, uint8_t sample_width, uint8_t channels)
{
	printf("Codec is unsupported\n");
}

int codec_play_init(void)
{
	return 0;
}

void codec_play_start(void)
{
}

void codec_play_stop(void)
{
}

void codec_keep_play(void)
{
}

#endif
