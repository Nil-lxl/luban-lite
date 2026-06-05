/*
 * Copyright (C) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui <huahui.mai@artinchip.com>
 *           Zhengcun <zhengcun.chen@artinchip.com>
 */

#include <stdatomic.h>
#include "lv_aic_spi.h"
#include "aic_iopoll.h"

#define TE_TIMEOUT_MS   100

#define ALIGN_8B(x) (((x) + (7)) & ~(7))

#ifdef  LV_SPI_BUS_WIDTH_1
static const struct lv_spi_config spi_configs[] = {
    [0] = {
        .spi_cfg = {
            .bus_name = "spi1",
            .dev_name = "spi1dev",
            .max_hz = 25000000,
            .mode = RT_SPI_MODE_0 | RT_SPI_MSB,
            .bus_width = 1,
            .rs_pin = "PD.11",
        },
        .bl_pin = "PE.12",
        .bl_low_active = false,
        .width = LV_LCD_WIDTH,
        .height = LV_LCD_HEIGHT,
    },
};
#elif defined LV_SPI_BUS_WIDTH_4
extern struct qspi_cfg qspi;
static const struct lv_spi_config spi_configs[] = {
    [0] = {
        .spi_cfg = {
            .bus_name = "qspi1",
            .dev_name = "qspi1dev",
            .max_hz = 50000000,
            .mode = RT_SPI_MODE_0 | RT_SPI_MSB,
            .bus_width = 4,
            .qspi_cfg = &qspi,
        },
        .bl_pin = "PE.12",
        .bl_low_active = false,
        .width = LV_LCD_WIDTH,
        .height = LV_LCD_HEIGHT,
    },
    [1] = {
        .spi_cfg = {
            .bus_name = "qspi2",
            .dev_name = "qspi2dev",
            .max_hz = 50000000,
            .mode = RT_SPI_MODE_0 | RT_SPI_MSB,
            .bus_width = 4,
            .qspi_cfg = &qspi,
        },
        .bl_pin = "PE.12",
        .bl_low_active = false,
        .width = LV_LCD_WIDTH,
        .height = LV_LCD_HEIGHT,
    },
};
#endif /* LV_SPI_BUS_WIDTH_4 */

#define LV_SPI_DEV_NUM    ARRAY_SIZE(spi_configs)

/* Array to hold all SPI display devices */
static struct lv_spi_dev **g_spi_devs = NULL;

static struct lv_spi_dev *lv_find_spi_dev(lv_display_t *disp)
{
    int i;

    for (i = 0; i < LV_SPI_DEV_NUM; i++) {
        if (g_spi_devs[i]->disp == disp) {
            return g_spi_devs[i];
        }
    }

    return NULL;
}

void lv_spi_write_buffer(struct lv_spi_dev *spi_dev, unsigned int cmd,
                         unsigned int len, const u8 *data)
{
    aic_spi_lcd_write_cmd(spi_dev->dev, cmd, len, data);
}

