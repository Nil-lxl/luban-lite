/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>
#include "aic_common.h"

#define BUFSZ               1024
#define RECORD_CHUNK_SZ     2048
#define SOUND_DEVICE_NAME   "i2s0_sound"
#define RECORD_TIME_MS      5000  // Record for 10 seconds
#define PLAY_BUFFER_COUNT   4      // Number of ring buffer blocks

// WAV file header structure
struct wav_header {
    char  riff_id[4];              /* "RIFF" */
    int   riff_datasize;
    char  riff_type[4];            /* "WAVE" */
    char  fmt_id[4];               /* "fmt " */
    int   fmt_datasize;            /* fmt chunk data size,16 for pcm */
    short fmt_compression_code;    /* 1 for PCM */
    short fmt_channels;            /* 1(mono) or 2(stereo) */
    int   fmt_sample_rate;         /* samples per second */
    int   fmt_avg_bytes_per_sec;
    short fmt_block_align;
    short fmt_bit_per_sample;      /* bits of each sample(8,16,32). */
    char  data_id[4];              /* "data" */
    int   data_datasize;           /* data chunk size,pcm_size - 44 */
};

// WAV info structure
struct RIFF_HEADER_DEF {
    char riff_id[4];     // 'R','I','F','F'
    uint32_t riff_size;
    char riff_format[4]; // 'W','A','V','E'
};

struct WAVE_FORMAT_DEF {
    uint16_t FormatTag;
    uint16_t Channels;
    uint32_t SamplesPerSec;
    uint32_t AvgBytesPerSec;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
};

struct FMT_BLOCK_DEF {
    char fmt_id[4];    // 'f','m','t',' '
    uint32_t fmt_size;
    struct WAVE_FORMAT_DEF wav_format;
};

struct DATA_BLOCK_DEF {
    char data_id[4];     // 'd','a','t','a'
    uint32_t data_size;
};

struct wav_info {
    struct RIFF_HEADER_DEF header;
    struct FMT_BLOCK_DEF   fmt_block;
    struct DATA_BLOCK_DEF  data_block;
};

// Ring buffer structure
struct ring_buffer {
    uint8_t *buffer;
    uint32_t size;
    uint32_t head;
    uint32_t tail;
    rt_bool_t full;
};

// Global variables
static struct ring_buffer g_play_ringbuf;
static rt_device_t g_audio_dev = RT_NULL;
static rt_sem_t g_rx_sem = RT_NULL;
static rt_sem_t g_tx_sem = RT_NULL;
static int g_total_record_length = 0;
static rt_bool_t g_play_finished = RT_FALSE;
static rt_bool_t g_record_finished = RT_FALSE;

// Ring buffer operation functions
static void ring_buffer_init(struct ring_buffer *rb, uint8_t *buffer, uint32_t size)
{
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->full = RT_FALSE;
}

static uint32_t ring_buffer_len(struct ring_buffer *rb)
{
    if (rb->full)
        return rb->size;

    if (rb->head >= rb->tail)
        return rb->head - rb->tail;
    else
        return rb->size - (rb->tail - rb->head);
}

static rt_bool_t ring_buffer_empty(struct ring_buffer *rb)
{
    return (!rb->full && (rb->head == rb->tail));
}

static rt_bool_t ring_buffer_full(struct ring_buffer *rb)
{
    return rb->full;
}

static uint32_t ring_buffer_put(struct ring_buffer *rb, uint8_t *data, uint32_t len)
{
    uint32_t free_space;
    uint32_t i;

    free_space = rb->size - ring_buffer_len(rb);
    if (free_space == 0)
        return 0;

    if (len > free_space)
        len = free_space;

    for (i = 0; i < len; i++) {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->size;
        if (rb->head == rb->tail)
            rb->full = RT_TRUE;
    }

    return len;
}

static uint32_t ring_buffer_get(struct ring_buffer *rb, uint8_t *data, uint32_t len)
{
    uint32_t data_len;
    uint32_t i;

    data_len = ring_buffer_len(rb);
    if (data_len == 0)
        return 0;

    if (len > data_len)
        len = data_len;

    for (i = 0; i < len; i++) {
        data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % rb->size;
        rb->full = RT_FALSE;
    }

    return len;
}

