/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "audio_module.h"
#include "../core/module.h"
#include "../core/msg_def.h"
#include "../core/msg_bus.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <opus.h>
#include <rtdbg.h>

#define DBG_SECTION_NAME "AUDIO_MOD"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#define SOUND_DEVICE_NAME   "i2s0_sound"
#define RECORD_SAMPLERATE   16000
#define RECORD_CHANNEL      1
#define BUFSZ               1024
#define RECORD_CHUNK_SZ     ((RECORD_SAMPLERATE * RECORD_CHANNEL * 2) * 20 / 1000)  // 20ms
#define GET_TICK_DIFF(start, end) ((end) > (start) ? ((end) - (start)) : ((0xffffffff - (start)) + (end) + 1))
#define SOUND_MESSAGE_TIMEOUT 150

/* OPUS decode parameters */
#define OPUS_DECODE_SAMPLE_RATE    24000
#define OPUS_DECODE_CHANNELS       1
#define OPUS_DECODE_FRAME_SIZE     2048

/* Audio packet header struct (for OPUS data sent by XiaoZhi) */
typedef struct {
    unsigned short version;
    unsigned short type;
    unsigned int reserved;
    unsigned int timestamp;
    unsigned int payload_size;
} audio_packet_header_t;

/* Audio module private data */
typedef struct {
    rt_device_t mic_dev;
    rt_device_t snd_dev;
    rt_sem_t    sem;
    rt_bool_t   audio_have_play_flag;
    rt_bool_t   initialized;
    OpusEncoder *opus_encoder;
    OpusDecoder *opus_decoder;
    char        wav_data[4096];
    int         wav_data_size;
    uint32_t    play_start_tick;  /* Playback start timestamp */
    uint8_t     play_need_init;   /* Whether playback needs initialization */
    int         current_volume;   /* Current volume (0-100), default 80 */
} audio_module_priv_t;

static audio_module_priv_t audio_priv = {0};

/* Forward declarations */
static int audio_play_pcm_data(uint8_t *buffer, uint32_t size);
static int audio_init_output_device_for_volume(void);

/* Endianness conversion helpers */
static unsigned short be16_to_cpu(unsigned short be_val)
{
    return (be_val >> 8) | (be_val << 8);
}

static unsigned int be32_to_cpu(unsigned int be_val)
{
    return ((be_val & 0x000000FF) << 24) |
           ((be_val & 0x0000FF00) << 8)  |
           ((be_val & 0x00FF0000) >> 8)  |
           ((be_val & 0xFF000000) >> 24);
}

/* Initialize OPUS decoder */
static int init_opus_decoder(void)
{
    int error;

    if (audio_priv.opus_decoder != RT_NULL) {
        opus_decoder_destroy(audio_priv.opus_decoder);
        audio_priv.opus_decoder = RT_NULL;
    }

    audio_priv.opus_decoder = opus_decoder_create(OPUS_DECODE_SAMPLE_RATE, OPUS_DECODE_CHANNELS, &error);
    if (error != OPUS_OK || audio_priv.opus_decoder == RT_NULL) {
        LOG_E("Failed to create OPUS decoder: %d", error);
        return -RT_ERROR;
    }

    LOG_I("OPUS decoder initialized successfully");
    return RT_EOK;
}

/* Destroy OPUS decoder */
static void deinit_opus_decoder(void)
{
    if (audio_priv.opus_decoder != RT_NULL) {
        opus_decoder_destroy(audio_priv.opus_decoder);
        audio_priv.opus_decoder = RT_NULL;
    }
}