static void lv_disp_data_blt(struct lv_spi_dev *spi_dev)
{
    extern enum mpp_pixel_format lv_fmt_to_mpp_fmt(lv_color_format_t cf);
    struct mpp_ge *ge2d_dev = spi_dev->ge2d_dev;
    lv_disp_t *disp = spi_dev->disp;
    uint32_t src_buf = (uint32_t)(ulong)spi_dev->data;
    uint32_t dest_buf = (uint32_t)(ulong)spi_dev->tx_buf;
    int32_t src_width = lv_display_get_horizontal_resolution(disp);
    int32_t src_height = lv_display_get_vertical_resolution(disp);
    lv_color_format_t cf = lv_display_get_color_format(disp);
    int32_t src_stride = lv_draw_buf_width_to_stride(src_width, cf);
    int32_t dst_stride = (int32_t)ALIGN_8B(src_width * 2); // rgb565 bpp = 2;
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);
    enum mpp_pixel_format fmt = lv_fmt_to_mpp_fmt(cf);

    struct ge_bitblt blt = { 0 };

    blt.src_buf.buf_type    = MPP_PHY_ADDR;
    blt.src_buf.phy_addr[0] = src_buf;
    blt.src_buf.stride[0]   = src_stride;
    blt.src_buf.size.width  = src_width;
    blt.src_buf.size.height = src_height;
    blt.src_buf.format      = fmt;

    blt.dst_buf.buf_type    = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = dest_buf;
    blt.dst_buf.stride[0]   = dst_stride;
    blt.dst_buf.format      = MPP_FMT_RGB_565;

    if (rotation == LV_DISPLAY_ROTATION_0 || rotation == LV_DISPLAY_ROTATION_180) {
        blt.dst_buf.size.width  = src_width;
        blt.dst_buf.size.height = src_height;
    } else {
        blt.dst_buf.size.width  = src_height;
        blt.dst_buf.size.height = src_width;
    }
    blt.ctrl.dither_en      = 1;

    switch (rotation) {
    case LV_DISPLAY_ROTATION_0:
        blt.ctrl.flags = MPP_ROTATION_0;
        break;
    case LV_DISPLAY_ROTATION_90:
        /* LV_DISP_ROT_90 means display rotate 90 degrees counterclockwise,
         * so set degree to MPP_ROTATION_270
         */
        blt.ctrl.flags = MPP_ROTATION_270;
        break;
    case LV_DISPLAY_ROTATION_180:
        blt.ctrl.flags = MPP_ROTATION_180;
        break;
    case LV_DISPLAY_ROTATION_270:
        blt.ctrl.flags = MPP_ROTATION_90;
        break;
    default:
        break;
    }

    int ret = mpp_ge_bitblt(ge2d_dev, &blt);
    if (ret < 0) {
        LV_LOG_ERROR("mpp ge bitblt fail\n");
        return;
    }

    ret = mpp_ge_emit(ge2d_dev);
    if (ret < 0) {
        LV_LOG_ERROR("mpp ge emit fail\n");
        return;
    }

    ret = mpp_ge_sync(ge2d_dev);
    if (ret < 0) {
        LV_LOG_ERROR("mpp ge sync fail\n");
        return;
    }

    lv_draw_sw_rgb565_swap(spi_dev->tx_buf, src_width * src_height);
    aicos_dcache_clean_invalid_range((ulong *)spi_dev->tx_buf, (ulong)ALIGN_UP(spi_dev->tx_len, CACHE_LINE_SIZE));
}

static void te_input_irq_handler(void *args)
{
    struct lv_spi_dev *spi_dev = args;

    aicos_wqueue_wakeup(spi_dev->te_queue);
}

static int te_input_pin_cfg(struct lv_spi_dev *spi_dev)
{
    if (spi_dev->te_queue && spi_dev->te_pin) {
        rt_pin_mode(spi_dev->te_pin, PIN_MODE_INPUT_PULLUP);
        rt_pin_attach_irq(spi_dev->te_pin, PIN_IRQ_MODE_RISING_FALLING,
                        te_input_irq_handler, spi_dev);

        rt_pin_irq_enable(spi_dev->te_pin, PIN_IRQ_ENABLE);
    }

    return 0;
}

static void disp_thread(void *arg)
{
    struct lv_spi_dev *spi_dev = arg;
    const struct lv_spi_config *config = spi_dev->config;

    while (1) {
        aicos_sem_take(spi_dev->display_sem, AICOS_WAIT_FOREVER);

        lv_disp_data_blt(spi_dev);

        if (spi_dev->bl_en && spi_dev->te_queue) {
            int ret = aicos_wqueue_wait(spi_dev->te_queue, TE_TIMEOUT_MS);
            if (ret < 0)
                LV_LOG_ERROR("SPI wait TE irq timeout, ret: %d\n", ret);
        }

        aic_spi_lcd_flush(spi_dev->dev, spi_dev->tx_buf, spi_dev->tx_len);
        aic_spi_lcd_wait_completion(spi_dev->dev);

        if (!spi_dev->bl_en) {
            aic_spi_lcd_backlight_gpio_enable(config->bl_pin, config->bl_low_active, true);

            spi_dev->bl_en = true;
            te_input_pin_cfg(spi_dev);
        }

        aicos_sem_give(spi_dev->sync_ready);
    }
}