// Initialize WAV file header
static void wavheader_init(struct wav_header *header, int sample_rate,
                           int channels, int datasize)
{
    rt_memcpy(header->riff_id, "RIFF", 4);
    header->riff_datasize = datasize + 44 - 8;
    rt_memcpy(header->riff_type, "WAVE", 4);
    rt_memcpy(header->fmt_id, "fmt ", 4);
    header->fmt_datasize = 16;
    header->fmt_compression_code = 1;
    header->fmt_channels = channels;
    header->fmt_sample_rate = sample_rate;
    header->fmt_bit_per_sample = 16;
    header->fmt_avg_bytes_per_sec = header->fmt_sample_rate *
                                    header->fmt_channels *
                                    header->fmt_bit_per_sample / 8;
    header->fmt_block_align = header->fmt_bit_per_sample *
                              header->fmt_channels / 8;
    rt_memcpy(header->data_id, "data", 4);
    header->data_datasize = datasize;
}

// Record thread
static void record_thread_entry(void *parameter)
{
    int fd_record = *(int *)parameter;
    uint8_t *record_buffer;
    struct wav_header record_header;
    struct rt_audio_caps record_caps = {0};
    int record_length;
    int stream = 0;

    record_buffer = rt_malloc(RECORD_CHUNK_SZ);
    if (record_buffer == RT_NULL) {
        rt_kprintf("malloc record buffer failed!\n");
        goto __exit;
    }

    // Configure record parameters
    record_caps.main_type = AUDIO_TYPE_INPUT;
    record_caps.sub_type = AUDIO_DSP_PARAM;
    record_caps.udata.config.samplerate = 16000;
    record_caps.udata.config.channels = 2;
    record_caps.udata.config.samplebits = 16;
    rt_device_control(g_audio_dev, AUDIO_CTL_CONFIGURE, &record_caps);

    // Start record stream
    stream = AUDIO_STREAM_RECORD;
    rt_device_control(g_audio_dev, AUDIO_CTL_START, (void *)&stream);

    // Wait for start
    rt_thread_mdelay(100);

    // Record loop
    while (!g_record_finished) {
        record_length = rt_device_read(g_audio_dev, 0, record_buffer, RECORD_CHUNK_SZ);
        if (record_length > 0) {
            write(fd_record, record_buffer, record_length);
            g_total_record_length += record_length;
        }

        // Check if recording time is reached
        if ((g_total_record_length / RECORD_CHUNK_SZ) > (RECORD_TIME_MS / 20)) {
            g_record_finished = RT_TRUE;
            break;
        }

        // rt_thread_mdelay(5);
    }

    // Update WAV header of record file
    wavheader_init(&record_header, 16000, 2, g_total_record_length);
    lseek(fd_record, 0, SEEK_SET);
    write(fd_record, &record_header, sizeof(struct wav_header));

__exit:
    if (record_buffer)
        rt_free(record_buffer);

    rt_kprintf("Record thread finished\n");
}