/* Decode OPUS data and play */
static int audio_decode_opus_and_play(const unsigned char *opus_data, unsigned int opus_len)
{
    opus_int16 *pcm_buffer;
    int samples;
    int pcm_bytes;

    if (audio_priv.opus_decoder == RT_NULL || opus_data == RT_NULL || opus_len == 0) {
        LOG_E("Invalid parameters for OPUS decode");
        return -RT_ERROR;
    }

    /* Allocate PCM buffer */
    pcm_buffer = (opus_int16*)rt_malloc(OPUS_DECODE_FRAME_SIZE * OPUS_DECODE_CHANNELS * sizeof(opus_int16));
    if (pcm_buffer == RT_NULL) {
        LOG_E("Failed to allocate PCM buffer");
        return -RT_ENOMEM;
    }

    /* Decode OPUS data */
    samples = opus_decode(audio_priv.opus_decoder, opus_data, opus_len,
                         pcm_buffer, OPUS_DECODE_FRAME_SIZE, 0);

    if (samples > 0) {
        pcm_bytes = samples * sizeof(short) * OPUS_DECODE_CHANNELS;

        /* Accumulate audio data */
        if ((int)(audio_priv.wav_data_size + pcm_bytes) < (int)sizeof(audio_priv.wav_data)) {
            rt_memcpy(audio_priv.wav_data + audio_priv.wav_data_size,
                     pcm_buffer, pcm_bytes);
            audio_priv.wav_data_size += pcm_bytes;
        }

        /* Play after accumulated size reaches threshold */
        if (audio_priv.wav_data_size >= 2048) {
            LOG_D("Playing PCM data, size=%d", audio_priv.wav_data_size);
            if (audio_play_pcm_data((uint8_t *)audio_priv.wav_data, audio_priv.wav_data_size) != RT_EOK) {
                LOG_E("Failed to play PCM data");
            }
            audio_priv.wav_data_size = 0;
        }
    } else {
        LOG_E("OPUS decoding failed with error code: %d", samples);
        rt_free(pcm_buffer);
        return -RT_ERROR;
    }

    rt_free(pcm_buffer);
    return RT_EOK;
}

/* Handle OPUS audio data with header (currently parse/skip header only; payload_size consistency is not strictly validated) */
static int audio_decode_opus_packet(const unsigned char *packet_data, unsigned int packet_len)
{
    audio_packet_header_t *header;
    const unsigned char *opus_data;
    unsigned int opus_len;
    unsigned int payload_size;

    if (packet_data == RT_NULL || packet_len < sizeof(audio_packet_header_t)) {
        LOG_E("Invalid packet data or too short");
        return -RT_ERROR;
    }

    /* Parse packet header */
    header = (audio_packet_header_t *)packet_data;
    payload_size = be32_to_cpu(header->payload_size);

    /* Get OPUS payload (skip header) */
    opus_data = packet_data + sizeof(audio_packet_header_t);
    opus_len = packet_len - sizeof(audio_packet_header_t);

    /* Validate payload length from header to avoid decoding malformed packets */
    if (payload_size != opus_len) {
        LOG_E("Invalid OPUS packet payload size: header=%u, actual=%u", payload_size, opus_len);
        return -RT_ERROR;
    }

    /* Decode and play */
    return audio_decode_opus_and_play(opus_data, opus_len);
}

/* Initialize semaphore */
static void audio_sem_init(void)
{
    audio_priv.sem = rt_sem_create("audio_sem", 1, RT_IPC_FLAG_FIFO);
    if (audio_priv.sem == RT_NULL) {
        LOG_E("sem_init failed!");
    }
}

/* Initialize audio output device (for playback)
 * Note: this function is deprecated and kept only for compatibility
 */