static struct lv_spi_dev *lv_spi_setup(int id, int width, int height, int fb_size)
{
    const struct lv_spi_config *config = &spi_configs[id];
    struct lv_spi_dev *spi = NULL;
    int tx_size;

    spi = rt_malloc(sizeof(struct lv_spi_dev));
    if (!spi) {
        LV_LOG_ERROR("malloc spi dev failed\n");
        return NULL;
    }
    rt_memset(spi, 0, sizeof(*spi));

    spi->id = id;
    spi->fb_size = fb_size;
    spi->power_on = false;

    if (config->te_pin) {
        spi->te_pin = rt_pin_get(config->te_pin);
        if (spi->te_pin < 0) {
            LV_LOG_ERROR("get spi te pin %s failed\n", config->te_pin);
            rt_free(spi);
            return NULL;
        }

        spi->te_queue = aicos_wqueue_create();
    }

    spi->display_sem = aicos_sem_create(0);
    spi->sync_ready = aicos_sem_create(0);
    spi->frame_count = 0;
    spi->bl_en = false;

    spi->ge2d_dev = mpp_ge_open();
    spi->config = config;

    spi->dev = aic_spi_lcd_init(&config->spi_cfg);
    if (!spi->dev) {
        pr_err("aic spi lcd init failed\n");
        rt_free(spi);
        return NULL;
    }

    /* default trtransfer RGB565 framebuffer data */
    tx_size = width * height * 2;
    spi->tx_len = tx_size;
    spi->tx_buf = aicos_malloc_align(MEM_CMA, tx_size, CACHE_LINE_SIZE);
    if (!spi->tx_buf) {
        LV_LOG_ERROR("malloc display buf failed\n");
        rt_free(spi);
        return NULL;
    }
    aicos_dcache_clean_invalid_range((ulong *)spi->tx_buf, (ulong)ALIGN_UP(tx_size, CACHE_LINE_SIZE));

    return spi;
}

void lv_spi_screen_enable(struct lv_spi_dev *spi_dev)
{
    aicos_thread_t thid = NULL;

    lv_spi_panel_enable(spi_dev);

    thid = aicos_thread_create(spi_dev->config->spi_cfg.bus_name,
                               8192, 20 + spi_dev->id, disp_thread, spi_dev);
    if (thid == NULL)
        LV_LOG_ERROR("Failed to create display thread\n");
}

static lv_color_format_t lv_display_fmt(enum mpp_pixel_format cf)
{
    lv_color_format_t fmt = LV_COLOR_FORMAT_ARGB8888;
    switch(cf) {
        case MPP_FMT_RGB_565:
            fmt = LV_COLOR_FORMAT_RGB565;
            break;
        case MPP_FMT_RGB_888:
            fmt = LV_COLOR_FORMAT_RGB888;
            break;
        case MPP_FMT_ARGB_8888:
            fmt = LV_COLOR_FORMAT_ARGB8888;
            break;
        case MPP_FMT_XRGB_8888:
            fmt = LV_COLOR_FORMAT_XRGB8888;
            break;
        default:
            LV_LOG_ERROR("unsupported format:%d", cf);
            break;
    }
    return fmt;
}
static void spi_dev_poweron(struct lv_spi_dev *spi_dev)
{
    if (!spi_dev->power_on) {
        lv_spi_screen_enable(spi_dev);
        spi_dev->power_on = true;
    }
}

void lv_spi_flush(lv_display_t *disp, u8 *data)
{
    struct lv_spi_dev *spi_dev = lv_find_spi_dev(disp);

    if (!spi_dev)
        return;

    if (spi_dev->frame_count >= 1)
        aicos_sem_take(spi_dev->sync_ready, AICOS_WAIT_FOREVER);
    else
        spi_dev->frame_count++;

    spi_dev->data = data;

    aicos_sem_give(spi_dev->display_sem);
}

