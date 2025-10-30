/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui <huahui.mai@artinchip.com>
 */

#include "lv_aic_spi.h"
#include "aic_iopoll.h"

#define SPI_DEV_NAME "spidev"

#define SPI_BUF_NAME "spi2"
#define SPI_MODE     (RT_SPI_MODE_0 | RT_SPI_MSB)
#define SPI_MAX_HZ   70000000

#define RS_PIN      "PB.11"
#define BL_PIN      "PC.0"

#define BL_LOW_ACTIVE 1

#if BL_LOW_ACTIVE
#define BL_ACTIVE_LEVEL PIN_LOW
#else
#define BL_ACTIVE_LEVEL PIN_HIGH
#endif

#define LV_SPI_USE_TE   0
#define TE_PIN          "PB.10"
#define TE_TIMEOUT_MS   100

#define ALIGN_8B(x) (((x) + (7)) & ~(7))

static struct lv_spi_dev *g_spi_dev = NULL;

static void lv_disp_data_blt(struct lv_spi_dev *spi_dev);

void lv_spi_write_buffer(struct lv_spi_dev *spi_dev, unsigned int cmd, unsigned int len, const u8 *data)
{
    struct rt_spi_device *spi = (struct rt_spi_device *)spi_dev->dev;
    int ret;

    rt_spi_take_bus(spi);

    rt_pin_write(spi_dev->rs_pin, PIN_LOW);
    rt_spi_transfer(spi, (u8[]){ cmd }, NULL, 1);

    rt_pin_write(spi_dev->rs_pin, PIN_HIGH);
    if (len != 0) {
        ret = rt_spi_transfer(spi, (void *)data, NULL, len);
        if (ret != len)
            LV_LOG_ERROR("Send spi data failed. ret 0x%x\n", (int)ret);
    }

    rt_spi_release_bus(spi);
}

void lv_spi_flush(u8 *data, unsigned int len)
{
    struct lv_spi_dev *spi_dev = g_spi_dev;
    struct rt_spi_device *spi = (struct rt_spi_device *)spi_dev->dev;

    spi_dev->data = data;
    spi_dev->len = len;

    /*
     * The first frame is sent throught synchronous interface.
     * We need to wait completion in the second frame.
     */
    if (spi_dev->frame_count == 2)
        rt_spi_wait_completion(spi);
    else
        spi_dev->frame_count++;

    aicos_sem_give(spi_dev->display_sem);
}

static int lv_spi_init(struct lv_spi_dev *spi_dev)
{
    struct rt_spi_configuration spi_cfg;
    struct rt_spi_device *spi_device = NULL;
    struct rt_device *dev = NULL;
    rt_err_t result = RT_EOK;

    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (spi_device == RT_NULL) {
        LV_LOG_ERROR("rt malloc spi device failed.\n");
        return -RT_ERROR;
    }

    result = rt_spi_bus_attach_device(spi_device, SPI_DEV_NAME, spi_dev->bus_name, NULL);
    if (result != RT_EOK && spi_device != NULL) {
        LV_LOG_ERROR("rt spi bus attach device failed.\n");
        result = -RT_ERROR;
        goto err;
    }

    spi_device = (struct rt_spi_device *)rt_device_find(SPI_DEV_NAME);
    if (!spi_device) {
        LV_LOG_ERROR("Failed to get device in %s\n", SPI_DEV_NAME);
        result = -RT_ERROR;
        goto err;
    }

    dev = (struct rt_device *)spi_device;
    if (dev->type != RT_Device_Class_SPIDevice) {
        spi_device = NULL;
        LV_LOG_ERROR("%s is not SPI device.\n", SPI_DEV_NAME);
        result = -RT_ERROR;
        goto err;
    }

    rt_memset(&spi_cfg, 0, sizeof(spi_cfg));
    spi_cfg.mode = spi_dev->mode;
    spi_cfg.max_hz = spi_dev->max_hz;
    result = rt_spi_configure(spi_device, &spi_cfg);
    if (result < 0) {
        LV_LOG_ERROR("qspi configure failure.\n");
        result = -RT_ERROR;
        goto err;
    }

    spi_dev->dev = spi_device;
    return result;

err:
    if (spi_dev)
        rt_free(spi_dev);

    spi_dev = NULL;
    return result;
}