static int audio_init_output_device_for_volume(void)
{
    struct rt_audio_caps caps = {0};
    rt_device_t temp_dev = RT_NULL;
    rt_err_t ret;

    /* If device is already opened, return success directly */
    if (audio_priv.snd_dev != RT_NULL) {
        return RT_EOK;
    }

    /* Try to acquire semaphore with timeout to avoid deadlock (100ms timeout) */
    if (rt_sem_take(audio_priv.sem, rt_tick_from_millisecond(100)) != RT_EOK) {
        LOG_W("Cannot acquire semaphore for volume setting (mic may be in use), trying to set volume without opening device");
        /* If semaphore is unavailable (mic in use), try direct device operation (without opening) */
        temp_dev = rt_device_find(SOUND_DEVICE_NAME);
        if (temp_dev != RT_NULL) {
            /* Try setting volume (some drivers may allow it even when unopened) */
            caps.main_type = AUDIO_TYPE_MIXER;
            caps.sub_type = AUDIO_MIXER_VOLUME;
            caps.udata.value = 0;  /* Temporary value, set by caller */
            ret = rt_device_control(temp_dev, AUDIO_CTL_CONFIGURE, &caps);
            if (ret == RT_EOK) {
                LOG_D("Volume can be set without opening device");
                return RT_EOK;
            }
        }
        return -RT_ERROR;
    }

    /* Close mic device while holding semaphore to avoid races
     * Mic must be closed before opening playback device, because the same device cannot be opened for input and output simultaneously
     */
    if (audio_priv.mic_dev != RT_NULL) {
        rt_device_close(audio_priv.mic_dev);
        audio_priv.mic_dev = RT_NULL;
    }

    /* Open playback device */
    audio_priv.snd_dev = rt_device_find(SOUND_DEVICE_NAME);
    if (audio_priv.snd_dev == RT_NULL) {
        LOG_E("%s not found!", SOUND_DEVICE_NAME);
        rt_sem_release(audio_priv.sem);
        return -RT_ERROR;
    }

    if (rt_device_open(audio_priv.snd_dev, RT_DEVICE_OFLAG_WRONLY) != RT_EOK) {
        LOG_E("Failed to open audio output device");
        audio_priv.snd_dev = RT_NULL;
        rt_sem_release(audio_priv.sem);
        return -RT_ERROR;
    }

    caps.main_type = AUDIO_TYPE_OUTPUT;
    caps.sub_type = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = 24000;
    caps.udata.config.channels = 1;
    caps.udata.config.samplebits = 16;
    ret = rt_device_control(audio_priv.snd_dev, AUDIO_CTL_CONFIGURE, &caps);
    if (ret != RT_EOK) {
        LOG_E("Failed to configure audio output device");
        rt_device_close(audio_priv.snd_dev);
        audio_priv.snd_dev = RT_NULL;
        rt_sem_release(audio_priv.sem);
        return -RT_ERROR;
    }

    LOG_D("Audio output device initialized for volume setting");
    /* Note: release semaphore immediately after setting volume; do not keep device open */
    rt_sem_release(audio_priv.sem);
    return RT_EOK;
}

/* Play PCM data */
static int audio_play_pcm_data(uint8_t *buffer, uint32_t size)
{
    struct rt_audio_caps caps = {0};
    uint32_t end_tick;

    end_tick = rt_tick_get();
    /* If time since last playback exceeds threshold, previous playback ended and device must be reinitialized */
    if (GET_TICK_DIFF(audio_priv.play_start_tick, end_tick) > SOUND_MESSAGE_TIMEOUT) {
        /* Acquire semaphore to close mic device and open playback device */
        if (rt_sem_take(audio_priv.sem, rt_tick_from_millisecond(100)) == RT_EOK) {
            audio_priv.audio_have_play_flag = 1;
            audio_priv.play_need_init = 0;  /* Reset init flag; reinitialization required */
        } else {
            LOG_E("Failed to take semaphore for audio playback");
            return -RT_ERROR;
        }
    }

    audio_priv.play_start_tick = rt_tick_get();

    if (audio_priv.play_need_init == 0) {
        audio_priv.play_need_init = 1;
        /* Close mic device while holding semaphore to avoid races
         * Mic must be closed before opening playback device, because the same device cannot be opened for input and output simultaneously
         */
        if (audio_priv.mic_dev != RT_NULL) {
            rt_device_close(audio_priv.mic_dev);
            audio_priv.mic_dev = RT_NULL;
        }

        /* Open playback device */
        audio_priv.snd_dev = rt_device_find(SOUND_DEVICE_NAME);
        if (audio_priv.snd_dev == RT_NULL) {
            LOG_E("%s not found!", SOUND_DEVICE_NAME);
            audio_priv.play_need_init = 0;
            rt_sem_release(audio_priv.sem);
            return -RT_ERROR;
        }

        if (rt_device_open(audio_priv.snd_dev, RT_DEVICE_OFLAG_WRONLY) != RT_EOK) {
            LOG_E("Failed to open audio output device");
            audio_priv.snd_dev = RT_NULL;
            audio_priv.play_need_init = 0;
            rt_sem_release(audio_priv.sem);
            return -RT_ERROR;
        }

        caps.main_type = AUDIO_TYPE_OUTPUT;
        caps.sub_type = AUDIO_DSP_PARAM;
        caps.udata.config.samplerate = 24000;
        caps.udata.config.channels = 1;
        caps.udata.config.samplebits = 16;
        if (rt_device_control(audio_priv.snd_dev, AUDIO_CTL_CONFIGURE, &caps) != RT_EOK) {
            LOG_E("Failed to configure audio output device");
            rt_device_close(audio_priv.snd_dev);
            audio_priv.snd_dev = RT_NULL;
            audio_priv.play_need_init = 0;
            rt_sem_release(audio_priv.sem);
            return -RT_ERROR;
        }

        /* Apply saved volume value */
        caps.main_type = AUDIO_TYPE_MIXER;
        caps.sub_type = AUDIO_MIXER_VOLUME;
        caps.udata.value = audio_priv.current_volume;
        LOG_D("Applying saved volume %d to audio device during initialization", audio_priv.current_volume);
        if (rt_device_control(audio_priv.snd_dev, AUDIO_CTL_CONFIGURE, &caps) == RT_EOK) {
            LOG_D("Audio playback device initialized with volume %d", audio_priv.current_volume);
        } else {
            LOG_W("Failed to set volume %d during device initialization", audio_priv.current_volume);
        }

        /* Note: do not release semaphore here; it is released by timeout check in playback thread loop */
    }

    /* Check whether device is initialized */
    if (audio_priv.snd_dev == RT_NULL) {
        LOG_E("Audio output device not initialized");
        return -RT_ERROR;
    }

    /* Play audio data while holding semaphore to prevent interference from recording thread */
    if (buffer != RT_NULL && size > 0) {
        int offset = 0;
        int to_read = 0;
        while (size > 0) {
            to_read = (size > BUFSZ) ? BUFSZ : size;
            rt_device_write(audio_priv.snd_dev, 0, buffer + offset, to_read);
            size -= to_read;
            offset += to_read;
        }
    }

    return RT_EOK;
}

