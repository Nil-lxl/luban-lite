/*
 * Copyright (c) 2026 ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  xyg <yiguan.xu@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/audio.h>
#include <aic_core.h>
#include "hal_i2s.h"

#define LOCAL_RX_BUF_SIZE     (RT_AUDIO_RECORD_PIPE_SIZE * 16)
#define TX_SAMPLE_COUNT       512
#define RX_READ_CHUNK_SIZE    2048
#define SYNC_WORD             0x5AA55AA5U
#define RX_CAPTURE_TIMEOUT_MS 5000
#define MAX_MISMATCH_PRINT    8
#define MIN(x, y)             ((x) < (y) ? (x) : (y))
#ifdef AIC_USING_I2S0
#define I2S_IDX      0
#define SND_DEV_NAME "i2s0_sound"
#elif defined(AIC_USING_I2S1)
#define I2S_IDX      1
#define SND_DEV_NAME "i2s1_sound"
#endif

static struct {
    rt_device_t dev;
    rt_uint8_t *buf;
    rt_uint32_t capacity;
    rt_uint32_t total;
    rt_sem_t done_sem;
    rt_bool_t stop_flag;
} g_rx_ctx;

static aic_i2s_ctrl g_i2s_ctrl = { 0 };
static rt_uint32_t g_rx_buf[LOCAL_RX_BUF_SIZE / sizeof(rt_uint32_t)];

static void i2s_loopback_enable(void)
{
    hal_i2s_init(&g_i2s_ctrl, I2S_IDX);
    hal_i2s_enable_loopback(&g_i2s_ctrl);
}

static void rx_capture_entry(void *param)
{
    rt_uint32_t remaining = 0;
    rt_uint32_t rx_len = 0;

    while (!g_rx_ctx.stop_flag && g_rx_ctx.total < g_rx_ctx.capacity) {
        remaining = g_rx_ctx.capacity - g_rx_ctx.total;
        if (remaining == 0)
            break;
        rx_len = rt_device_read(g_rx_ctx.dev, 0, g_rx_ctx.buf + g_rx_ctx.total,
                                MIN(RX_READ_CHUNK_SIZE, remaining));
        if (rx_len > 0)
            g_rx_ctx.total += rx_len;
    }
    if (!g_rx_ctx.stop_flag) {
        rt_sem_release(g_rx_ctx.done_sem);
    }
}

static void build_tx_buffer(rt_uint32_t *buf, rt_uint32_t count)
{
    buf[0] = SYNC_WORD;
    for (rt_uint32_t i = 1; i < count; i++)
        buf[i] = (rt_uint32_t)(i * 5);
}

static rt_bool_t verify_loopback_data(const rt_uint32_t *rx_buf, rt_uint32_t rx_total_samples,
                                      const rt_uint32_t *tx_buf, rt_uint32_t tx_count,
                                      rt_uint32_t sync_idx)
{
    rt_uint32_t rx_avail = rx_total_samples - sync_idx;
    rt_bool_t passed = RT_TRUE;

    rt_kprintf("\n[Verify] TX samples: %u | RX samples from sync: %u\n", tx_count, rx_avail);

    if (rx_avail < tx_count) {
        rt_kprintf("[FAIL] Size mismatch: expected %u samples, only received %u\n", tx_count,
                   rx_avail);
        passed = RT_FALSE;
    } else {
        rt_kprintf("[PASS] Size check OK: received at least %u samples after sync\n", tx_count);
    }
    rt_uint32_t cmp_count = (rx_avail < tx_count) ? rx_avail : tx_count;
    rt_uint32_t mismatch = 0;

    for (rt_uint32_t i = 0; i < cmp_count; i++) {
        rt_uint32_t rx_val = rx_buf[sync_idx + i];
        rt_uint32_t tx_val = tx_buf[i];

        if (rx_val != tx_val) {
            if (mismatch < MAX_MISMATCH_PRINT)
                rt_kprintf("  [MISMATCH] [%u] tx=0x%08X  rx=0x%08X\n", i, tx_val, rx_val);
            mismatch++;
            passed = RT_FALSE;
        }
    }

    if (mismatch > MAX_MISMATCH_PRINT)
        rt_kprintf("  ... and %u more mismatch(es) omitted\n", mismatch - MAX_MISMATCH_PRINT);

    if (passed)
        rt_kprintf("[PASS] All %u samples matched\n", tx_count);
    else
        rt_kprintf("[FAIL] %u / %u sample(s) mismatched\n", mismatch, cmp_count);

    return passed;
}

static int test_audio_loopback(int argc, char *argv[])
{
    rt_uint32_t tx_buf[TX_SAMPLE_COUNT];
    struct rt_audio_caps caps;
    rt_device_t dev = RT_NULL;
    int sync_idx = -1;
    int sync_count = 0;
    int ret = 0;

    rt_memset(&g_rx_ctx, 0, sizeof(g_rx_ctx));
    rt_kprintf("\n========== I2S Loopback Test ==========\n");
    dev = rt_device_find(SND_DEV_NAME);
    if (!dev) {
        rt_kprintf("[FAIL] Device '%s' not found\n", SND_DEV_NAME);
        return -1;
    }

    if (rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK) {
        rt_kprintf("[FAIL] Device '%s' open failed\n", SND_DEV_NAME);
        return -1;
    }

    /* Configure OUTPUT path: 48 kHz, stereo, 32-bit */
    caps.main_type = AUDIO_TYPE_OUTPUT;
    caps.sub_type = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = 48000;
    caps.udata.config.channels = 2;
    caps.udata.config.samplebits = 32;
    if (rt_device_control(dev, AUDIO_CTL_CONFIGURE, &caps) != RT_EOK) {
        rt_kprintf("[FAIL] OUTPUT configure failed\n");
        rt_device_close(dev);
        return -1;
    }

    /* Configure INPUT path with identical parameters */
    caps.main_type = AUDIO_TYPE_INPUT;
    if (rt_device_control(dev, AUDIO_CTL_CONFIGURE, &caps) != RT_EOK) {
        rt_kprintf("[FAIL] INPUT configure failed\n");
        rt_device_close(dev);
        return -1;
    }

    /* Route TX output back into RX via hardware loopback */
    i2s_loopback_enable();
    /* Build the reference TX pattern */
    build_tx_buffer(tx_buf, TX_SAMPLE_COUNT);
    /* Initialise RX capture context */
    rt_memset(g_rx_buf, 0, sizeof(g_rx_buf));
    g_rx_ctx.dev = dev;
    g_rx_ctx.buf = (rt_uint8_t *)g_rx_buf;
    g_rx_ctx.capacity = LOCAL_RX_BUF_SIZE;
    g_rx_ctx.total = 0;

    g_rx_ctx.done_sem = rt_sem_create("rx_done", 0, RT_IPC_FLAG_FIFO);
    if (!g_rx_ctx.done_sem) {
        rt_kprintf("[FAIL] Semaphore creation failed\n");
        rt_device_close(dev);
        return -1;
    }
    /* Start the RX capture thread before writing so no data is missed */
    rt_thread_t rx_thread = rt_thread_create("rx_cap", rx_capture_entry, RT_NULL, 2048, 5, 5);
    if (!rx_thread) {
        rt_kprintf("[FAIL] RX thread creation failed\n");
        rt_sem_delete(g_rx_ctx.done_sem);
        rt_device_close(dev);
        return -1;
    }
    rt_thread_startup(rx_thread);

    /* Transmit the TX buffer – loopback hardware echoes it to RX */
    rt_size_t tx_written = rt_device_write(dev, 0, tx_buf, sizeof(tx_buf));
    if (tx_written != sizeof(tx_buf)) {
        rt_kprintf("[FAIL] TX write failed: expected %u bytes, wrote %u\n", sizeof(tx_buf),
                   tx_written);
        g_rx_ctx.stop_flag = RT_TRUE;
        rt_device_close(dev);
        rt_thread_mdelay(100);
        rt_thread_delete(rx_thread);
        rt_sem_delete(g_rx_ctx.done_sem);
        return -1;
    }

    /* Block until capture completes or the watchdog timeout fires */
    if (rt_sem_take(g_rx_ctx.done_sem, rt_tick_from_millisecond(RX_CAPTURE_TIMEOUT_MS)) != RT_EOK) {
        rt_kprintf("[WARN] RX capture timeout, captured %u bytes so far\n", g_rx_ctx.total);
        g_rx_ctx.stop_flag = RT_TRUE;
    }

    /* Close device first to unblock any pending rt_device_read in rx thread,
     * then wait for the thread to exit its loop before deleting it */
    rt_device_close(dev);

    rt_thread_mdelay(100);
    rt_thread_delete(rx_thread);
    rt_sem_delete(g_rx_ctx.done_sem);

    rt_uint32_t rx_samples = g_rx_ctx.total / sizeof(rt_uint32_t);
    rt_kprintf("RX captured: %u samples (%u bytes)\n", rx_samples, g_rx_ctx.total);

    /* Locate the first sync word in the received buffer */
    for (rt_uint32_t i = 0; i < rx_samples; i++) {
        if (g_rx_buf[i] == SYNC_WORD) {
            if (sync_idx < 0) {
                sync_idx = (int)i;
                rt_kprintf("Sync word first occurrence at index %d\n", sync_idx);
            }
            sync_count++;
        }
    }
    rt_kprintf("Sync word 0x%08X found %d time(s)\n", SYNC_WORD, sync_count);

    if (sync_idx < 0) {
        rt_kprintf("[FAIL] Sync word not found in RX buffer\n");
        ret = -1;
    } else {
        rt_bool_t ok = verify_loopback_data(g_rx_buf, rx_samples, tx_buf, TX_SAMPLE_COUNT,
                                            (rt_uint32_t)sync_idx);
        ret = ok ? 0 : -1;
    }

    rt_kprintf("========== Test %s ==========\n\n", ret == 0 ? "PASSED" : "FAILED");
    return ret;
}

MSH_CMD_EXPORT_ALIAS(test_audio_loopback, test_audio_loopback, I2S loopback by audio data stream);