// Play thread
static void play_thread_entry(void *parameter)
{
    int fd_play = *(int *)parameter;
    uint8_t *play_buffer;
    struct wav_info *play_info = RT_NULL;
    struct rt_audio_caps play_caps = {0};
    int len, stream = 0;
    u64 play_size = 0;
    uint8_t *ring_buffer_data;

    play_buffer = rt_malloc(BUFSZ);
    if (play_buffer == RT_NULL) {
        rt_kprintf("malloc play buffer failed!\n");
        goto __exit;
    }

    play_info = (struct wav_info *) rt_malloc(sizeof(struct wav_info));
    if (play_info == RT_NULL) {
        rt_kprintf("malloc play info failed!\n");
        goto __exit;
    }

    // Read play file header info
    len = read(fd_play, &(play_info->header), sizeof(struct RIFF_HEADER_DEF));
    if (len < sizeof(struct RIFF_HEADER_DEF) ||
        strncmp(play_info->header.riff_id, "RIFF", 4)) {
        rt_kprintf("Get play file header chunk failed!\n");
        goto __exit;
    }

    len = read(fd_play, &(play_info->fmt_block), sizeof(struct FMT_BLOCK_DEF));
    if (len < sizeof(struct FMT_BLOCK_DEF) ||
        strncmp(play_info->fmt_block.fmt_id, "fmt", 3)) {
        rt_kprintf("Get play file format chunk failed!\n");
        goto __exit;
    }

    // Find data chunk of play file
    do {
        len = read(fd_play, &(play_info->data_block), sizeof(struct DATA_BLOCK_DEF));
        if (len < sizeof(struct DATA_BLOCK_DEF)) {
            rt_kprintf("Get play file data chunk failed!\n");
            goto __exit;
        } else if (strncmp(play_info->data_block.data_id, "data", 4)) {
            lseek(fd_play, play_info->data_block.data_size, SEEK_CUR);
        } else {
            break;
        }
    } while(1);

    rt_kprintf("play file information:\n");
    rt_kprintf("samplerate %d\n", play_info->fmt_block.wav_format.SamplesPerSec);
    rt_kprintf("channel %d\n", play_info->fmt_block.wav_format.Channels);
    rt_kprintf("samplebits %d\n", play_info->fmt_block.wav_format.BitsPerSample);
    rt_kprintf("data_size 0x%08x\n", play_info->data_block.data_size);

    // Configure play parameters
    play_caps.main_type = AUDIO_TYPE_OUTPUT;
    play_caps.sub_type = AUDIO_DSP_PARAM;
    play_caps.udata.config.samplerate = play_info->fmt_block.wav_format.SamplesPerSec;
    play_caps.udata.config.channels = play_info->fmt_block.wav_format.Channels;
    play_caps.udata.config.samplebits = play_info->fmt_block.wav_format.BitsPerSample;
    rt_device_control(g_audio_dev, AUDIO_CTL_CONFIGURE, &play_caps);

    // Start play stream
    stream = AUDIO_STREAM_REPLAY;
    rt_device_control(g_audio_dev, AUDIO_CTL_START, (void *)&stream);

    // Wait for amplifier stable
    rt_thread_mdelay(200);

    play_size = play_info->data_block.data_size;

    // Create ring buffer
    ring_buffer_data = rt_malloc(BUFSZ * PLAY_BUFFER_COUNT);
    if (ring_buffer_data == RT_NULL) {
        rt_kprintf("malloc ring buffer failed!\n");
        goto __exit;
    }

    ring_buffer_init(&g_play_ringbuf, ring_buffer_data, BUFSZ * PLAY_BUFFER_COUNT);

    // Preload some data to ring buffer
    while (!ring_buffer_full(&g_play_ringbuf) && play_size > 0) {
        int to_read = (play_size > BUFSZ) ? BUFSZ : play_size;
        len = read(fd_play, play_buffer, to_read);
        if (len <= 0)
            break;

        ring_buffer_put(&g_play_ringbuf, play_buffer, len);
        play_size -= len;
    }

    // Play loop
    while (!g_play_finished) {
        // If ring buffer has data, then play
        if (!ring_buffer_empty(&g_play_ringbuf)) {
            uint32_t get_len;
            get_len = ring_buffer_get(&g_play_ringbuf, play_buffer, BUFSZ);
            if (get_len > 0) {
                rt_device_write(g_audio_dev, 0, play_buffer, get_len);
            }
        } else if (play_size > 0) {
            // Buffer is empty, read more data from file
            int to_read = (play_size > BUFSZ) ? BUFSZ : play_size;
            len = read(fd_play, play_buffer, to_read);
            if (len <= 0) {
                // File read finished
                g_play_finished = RT_TRUE;
                break;
            }

            ring_buffer_put(&g_play_ringbuf, play_buffer, len);
            play_size -= len;
        } else {
            // Play finished
            g_play_finished = RT_TRUE;
            break;
        }

        rt_thread_mdelay(1);
    }

__exit:
    if (play_buffer)
        rt_free(play_buffer);

    if (play_info)
        rt_free(play_info);

    rt_kprintf("Play thread finished\n");
}