/* Recording thread entry */
static void audio_record_thread_entry(void *parameter)
{
    uint8_t *buffer = NULL;
    struct rt_audio_caps caps = {0};
    int length = 0;
    OpusEncoder *encoder;
    int err;

    LOG_I("Audio record thread started");

    /* Create OPUS encoder */
    encoder = opus_encoder_create(RECORD_SAMPLERATE, RECORD_CHANNEL, OPUS_APPLICATION_AUDIO, &err);
    if (!encoder) {
        LOG_E("Failed to create Opus encoder (error: %d)", err);
        return;
    }
    opus_encoder_ctl(encoder, OPUS_SET_BITRATE(128000));

    buffer = (uint8_t *)rt_malloc(RECORD_CHUNK_SZ);
    if (buffer == RT_NULL) {
        LOG_E("malloc record chunk size failed!");
        opus_encoder_destroy(encoder);
        return;
    }

    audio_priv.mic_dev = rt_device_find(SOUND_DEVICE_NAME);
    if (audio_priv.mic_dev == RT_NULL) {
        LOG_E("%s not found!", SOUND_DEVICE_NAME);
        rt_free(buffer);
        opus_encoder_destroy(encoder);
        return;
    }

    rt_device_open(audio_priv.mic_dev, RT_DEVICE_OFLAG_RDONLY);

    caps.main_type = AUDIO_TYPE_INPUT;
    caps.sub_type = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = RECORD_SAMPLERATE;
    caps.udata.config.channels = 1;
    caps.udata.config.samplebits = 16;
    rt_device_control(audio_priv.mic_dev, AUDIO_CTL_CONFIGURE, &caps);

    /* External variable indicating XiaoZhi readiness */
    extern uint8_t xiaozhi_is_ok;

    while (1) {
        length = 0;
        rt_memset(buffer, 0, RECORD_CHUNK_SZ);

        /* Wait on semaphore (mutual exclusion with playback) */
        if (rt_sem_take(audio_priv.sem, rt_tick_from_millisecond(100)) == RT_EOK) {
            if (audio_priv.audio_have_play_flag == 1) {
                LOG_I("Audio playback finished, reinitialize mic device");
                audio_priv.audio_have_play_flag = 0;

                /* Note: mic device has been closed and set to NULL by playback thread.
                 * No need to close again; reopen by finding/opening device directly
                 * (follows original logic to avoid duplicate close on already-closed device).
                 */

                /* Reopen mic device */
                audio_priv.mic_dev = rt_device_find(SOUND_DEVICE_NAME);
                if (audio_priv.mic_dev != RT_NULL) {
                    if (rt_device_open(audio_priv.mic_dev, RT_DEVICE_OFLAG_RDONLY) == RT_EOK) {
                        if (rt_device_control(audio_priv.mic_dev, AUDIO_CTL_CONFIGURE, &caps) == RT_EOK) {
                            LOG_I("Mic device reinitialized successfully");
                        } else {
                            LOG_E("Failed to configure mic device");
                            rt_device_close(audio_priv.mic_dev);
                            audio_priv.mic_dev = RT_NULL;
                        }
                    } else {
                        LOG_E("Failed to reopen mic device");
                        audio_priv.mic_dev = RT_NULL;
                    }
                } else {
                    LOG_E("Failed to find mic device");
                }
            }

            /* Read only when mic device is valid */
            if (audio_priv.mic_dev != RT_NULL) {
                length = rt_device_read(audio_priv.mic_dev, 0, buffer, RECORD_CHUNK_SZ);
            } else {
                length = 0;  /* Device invalid, skip this recording cycle */
            }
            rt_sem_release(audio_priv.sem);
        }

        /* If XiaoZhi is ready, encode and send audio */
        if (length > 0 && xiaozhi_is_ok) {
            unsigned char packet[2048] = {0};
            int packet_size = opus_encode(encoder, (opus_int16 *)buffer, length / 2,
                                         packet, sizeof(packet));

            if (packet_size > 0) {
                /* Send encoded OPUS data to XiaoZhi module */
                app_message_t msg;
                rt_memset(&msg, 0, sizeof(msg));
                msg.header.type = MSG_TYPE_AUDIO_ENCODE_OPUS;
                msg.header.src_module = MODULE_ID_AUDIO;
                msg.header.dst_module = MODULE_ID_XIAOZHI;

                /* Allocate memory and copy data */
                void *opus_data = rt_malloc(packet_size);
                if (opus_data != RT_NULL) {
                    rt_memcpy(opus_data, packet, packet_size);
                    msg.header.data_len = sizeof(msg_audio_data_t);
                    msg.data.audio.data = opus_data;
                    msg.data.audio.data_len = packet_size;
                    msg.data.audio.sample_rate = RECORD_SAMPLERATE;
                    msg.data.audio.channels = RECORD_CHANNEL;
                    msg.data.audio.bits = 16;
                    module_send_msg(&msg);
                }
            }
        }
    }

    rt_device_close(audio_priv.mic_dev);
    opus_encoder_destroy(encoder);
    rt_free(buffer);
}

