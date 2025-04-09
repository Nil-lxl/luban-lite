/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <hal_audio.h>
#include <unistd.h>
#include "ringbuffer.h"
#include "sound.h"

#ifdef LPKG_USING_DFS
#include <dfs.h>
#include <dfs_fs.h>
#ifdef LPKG_USING_DFS_ELMFAT
#include <dfs_elm.h>
#endif
#endif

#define TX_FIFO_SIZE                    4096
#define TX_FIFO_PERIOD_COUNT            2
#define BUFFER_SIZE                     2048
aic_audio_ctrl audio_ctrl;
ringbuf_t *ring_buf;
rt_uint8_t audio_tx_fifo[TX_FIFO_SIZE] __attribute__((aligned(64)));
int fd;
uint8_t *buffer = NULL;
volatile uint8_t read_done, start_flag;

void ring_buffer_fill(void)
{
    int len = 0;
    int wr_cnt;

    if (start_flag)
    {
        len = read(fd, buffer, BUFFER_SIZE);
        if (len <= 0)
            read_done = 1;
        else
        {
            while (len)
            {
                wr_cnt = ringbuf_in(ring_buf, buffer, len);
                len -= wr_cnt;
            }
        }
    }
}

static void drv_audio_callback(aic_audio_ctrl *pcodec, void *arg)
{
    unsigned long event = (unsigned long)arg;

    switch (event)
    {
    case AUDIO_TX_PERIOD_INT:
        ring_buf->read += pcodec->tx_info.buf_info.period_len;
        ring_buf->data_len -= pcodec->tx_info.buf_info.period_len;
        ring_buffer_fill();
        aicos_dcache_clean_range((void *)audio_tx_fifo, TX_FIFO_SIZE);
        break;
    default:
        hal_log_err("%s(%d)\n", __func__, __LINE__);
        break;
    }

}

int wav_file_parse(int fd, struct wav_header *info)
{
    if (fd < 0)
        return fd;

    if (read(fd, info, sizeof(struct wav_header)) <= 0)
    {
        hal_log_err("read info error\n");
        return -EINVAL;
    }

    if (info->fmt_bit_per_sample != 16)
    {
        hal_log_err("audio only support 16 bits samplebits!!!\n");
        return -EINVAL;
    }

    printf("wav information:\n");
    printf("samplerate: %d\n", info->fmt_sample_rate);
    printf("channel: %d\n", info->fmt_channels);
    printf("samplebits: %d\n", info->fmt_bit_per_sample);

    return 0;
}

void audio_config(aic_audio_ctrl *pcodec, struct wav_header *info)
{
    pcodec->tx_info.buf_info.buf = (void *)audio_tx_fifo;
    pcodec->tx_info.buf_info.buf_len = TX_FIFO_SIZE;
    pcodec->tx_info.buf_info.period_len = TX_FIFO_SIZE / TX_FIFO_PERIOD_COUNT;

    hal_audio_init(pcodec);
    hal_audio_attach_callback(pcodec, drv_audio_callback, NULL);

    /* Configure audio format */
    /* AudioCodec only support 16 bits */
    hal_audio_set_playback_channel(pcodec, info->fmt_channels);
    hal_audio_set_samplerate(pcodec, info->fmt_sample_rate);
    pcodec->config.samplerate = info->fmt_sample_rate;
    pcodec->config.channel = info->fmt_channels;
    pcodec->config.samplebits = 16;

#ifdef AIC_AUDIO_SPK_0
    hal_audio_set_playback_by_spk0(pcodec);
#endif
#ifdef AIC_AUDIO_SPK_1
    hal_audio_set_playback_by_spk1(pcodec);
#endif
#ifdef AIC_AUDIO_SPK_0_1
    hal_audio_set_playback_by_spk0(pcodec);
    hal_audio_set_playback_by_spk1(pcodec);
#ifdef AIC_AUDIO_SPK0_OUTPUT_DIFFERENTIAL
    hal_audio_set_pwm0_differential(pcodec);
#endif
#ifdef AIC_AUDIO_SPK1_OUTPUT_DIFFERENTIAL
    hal_audio_set_pwm1_differential(pcodec);
#endif
#endif
}