// Test function for simultaneous record and play
int test_i2s_record_play(int argc, char **argv)
{
    int fd_record = -1, fd_play = -1;
    int ret = RT_EOK;
    rt_thread_t record_thread = RT_NULL;
    rt_thread_t play_thread = RT_NULL;

    // Check parameters
    if (argc != 3) {
        rt_kprintf("Usage:\n");
        rt_kprintf("\ti2s_record_play record_file.wav play_file.wav\n");
        rt_kprintf("\tFor example:\n");
        rt_kprintf("\t\ti2s_record_play record.wav play.wav\n");
        return -RT_EINVAL;
    }

    // Open record file
    fd_record = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC);
    if (fd_record < 0) {
        rt_kprintf("open record file failed!\n");
        return -RT_ERROR;
    }

    // Create empty WAV header
    struct wav_header empty_header;
    rt_memset(&empty_header, 0, sizeof(empty_header));
    write(fd_record, &empty_header, sizeof(empty_header));

    // Open play file
    fd_play = open(argv[2], O_RDONLY);
    if (fd_play < 0) {
        rt_kprintf("open play file failed!\n");
        ret = -RT_ERROR;
        goto __exit;
    }

    // Find audio device
    g_audio_dev = rt_device_find(SOUND_DEVICE_NAME);
    if (!g_audio_dev) {
        rt_kprintf("%s not found!\n", SOUND_DEVICE_NAME);
        ret = -RT_ERROR;
        goto __exit;
    }

    // Open device
    rt_device_open(g_audio_dev, RT_DEVICE_OFLAG_RDWR);

    // Initialize global variables
    g_total_record_length = 0;
    g_play_finished = RT_FALSE;
    g_record_finished = RT_FALSE;

    // Create semaphores
    g_rx_sem = rt_sem_create("rx_sem", 0, RT_IPC_FLAG_FIFO);
    g_tx_sem = rt_sem_create("tx_sem", PLAY_BUFFER_COUNT, RT_IPC_FLAG_FIFO);

    // Create record thread
    record_thread = rt_thread_create("record_thread",
                                     record_thread_entry,
                                     &fd_record,
                                     2048,
                                     8,  // Higher priority
                                     10);

    if (record_thread != RT_NULL) {
        rt_thread_startup(record_thread);
    } else {
        rt_kprintf("create record thread failed!\n");
        ret = -RT_ERROR;
        goto __exit;
    }

    // Create play thread
    play_thread = rt_thread_create("play_thread",
                                   play_thread_entry,
                                   &fd_play,
                                   2048,
                                   7,  // Higher priority
                                   10);

    if (play_thread != RT_NULL) {
        rt_thread_startup(play_thread);
    } else {
        rt_kprintf("create play thread failed!\n");
        ret = -RT_ERROR;
        goto __exit;
    }

    // Wait for both threads to finish
    while (!g_play_finished || !g_record_finished) {
        rt_thread_mdelay(100);
    }

    // Wait for threads to end
    rt_thread_mdelay(100);

__exit:
    // Close device
    if (g_audio_dev) {
        rt_device_close(g_audio_dev);
        g_audio_dev = RT_NULL;
    }

    // Close files
    if (fd_record >= 0) {
        close(fd_record);
    }

    if (fd_play >= 0) {
        close(fd_play);
    }

    // Delete semaphores
    if (g_rx_sem) {
        rt_sem_delete(g_rx_sem);
        g_rx_sem = RT_NULL;
    }

    if (g_tx_sem) {
        rt_sem_delete(g_tx_sem);
        g_tx_sem = RT_NULL;
    }

    // Reset global state
    g_play_finished = RT_FALSE;
    g_record_finished = RT_FALSE;

    return ret;
}

MSH_CMD_EXPORT_ALIAS(test_i2s_record_play, i2s_record_play, record and play simultaneously via i2s);