/* Audio module message handler */
static int audio_msg_handler(const app_message_t *msg)
{
    if (msg == RT_NULL) {
        return -RT_EINVAL;
    }

    switch (msg->header.type) {
        case MSG_TYPE_SYS_INIT:
            LOG_I("Audio module received init message");
            break;

        case MSG_TYPE_AUDIO_PLAY_PCM:
        case MSG_TYPE_XIAOZHI_AUDIO_RECV:
            /* Play PCM audio data */
            if (msg->data.audio.data != RT_NULL && msg->data.audio.data_len > 0) {
                audio_play_pcm_data((uint8_t *)msg->data.audio.data, msg->data.audio.data_len);
                /* Free memory */
                rt_free(msg->data.audio.data);
            }
            break;

        case MSG_TYPE_AUDIO_DECODE_OPUS:
            /* Decode OPUS audio data and play */
            if (msg->data.audio.data != RT_NULL && msg->data.audio.data_len > 0) {
                LOG_D("Received OPUS data to decode, len=%d", msg->data.audio.data_len);
                if (audio_decode_opus_packet((const unsigned char *)msg->data.audio.data,
                                            msg->data.audio.data_len) != RT_EOK) {
                    LOG_E("Failed to decode OPUS packet");
                }
                /* Free memory */
                rt_free(msg->data.audio.data);
            } else {
                LOG_W("Received empty OPUS data");
            }
            break;

        case MSG_TYPE_AUDIO_SET_VOLUME:
            /* Set volume */
            {
                int volume = msg->data.audio_volume.volume;
                LOG_D("Audio module received volume set message: %d", volume);
                if (audio_module_set_volume(volume) == RT_EOK) {
                    LOG_D("Volume set to %d successfully", volume);
                } else {
                    LOG_E("Failed to set volume to %d", volume);
                }
            }
            break;

        case MSG_TYPE_AUDIO_GET_VOLUME:
            /* Get volume (can be returned via response message if needed) */
            {
                int volume = audio_module_get_volume();
                (void)volume;
                LOG_D("Audio module received volume get message, current volume: %d", volume);
                /* Note: return volume to requester via response-message mechanism if required */
            }
            break;

        default:
            LOG_D("Audio module received unhandled msg type: 0x%02X", msg->header.type);
            break;
    }

    return RT_EOK;
}