static void spi_dev_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    lv_draw_buf_t *disp_buf = lv_display_get_buf_active(disp);
    struct lv_spi_dev *spi_dev = lv_find_spi_dev(disp);
    (void)px_map;

    if (!spi_dev)
        return;

    if (lv_disp_flush_is_last(disp)) {
        spi_dev_poweron(spi_dev);
        aicos_dcache_clean_invalid_range((ulong *)disp_buf->data, (ulong)ALIGN_UP(spi_dev->fb_size, CACHE_LINE_SIZE));
        lv_spi_flush(disp, disp_buf->data);
    }

    lv_display_flush_ready(disp);
}

int lv_spi_display_init(int use_frame_buffer)
{
    void *buf1 = NULL, *buf2 = NULL;
    struct aicfb_screeninfo info;
    struct mpp_fb *fb = NULL;
    int width, height, fb_size;
    int i;

    fb = mpp_fb_open();
    if (!fb) {
        LV_LOG_ERROR("open mpp fb failed");
        return -1;
    }
    mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &info);

    lv_color_format_t cf = lv_display_fmt(info.format);
    if (cf == LV_COLOR_FORMAT_UNKNOWN)
        return -1;

    g_spi_devs = lv_malloc_zeroed(sizeof(struct lv_spi_dev *) * LV_SPI_DEV_NUM);
    if (!g_spi_devs) {
        LV_LOG_ERROR("malloc spi devs array failed");
        return -1;
    }

    for (i = 0; i < LV_SPI_DEV_NUM; i++) {
        width = spi_configs[i].width ? spi_configs[i].width : info.width;
        height = spi_configs[i].height ? spi_configs[i].height : info.height;

        width = ALIGN_8B(width);
        fb_size = width * height * lv_color_format_get_size(cf);

        struct lv_spi_dev *spi_dev = lv_spi_setup(i, width, height, fb_size);
        if (!spi_dev) {
            LV_LOG_ERROR("spi setup failed for device %d", i);
            goto err;
        }
        g_spi_devs[i] = spi_dev;

        if (i == 0 && use_frame_buffer) {
            buf1 = info.framebuffer;
#ifdef AIC_PAN_DISPLAY
            buf2 = info.framebuffer + info.smem_len;
#else
            buf2 = NULL;
#endif
        } else {
            buf1 = aicos_malloc_align(MEM_CMA, fb_size, CACHE_LINE_SIZE);
            if (!buf1) {
                LV_LOG_ERROR("malloc display buf1 failed");
                goto err;
            }
            buf2 = aicos_malloc_align(MEM_CMA, fb_size, CACHE_LINE_SIZE);
            if (!buf2) {
                LV_LOG_ERROR("malloc display buf2 failed");
                goto err;
            }
        }

        lv_display_t *disp = lv_display_create(width, height);
        lv_display_set_color_format(disp, cf);
        lv_display_set_flush_cb(disp, spi_dev_flush);

        lv_display_set_buffers(disp, buf1, buf2, fb_size, LV_DISPLAY_RENDER_MODE_DIRECT);
#if defined(LV_DISPLAY_ROTATE_EN) && defined(AIC_LVGL_DOUBLE_DISP_DEMO)
        lv_display_set_rotation(disp, LV_SECOND_ROTATE_DEGREE / 90);
#elif defined(LV_DISPLAY_ROTATE_EN) && defined(LV_USE_SPI_REPLACE_MIPI_DBI)
        lv_display_set_rotation(disp, LV_ROTATE_DEGREE / 90);
#endif

        spi_dev->disp = disp;
    }

    return 0;

err:
    if (g_spi_devs) {
        for (i = 0; i < LV_SPI_DEV_NUM; i++) {
            if (g_spi_devs[i]) {
                if (i == 0 && use_frame_buffer)
                    continue;

                if (g_spi_devs[i]->tx_buf)
                    aicos_free_align(MEM_CMA, g_spi_devs[i]->tx_buf);

                if (g_spi_devs[i]->ge2d_dev)
                    mpp_ge_close(g_spi_devs[i]->ge2d_dev);

                rt_free(g_spi_devs[i]);
            }
        }
        lv_free(g_spi_devs);
        g_spi_devs = NULL;
    }

    if (fb)
        mpp_fb_close(fb);

    LV_LOG_ERROR("create lv spi display failed");
    return -1;
}