static void lv_disp_data_blt(struct lv_spi_dev *spi_dev)
{
    struct ge_bitblt blt = { 0 };
    struct mpp_ge *ge2d_dev = spi_dev->ge2d_dev;

    blt.src_buf.buf_type    = MPP_PHY_ADDR;
    blt.src_buf.phy_addr[0] = (uint32_t)(ulong)spi_dev->data;
    blt.src_buf.stride[0]   = spi_dev->info.stride;
    blt.src_buf.size.width  = spi_dev->info.width;
    blt.src_buf.size.height = spi_dev->info.height;
    blt.src_buf.format      = spi_dev->info.format;

    blt.dst_buf.buf_type    = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = (uint32_t)(ulong)spi_dev->tx_buf;
    blt.dst_buf.stride[0]   = ALIGN_8B(spi_dev->info.width * 2); // rgb565 bpp = 2;
    blt.dst_buf.format      = MPP_FMT_RGB_565;
    blt.dst_buf.size.width  = spi_dev->info.width;
    blt.dst_buf.size.height = spi_dev->info.height;

    blt.ctrl.dither_en      = 1;

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

    lv_draw_sw_rgb565_swap(spi_dev->tx_buf, spi_dev->info.width * spi_dev->info.height);
    aicos_dcache_clean_invalid_range((ulong *)spi_dev->tx_buf, (ulong)ALIGN_UP(spi_dev->tx_len, CACHE_LINE_SIZE));
}

#if LV_SPI_USE_TE
static void te_input_irq_handler(void *args)
{
    struct lv_spi_dev *spi_dev = args;

    aicos_wqueue_wakeup(spi_dev->te_queue);
}
#endif

static int te_input_pin_cfg(struct lv_spi_dev *spi_dev)
{
#if LV_SPI_USE_TE
    u32 pin = 0;
    pin = rt_pin_get(TE_PIN);
    if (pin < 0) {
        LV_LOG_ERROR("failed to get TE pin\n");
        return pin;
    }

    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(pin, PIN_IRQ_MODE_RISING_FALLING,
                      te_input_irq_handler, spi_dev);

    rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);
#endif
    return 0;
}

static inline void *lv_spi_create_te_queue(void)
{
#if LV_SPI_USE_TE
    return aicos_wqueue_create();
#else
    return NULL;
#endif
}

static void disp_thread(void *arg)
{
    struct lv_spi_dev *spi_dev = arg;
    struct rt_spi_device *spi = (struct rt_spi_device *)spi_dev->dev;
    static bool bl_en = false;

    while (1) {
        aicos_sem_take(spi_dev->display_sem, AICOS_WAIT_FOREVER);

        lv_disp_data_blt(spi_dev);

        if (bl_en && spi_dev->te_queue) {
            int ret = aicos_wqueue_wait(spi_dev->te_queue, TE_TIMEOUT_MS);
            if (ret < 0)
                    LV_LOG_ERROR("SPI wait TE irq timeout, ret: %d\n", ret);
        }

        rt_spi_nonblock_set(spi, 0);
        rt_pin_write(spi_dev->rs_pin, PIN_LOW);
        lv_spi_write_seq(spi_dev, 0x2c);

        if (bl_en)
            /* flush first pixel frame in synchronous mode */
            rt_spi_nonblock_set(spi, 1);

        rt_pin_write(spi_dev->rs_pin, PIN_HIGH);
        rt_spi_transfer(spi, spi_dev->tx_buf, NULL, spi_dev->tx_len);

        if (!bl_en) {
            /*
             * The first frame is sent throught the spi synchronous interface, and after
             * transmission is completed, switches to asynchronous and enables backlight.
             */
            rt_pin_write(spi_dev->bl_pin, BL_ACTIVE_LEVEL);

            bl_en = true;
            te_input_pin_cfg(spi_dev);
        }
    }
}

static struct lv_spi_dev *lv_spi_setup(void)
{
    struct lv_spi_dev *spi = NULL;
    rt_base_t pin;

    spi = rt_malloc(sizeof(struct lv_spi_dev));
    if (!spi) {
        LV_LOG_ERROR("malloc spi dev failed\n");
        return NULL;
    }
    rt_memset(spi, 0, sizeof(*spi));

    spi->bus_name = SPI_BUF_NAME;
    spi->mode     = SPI_MODE;
    spi->max_hz   = SPI_MAX_HZ;

    pin = rt_pin_get(RS_PIN);
    if (pin < 0) {
        LV_LOG_ERROR("get spi rs pin failed\n");
        rt_free(spi);
        return NULL;
    }

    rt_pin_mode(pin, PIN_MODE_OUTPUT);
    rt_pin_write(pin, PIN_LOW);
    spi->rs_pin = pin;

    spi->bl_pin = rt_pin_get(BL_PIN);
    if (spi->bl_pin < 0) {
        LV_LOG_ERROR("get spi bl pin failed\n");
        rt_free(spi);
        return NULL;
    }
    rt_pin_mode(spi->bl_pin, PIN_MODE_OUTPUT);
    rt_pin_write(spi->bl_pin, PIN_HIGH);