/* Audio module init */
static int audio_init(void)
{
    rt_memset(&audio_priv, 0, sizeof(audio_priv));
    audio_priv.initialized = RT_FALSE;
    audio_priv.current_volume = 80;  /* Default volume 80% */

    /* Initialize semaphore */
    audio_sem_init();

    /* Initialize OPUS decoder */
    init_opus_decoder();

    LOG_I("Audio module initialized");

    /* Subscribe messages */
    msg_bus_subscribe(MODULE_ID_AUDIO, MSG_TYPE_AUDIO_PLAY_PCM);
    msg_bus_subscribe(MODULE_ID_AUDIO, MSG_TYPE_XIAOZHI_AUDIO_RECV);
    msg_bus_subscribe(MODULE_ID_AUDIO, MSG_TYPE_AUDIO_DECODE_OPUS);
    msg_bus_subscribe(MODULE_ID_AUDIO, MSG_TYPE_AUDIO_SET_VOLUME);
    msg_bus_subscribe(MODULE_ID_AUDIO, MSG_TYPE_AUDIO_GET_VOLUME);
    msg_bus_subscribe(MODULE_ID_AUDIO, MSG_TYPE_SYS_INIT);

    return RT_EOK;
}

/* Audio module deinit */
static int audio_deinit(void)
{
    if (audio_priv.mic_dev != RT_NULL) {
        rt_device_close(audio_priv.mic_dev);
        audio_priv.mic_dev = RT_NULL;
    }

    if (audio_priv.snd_dev != RT_NULL) {
        rt_device_close(audio_priv.snd_dev);
        audio_priv.snd_dev = RT_NULL;
    }

    if (audio_priv.sem != RT_NULL) {
        rt_sem_delete(audio_priv.sem);
        audio_priv.sem = RT_NULL;
    }

    /* Destroy OPUS decoder */
    deinit_opus_decoder();

    msg_bus_unsubscribe(MODULE_ID_AUDIO, MSG_TYPE_AUDIO_PLAY_PCM);
    msg_bus_unsubscribe(MODULE_ID_AUDIO, MSG_TYPE_XIAOZHI_AUDIO_RECV);
    msg_bus_unsubscribe(MODULE_ID_AUDIO, MSG_TYPE_AUDIO_DECODE_OPUS);
    msg_bus_unsubscribe(MODULE_ID_AUDIO, MSG_TYPE_SYS_UNKNOWN);

    LOG_I("Audio module deinitialized");
    return RT_EOK;
}

/* Audio playback thread entry */
static void audio_play_thread_entry(void *parameter)
{
    app_message_t msg;
    uint32_t end_tick;

    LOG_I("Audio play thread started");

    while (1) {
        /* Receive message */
        if (msg_bus_receive(MODULE_ID_AUDIO, &msg, 100) == RT_EOK) {
            audio_msg_handler(&msg);
        }

        /* Check timeout (close playback device and release semaphore when needed) */
        end_tick = rt_tick_get();
        if (GET_TICK_DIFF(audio_priv.play_start_tick, end_tick) > SOUND_MESSAGE_TIMEOUT) {
            /* If playback device is open, close it and release semaphore */
            if (audio_priv.play_need_init == 1 && audio_priv.snd_dev != RT_NULL) {
                rt_device_close(audio_priv.snd_dev);
                audio_priv.snd_dev = RT_NULL;
                audio_priv.play_need_init = 0;
                rt_sem_release(audio_priv.sem);
                LOG_D("Audio playback timeout, device closed and semaphore released");
            }
        }
    }
}