ringbuf_t * ringbuffer_init(void)
{
    ringbuf_t *ring_buffer;

    ring_buffer = aicos_malloc(MEM_CMA, sizeof(ringbuf_t));
    if (!ring_buffer)
    {
        hal_log_err("ring_buffer malloc error!\n");
        return NULL;
    }

    ring_buffer->buffer = audio_tx_fifo;
    ring_buffer->size = TX_FIFO_SIZE;
    ring_buffer->write = 0;
    ring_buffer->read = 0;
    ring_buffer->data_len = 0;

    return ring_buffer;
}

void pa_pin_init(int pa)
{
    unsigned int group, pin;

    pa = hal_gpio_name2pin(AIC_AUDIO_PA_ENABLE_GPIO);
    group = GPIO_GROUP(pa);
    pin = GPIO_GROUP_PIN(pa);
    hal_gpio_direction_output(group, pin);
#ifdef AIC_AUDIO_EN_PIN_HIGH
    hal_gpio_clr_output(group, pin);
#else
    hal_gpio_set_output(group, pin);
#endif
}

void pa_pin_enable(int pa)
{
    unsigned int group, pin;

    group = GPIO_GROUP(pa);
    pin = GPIO_GROUP_PIN(pa);

#ifdef AIC_AUDIO_EN_PIN_HIGH
    hal_gpio_set_output(group, pin);
#else
    hal_gpio_clr_output(group, pin);
#endif
}

void pa_pin_disable(int pa)
{
    unsigned int group, pin;

    group = GPIO_GROUP(pa);
    pin = GPIO_GROUP_PIN(pa);

#ifdef AIC_AUDIO_EN_PIN_HIGH
    hal_gpio_clr_output(group, pin);
#else
    hal_gpio_set_output(group, pin);
#endif
}

static int sound_aplay(int argc, char *argv[])
{
    int ret = 0;
    struct wav_header info = {0};
    unsigned int pa;

    if (argc != 2)
    {
        printf("Usage:\n"
               "\taplay song.wav\n");
        return 0;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        hal_log_err("open file failed!\n");
        goto __exit;
    }

    buffer = aicos_malloc(MEM_CMA, BUFFER_SIZE);
    if (!buffer)
    {
        hal_log_err("buffer malloc error!\n");
        goto __exit;
    }

    memset(audio_tx_fifo, 0, sizeof(audio_tx_fifo));

    ring_buf = ringbuffer_init();
    if (!ring_buf)
        goto __exit;

    ret = wav_file_parse(fd, &info);
    if (ret)
        goto __exit;

    audio_config(&audio_ctrl, &info);

    pa = hal_gpio_name2pin(AIC_AUDIO_PA_ENABLE_GPIO);
    pa_pin_init(pa);

    read_done = 0;
    start_flag = 0;
    hal_dma_init();
    aicos_request_irq(DMA_IRQn, hal_dma_irq, 0, NULL, NULL);

    hal_audio_playback_start(&audio_ctrl);
    /* Enable PA after a delay of 10ms to prevent pop */
    aicos_msleep(10);

    /* Enable Power Amplifier */
    pa_pin_enable(pa);
    /* Wait Power Amplifier stable */
    aicos_msleep(200);
    start_flag = 1;

    while (!read_done)
    {

    }

    pa_pin_disable(pa);
    hal_audio_playback_stop(&audio_ctrl);

__exit:
    if (fd >= 0)
        close(fd);
    if (buffer != NULL)
        aicos_free(MEM_CMA, buffer);
    if (ring_buf != NULL)
        aicos_free(MEM_CMA, ring_buf);

    return 0;
}

CONSOLE_CMD(aplay, sound_aplay, "aplay song.wav");