    g_spi_dev = spi;

    spi->display_sem = aicos_sem_create(0);
    spi->send_sem = aicos_sem_create(0);
    spi->te_queue = lv_spi_create_te_queue();
    spi->frame_count = 0;

    struct mpp_fb *fb = mpp_fb_open();
    mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &spi->info);

    int tx_size = spi->info.width * spi->info.height * 2;

    spi->tx_buf = aicos_malloc_align(MEM_CMA, tx_size, CACHE_LINE_SIZE);
    if (!spi->tx_buf) {
        LV_LOG_ERROR("malloc display buf failed\n");
        rt_free(spi);
        return NULL;
    }
    aicos_dcache_clean_invalid_range((ulong *)spi->tx_buf, (ulong)ALIGN_UP(tx_size, CACHE_LINE_SIZE));

    spi->tx_len = tx_size;
    spi->ge2d_dev = mpp_ge_open();

    if (lv_spi_init(spi) < 0) {
        rt_free(spi);

        if (spi->tx_buf)
            aicos_free_align(MEM_CMA, spi->tx_buf);

        return NULL;
    }

    return spi;
}

void lv_spi_screen_enable(void)
{
    struct lv_spi_dev *dev = NULL;
    aicos_thread_t thid = NULL;

    dev = lv_spi_setup();
    if (!dev) {
        LV_LOG_ERROR("spi setup failed\n");
        return;
    }

    lv_spi_panel_enable(dev);

    thid = aicos_thread_create("spi_disp", 8192, 15, disp_thread, dev);
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

static void spi_disp_poweron(void)
{
    static bool first_frame = true;

    if (first_frame) {

        lv_spi_screen_enable();
        first_frame = false;
    }
}

static void spi_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    spi_disp_t *spi_disp = (spi_disp_t *)lv_display_get_user_data(disp);
    lv_draw_buf_t *disp_buf = lv_display_get_buf_active(disp);
    (void)px_map;

    if (lv_disp_flush_is_last(disp)) {
        spi_disp_poweron();

        aicos_dcache_clean_invalid_range((ulong *)disp_buf->data, (ulong)ALIGN_UP(spi_disp->info.smem_len, CACHE_LINE_SIZE));
        lv_spi_flush(disp_buf->data, spi_disp->info.smem_len);
    }

    lv_display_flush_ready(disp);
}

void lv_spi_display_init(void)
{
    spi_disp_t *spi_disp = NULL;

    spi_disp = (spi_disp_t *)lv_malloc_zeroed(sizeof(spi_disp_t));
    if (!spi_disp) {
        LV_LOG_ERROR("malloc aic display failed");
        goto err;
    }

    spi_disp->fb = mpp_fb_open();
    if (!spi_disp->fb) {
        LV_LOG_ERROR("open mpp fb failed");
        goto err;
    }
    mpp_fb_ioctl(spi_disp->fb, AICFB_GET_SCREENINFO, &spi_disp->info);

    int width = spi_disp->info.width;
    int height = spi_disp->info.height;
    int fb_size = spi_disp->info.smem_len;

    spi_disp->buf1 = aicos_malloc_align(MEM_CMA, fb_size, CACHE_LINE_SIZE);
    if (!spi_disp->buf1) {
        LV_LOG_ERROR("malloc display buf1 failed");
        goto err;
    }

    spi_disp->buf2 = aicos_malloc_align(MEM_CMA, fb_size, CACHE_LINE_SIZE);
    if (!spi_disp->buf2) {
        LV_LOG_ERROR("malloc display buf2 failed");
        goto err;
    }

    lv_color_format_t cf = lv_display_fmt(spi_disp->info.format);
    if (cf == LV_COLOR_FORMAT_UNKNOWN)
        goto err;

    lv_display_t *disp = lv_display_create(width, height);
    lv_display_set_color_format(disp, cf);
    lv_display_set_flush_cb(disp, spi_disp_flush);
    lv_display_set_user_data(disp, spi_disp);

    lv_display_set_buffers(disp, spi_disp->buf1, spi_disp->buf2, fb_size, LV_DISPLAY_RENDER_MODE_DIRECT);

    spi_disp->disp = disp;

    return;

err:
    if (spi_disp)
        lv_free(spi_disp);
    if (spi_disp->buf1)
        lv_free(spi_disp->buf1);
    if (spi_disp->buf2)
        lv_free(spi_disp->buf2);

    LV_LOG_ERROR("create lv spi display failed");
}