/* Audio module operations */
static module_ops_t audio_ops = {
    .init = audio_init,
    .deinit = audio_deinit,
    .msg_handler = audio_msg_handler,
    .name = "Audio Module",
    .id = MODULE_ID_AUDIO,
};

/* Audio module instance */
static module_t audio_module = {
    .ops = &audio_ops,
    .active = RT_FALSE,
    .thread = RT_NULL,
    .priv_data = &audio_priv,
};

/* Get Audio module instance */
module_t *audio_module_get(void)
{
    return &audio_module;
}

/* Set audio output volume */
int audio_module_set_volume(int volume)
{
    struct rt_audio_caps caps = {0};
    rt_err_t ret = RT_EOK;

    if (volume < 0 || volume > 100) {
        LOG_E("Invalid volume value: %d (should be 0-100)", volume);
        return -RT_EINVAL;
    }

    /* Save volume value */
    audio_priv.current_volume = volume;
    LOG_D("Audio volume set to %d (saved)", volume);

    /* Apply volume immediately if playback device is already open */
    if (audio_priv.snd_dev != RT_NULL) {
        caps.main_type = AUDIO_TYPE_MIXER;
        caps.sub_type = AUDIO_MIXER_VOLUME;
        caps.udata.value = volume;
        LOG_D("Applying volume %d to opened audio device", volume);
        ret = rt_device_control(audio_priv.snd_dev, AUDIO_CTL_CONFIGURE, &caps);
        if (ret == RT_EOK) {
            LOG_D("Audio volume applied to device successfully: %d", volume);
        } else {
            LOG_E("Failed to apply audio volume to device, ret=%d, volume=%d", ret, volume);
        }
    } else {
        /* Device not open: only save volume; applied automatically on device init */
        LOG_D("Audio device not open, volume %d will be applied when device initializes", volume);
    }

    return ret;
}

/* Get current audio output volume */
int audio_module_get_volume(void)
{
    struct rt_audio_caps caps = {0};
    rt_err_t ret;

    /* If playback device is open, try reading actual volume from device */
    if (audio_priv.snd_dev != RT_NULL) {
        caps.main_type = AUDIO_TYPE_MIXER;
        caps.sub_type = AUDIO_MIXER_VOLUME;
        caps.udata.value = 0;
        ret = rt_device_control(audio_priv.snd_dev, AUDIO_CTL_GETCAPS, &caps);
        if (ret == RT_EOK) {
            audio_priv.current_volume = caps.udata.value;  /* Update cached value */
            LOG_D("Current audio volume from device: %d", caps.udata.value);
            return caps.udata.value;
        }
    }

    /* Device not open or read failed: return cached volume value */
    LOG_D("Audio device not open, returning saved volume: %d", audio_priv.current_volume);
    return audio_priv.current_volume;
}

/* Initialize Audio module thread */
int audio_module_thread_start(void)
{
    /* Start playback thread */
    audio_module.thread = rt_thread_create("audio_play",
                                           audio_play_thread_entry,
                                           RT_NULL,
                                           2048*8,  /* Increase stack size from 4096 to 8192 to prevent overflow */
                                           17,
                                           10);
    if (audio_module.thread != RT_NULL) {
        rt_thread_startup(audio_module.thread);
    }

    /* Start recording thread */
    rt_thread_t record_thread = rt_thread_create("audio_record",
                                                  audio_record_thread_entry,
                                                  RT_NULL,
                                                  8192 * 4,
                                                  18,
                                                  10);
    if (record_thread != RT_NULL) {
        rt_thread_startup(record_thread);
        return RT_EOK;
    }

    return -RT_ERROR;
}

