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

#define SPI_BUF_NAME "spi1"
#define SPI_MODE     (RT_SPI_MODE_0 | RT_SPI_MSB)
#define SPI_MAX_HZ   70000000

#define RS_PIN      "PB.11"

#define QSPI_DEV_NAME "qspidev"
#define QSPI_BUF_NAME "qspi2"
#define QSPI_MODE     (RT_SPI_MODE_0 | RT_SPI_MSB)
#define QSPI_MAX_HZ   70000000

#define QSPI_WRITE_CODE    (0xDE002C00)
#define QSPI_WRITE_LINES    1

#define QSPI_TRANSFER_CODE (0x32002C00)
#define QSPI_TRANSFER_LINES 4

/*spi control display*/
#define DATA_MODE_SINGLE 	0x0
#define DATA_MODE_DUAL		0x1
#define DATA_MODE_QUAD		0x2

#define SPI_IDMA_BL8		2

#define SPI_VSW_CMD		(0xD8006100)
#define SPI_VBP_CMD		(0xD8006000)
#define SPI_VLD_CMD		(0xDE006000)
#define SPI_VFP_CMD		(0xD8006000)

#ifdef  LV_SPI
#define LV_SPI_TYPE 1
#elif defined LV_QSPI
#define LV_SPI_TYPE 0
#endif

#ifdef LV_SPI_HARDWARE
#define USE_SPI_HARDWARE 1
#else
#define USE_SPI_HARDWARE 0
#endif

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

#define LENGTH_MAX 0x80000000

#define ALIGN_8B(x) (((x) + (7)) & ~(7))

static struct lv_spi_dev *g_spi_dev = NULL;

static void lv_disp_data_blt(struct lv_spi_dev *spi_dev);

rt_err_t lv_qspi_transfer_data(struct rt_qspi_device *device, const void *send_buf,
                               rt_size_t length, uint32_t qspi_code, int data_lines)
{
    struct rt_qspi_message message = {0};
    rt_err_t result = RT_EOK;

    RT_ASSERT(send_buf);

    message.qspi_data_lines = data_lines;

    /* If need the codecfg, can set the message.instruction*/

    /* send qspi code */
    message.address.content = qspi_code;
    message.address.size = sizeof(qspi_code);
    message.address.qspi_lines = 1;

    /* set send buf and send size */
    message.parent.send_buf = send_buf;
    message.parent.recv_buf = RT_NULL;
    message.parent.length = length;
    message.parent.cs_take = 1;
    message.parent.cs_release = 1;

    result = rt_qspi_transfer_message(device, &message);
    if (result == 0)
    {
        return -RT_EIO;
    }
    else
    {
        if ((length & LENGTH_MAX)) {
            LV_LOG_ERROR("The length is too long!!\r\n");
            return -RT_ERROR;
        }

        return length;
    }
}

void spi_write_buffer(struct lv_spi_dev *spi_dev, unsigned int cmd, unsigned int len, const u8 *data)
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

void qspi_write_buffer(struct lv_spi_dev *spi_dev, unsigned int cmd, unsigned int len, const u8 *data)
{
    struct rt_qspi_device *qspi = (struct rt_qspi_device *)spi_dev->dev;
    uint32_t qspi_code = (QSPI_WRITE_CODE & 0xFFFF00FF) | (cmd << 8);
    int ret;

    rt_spi_take_bus((struct rt_spi_device *)qspi);

    ret = lv_qspi_transfer_data(qspi, (void *)data, len, qspi_code, QSPI_WRITE_LINES);
    if (len != 0 && ret != len)
            LV_LOG_ERROR("Send spi data failed. ret 0x%x\n", (int)ret);

    rt_spi_release_bus((struct rt_spi_device *)qspi);
}

void lv_spi_write_buffer(struct lv_spi_dev *spi_dev, unsigned int cmd, unsigned int len, const u8 *data)
{
    if (spi_dev->spi_type)
        spi_write_buffer(spi_dev, cmd, len, data);
    else
        qspi_write_buffer(spi_dev, cmd, len, data);
}

void spi_flush_wait_completion(struct lv_spi_dev *spi_dev)
{
    struct rt_spi_device *spi = (struct rt_spi_device *)spi_dev->dev;

    rt_spi_wait_completion(spi);
}

void qspi_flush_wait_completion(struct lv_spi_dev *spi_dev)
{
    struct rt_qspi_device *qspi = (struct rt_qspi_device *)spi_dev->dev;

    rt_spi_wait_completion((struct rt_spi_device *)&qspi->parent);
}

void lv_spi_flush(u8 *data, unsigned int len)
{
    struct lv_spi_dev *spi_dev = g_spi_dev;
    uint32_t tick_start = 0;

    spi_dev->data = data;
    spi_dev->len = len;

    /*
     * The first frame is sent throught synchronous interface.
     * We need to wait completion in the second frame.
     */
    if (spi_dev->frame_count == 2) {
        /* Ensure the spi_disp_data_transm/qspi_disp_data_transm has been called */
        tick_start = lv_tick_get();
        while (atomic_load(&spi_dev->has_send_data) == 0) {
            if (lv_tick_elaps(tick_start) > 1000) {
                LV_LOG_ERROR("spi_disp_data_transm/qspi_disp_data_transm timeout\n");
                return;
            }
            aicos_mdelay(5);
        }
        atomic_store(&spi_dev->has_send_data, 0);

        if (spi_dev->spi_type)
            spi_flush_wait_completion(spi_dev);
        else
            qspi_flush_wait_completion(spi_dev);
    }
    else
        spi_dev->frame_count++;

    aicos_sem_give(spi_dev->display_sem);
}

static int spi_dev_init(struct lv_spi_dev *spi_dev,
                        struct rt_device *dev)
{
    struct rt_spi_configuration spi_cfg = {0};
    struct rt_spi_device *spi_device = NULL;
    int result = 0;

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

static int qspi_dev_init(struct lv_spi_dev *spi_dev,
                        struct rt_device *dev)
{
    struct rt_qspi_configuration qspi_cfg = {0};
    struct rt_qspi_device *qspi_device = NULL;
    int result = 0;

    qspi_device = (struct rt_qspi_device *)rt_malloc(sizeof(struct rt_qspi_device));
    if (qspi_device == RT_NULL) {
        LV_LOG_ERROR("rt malloc spi device failed.\n");
        return -RT_ERROR;
    }

    result = aic_qspi_bus_attach_device(spi_dev->bus_name, QSPI_DEV_NAME, 0, 4, NULL, NULL);
    if (result != RT_EOK) {
        LV_LOG_ERROR("rt qspi bus attach device failed.\n");
        result = -RT_ERROR;
        goto err;
    }

    qspi_device = (struct rt_qspi_device *)rt_device_find(QSPI_DEV_NAME);
    if (!qspi_device) {
        LV_LOG_ERROR("Failed to get device in %s\n", QSPI_DEV_NAME);
        result = -RT_ERROR;
        goto err;
    }

    dev = (struct rt_device *)qspi_device;
    if (dev->type != RT_Device_Class_SPIDevice) {
        qspi_device = NULL;
        LV_LOG_ERROR("%s is not SPI device.\n", QSPI_DEV_NAME);
        result = -RT_ERROR;
        goto err;
    }

    qspi_cfg.parent.mode = spi_dev->mode;
    qspi_cfg.parent.max_hz = spi_dev->max_hz;
    result = rt_qspi_configure(qspi_device, &qspi_cfg);
    if (result < 0) {
        LV_LOG_ERROR("qspi configure failure.\n");
        result = -RT_ERROR;
        goto err;
    }

    spi_dev->dev = qspi_device;

    return result;

err:
    if (spi_dev)
        rt_free(spi_dev);

    spi_dev = NULL;
    return result;
}

static int lv_spi_init(struct lv_spi_dev *spi_dev)
{
    struct rt_device *dev = NULL;
    int result = 0;

    if (spi_dev->spi_type)
        result = spi_dev_init(spi_dev, dev);
    else
        result = qspi_dev_init(spi_dev, dev);

    return result;
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
    int32_t dst_stride = (int32_t)ALIGN_8B(spi_dev->info.width * 2); // rgb565 bpp = 2;
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

#if USE_SPI_HARDWARE
    blt.dst_buf.stride[0]   = (int32_t)ALIGN_8B(spi_dev->info.width * 3);
    blt.dst_buf.format      = MPP_FMT_RGB_888;
#endif

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

static void spi_disp_data_transm(struct lv_spi_dev *spi_dev, bool bl_en)
{
    struct rt_spi_device *spi = (struct rt_spi_device *)spi_dev->dev;

    rt_spi_nonblock_set(spi, 0);
    rt_pin_write(spi_dev->rs_pin, PIN_LOW);
    lv_spi_write_seq(spi_dev, 0x2c);

    if (bl_en)
        /* flush first pixel frame in synchronous mode */
        rt_spi_nonblock_set(spi, 1);

    rt_pin_write(spi_dev->rs_pin, PIN_HIGH);
    rt_spi_transfer(spi, spi_dev->tx_buf, NULL, spi_dev->tx_len);
}

static void qspi_disp_data_transm(struct lv_spi_dev *spi_dev, bool bl_en)
{
    struct rt_qspi_device *qspi = (struct rt_qspi_device *)spi_dev->dev;

    rt_spi_nonblock_set((struct rt_spi_device *)&qspi->parent, 0);

    if (bl_en)
        /* flush first pixel frame in synchronous mode */
        rt_spi_nonblock_set((struct rt_spi_device *)&qspi->parent, 1);

    lv_qspi_transfer_data(qspi, spi_dev->tx_buf, spi_dev->tx_len, QSPI_TRANSFER_CODE, QSPI_TRANSFER_LINES);
    atomic_store(&spi_dev->has_send_data, 1);
}

static void disp_init_config(struct drv_spi_display_config *config)
{
    if (!config)
        return;

    rt_memset(config, 0, sizeof(struct drv_spi_display_config));

    config->dma_en = 1;
    config->spi_mode = DATA_MODE_QUAD;
    config->disp_on = 1;
}

static void disp_init_param(struct drv_spi_display_param *param)
{
    if (!param)
        return;

    rt_memset(param, 0, sizeof(struct drv_spi_display_param));

    param->mtw = 0;
    param->vsw_en = 1;
    param->vbp_en = 1;
    param->vfp_en = 1;

    param->len_stride = 400 * 3;
    param->len = 400 * 3;

    param->hvbp = 6;
    param->hvld = 400;
    param->hvfp = 6;

    param->cmd_vsw = SPI_VSW_CMD;
    param->cmd_vbp = SPI_VBP_CMD;
    param->cmd_vld = SPI_VLD_CMD;
    param->cmd_vfp = SPI_VFP_CMD;

    param->count = 60 * 48;
}

static void init_disp_trans(struct drv_spi_trans_data *trans, unsigned char *tx_buf)
{
    if (!trans)
        return;

    rt_memset(trans, 0, sizeof(struct drv_spi_trans_data));

    trans->tx_buf  = tx_buf;
    trans->en      = 1;
    trans->frm_cnt = 1;
}

static void disp_thread(void *arg)
{
    struct lv_spi_dev *spi_dev = arg;
    struct rt_qspi_device *qspi = (struct rt_qspi_device *)spi_dev->dev;

    static bool bl_en = false;
    static bool is_first_time = true;
    struct drv_spi_display_config disp_config;
    struct drv_spi_display_param disp_param;
    struct drv_spi_trans_data disp_trans;
    struct spi_disp_config spi_disp;

#if USE_SPI_HARDWARE
    disp_init_config(&disp_config);
    disp_init_param(&disp_param);

    rt_memset(&spi_disp, 0, sizeof(struct spi_disp_config));

    spi_disp.disp_param = &disp_param;
    spi_disp.disp_config = &disp_config;
    rt_spi_display_set((struct rt_spi_device *)qspi, &spi_disp);
#endif

    lv_disp_data_blt(spi_dev);
    if (bl_en && spi_dev->te_queue) {
        int ret = aicos_wqueue_wait(spi_dev->te_queue, TE_TIMEOUT_MS);
        if (ret < 0)
                LV_LOG_ERROR("SPI wait TE irq timeout, ret: %d\n", ret);
    }

    while (1) {
        aicos_sem_take(spi_dev->display_sem, AICOS_WAIT_FOREVER);

        lv_disp_data_blt(spi_dev);

        if (spi_dev->use_spi_hardware) {
            if (is_first_time) {
                rt_spi_nonblock_set((struct rt_spi_device *)&qspi->parent, 1);
                init_disp_trans(&disp_trans, spi_dev->tx_buf);
                rt_spi_display_trans((struct rt_spi_device *)qspi, &disp_trans);
                is_first_time = false;
            }
        } else {
            if (bl_en && spi_dev->te_queue) {
                int ret = aicos_wqueue_wait(spi_dev->te_queue, TE_TIMEOUT_MS);
                if (ret < 0)
                        LV_LOG_ERROR("SPI wait TE irq timeout, ret: %d\n", ret);
            }

            if (spi_dev->spi_type)
                spi_disp_data_transm(spi_dev, bl_en);
            else
                qspi_disp_data_transm(spi_dev, bl_en);

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
}

static int spi_setup_init(struct lv_spi_dev *spi, rt_base_t *pin)
{
    spi->bus_name = SPI_BUF_NAME;
    spi->mode     = SPI_MODE;
    spi->max_hz   = SPI_MAX_HZ;

    *pin = rt_pin_get(RS_PIN);
    if (*pin < 0) {
        LV_LOG_ERROR("get spi rs pin failed\n");
        rt_free(spi);
        return -1;
    }

    rt_pin_mode(*pin, PIN_MODE_OUTPUT);
    rt_pin_write(*pin, PIN_LOW);
    spi->rs_pin = *pin;

    return 0;
}

static int qspi_setup_init(struct lv_spi_dev *spi, rt_base_t *pin)
{
    spi->bus_name = QSPI_BUF_NAME;
    spi->mode     = QSPI_MODE;
    spi->max_hz   = QSPI_MAX_HZ;

    return 0;
}

static struct lv_spi_dev *lv_spi_setup(lv_display_t *disp)
{
    struct lv_spi_dev *spi = NULL;
    rt_base_t pin;
    int result;

    spi = rt_malloc(sizeof(struct lv_spi_dev));
    if (!spi) {
        LV_LOG_ERROR("malloc spi dev failed\n");
        return NULL;
    }
    rt_memset(spi, 0, sizeof(*spi));

    spi->spi_type = LV_SPI_TYPE;
    spi->use_spi_hardware = USE_SPI_HARDWARE;

    if (spi->spi_type)
        result = spi_setup_init(spi, &pin);
    else
        result = qspi_setup_init(spi, &pin);

    if (result != 0) {
        rt_free(spi);
        return NULL;
    }

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
    spi->te_queue = lv_spi_create_te_queue();
    spi->frame_count = 0;

    atomic_store(&spi->has_send_data, 0);

    struct mpp_fb *fb = mpp_fb_open();
    mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &spi->info);

    int tx_size = spi->info.width * spi->info.height * 2;

#if USE_SPI_HARDWARE
    tx_size = spi->info.width * spi->info.height * 3;
#endif

    spi->tx_buf = aicos_malloc_align(MEM_CMA, tx_size, CACHE_LINE_SIZE);
    if (!spi->tx_buf) {
        LV_LOG_ERROR("malloc display buf failed\n");
        rt_free(spi);
        return NULL;
    }
    aicos_dcache_clean_invalid_range((ulong *)spi->tx_buf, (ulong)ALIGN_UP(tx_size, CACHE_LINE_SIZE));

    spi->tx_len = tx_size;
    spi->ge2d_dev = mpp_ge_open();
    spi->disp = disp;
    if (lv_spi_init(spi) < 0) {
        rt_free(spi);

        if (spi->tx_buf)
            aicos_free_align(MEM_CMA, spi->tx_buf);

        return NULL;
    }

    return spi;
}

void lv_spi_screen_enable(lv_display_t *disp)
{
    struct lv_spi_dev *dev = NULL;
    aicos_thread_t thid = NULL;

    dev = lv_spi_setup(disp);
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

static void spi_disp_poweron(lv_display_t *disp)
{
    static bool first_frame = true;

    if (first_frame) {

        lv_spi_screen_enable(disp);
        first_frame = false;
    }
}

static void spi_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    spi_disp_t *spi_disp = (spi_disp_t *)lv_display_get_user_data(disp);
    lv_draw_buf_t *disp_buf = lv_display_get_buf_active(disp);
    (void)px_map;

    if (lv_disp_flush_is_last(disp)) {
        spi_disp_poweron(disp);
        aicos_dcache_clean_invalid_range((ulong *)disp_buf->data, (ulong)ALIGN_UP(spi_disp->info.smem_len, CACHE_LINE_SIZE));
        lv_spi_flush(disp_buf->data, spi_disp->info.smem_len);
    }

    lv_display_flush_ready(disp);
}

int lv_spi_display_init(int use_frame_buffer)
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

    if (use_frame_buffer == 0) {
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
    } else {
        spi_disp->buf1 = spi_disp->info.framebuffer;
#ifdef AIC_PAN_DISPLAY
        spi_disp->buf2 = spi_disp->info.framebuffer + fb_size;
#else
        spi_disp->buf2 = NULL;
#endif
    }

    lv_color_format_t cf = lv_display_fmt(spi_disp->info.format);
    if (cf == LV_COLOR_FORMAT_UNKNOWN)
        goto err;

    lv_display_t *disp = lv_display_create(width, height);
    lv_display_set_color_format(disp, cf);
    lv_display_set_flush_cb(disp, spi_disp_flush);
    lv_display_set_user_data(disp, spi_disp);

    lv_display_set_buffers(disp, spi_disp->buf1, spi_disp->buf2, fb_size, LV_DISPLAY_RENDER_MODE_DIRECT);
#if defined(LV_DISPLAY_ROTATE_EN) && defined(AIC_LVGL_DOUBLE_DISP_DEMO)
    lv_display_set_rotation(disp, LV_SECOND_ROTATE_DEGREE / 90);
#elif defined(LV_DISPLAY_ROTATE_EN) && defined(LV_USE_SPI_REPLACE_MIPI_DBI)
    lv_display_set_rotation(disp, LV_ROTATE_DEGREE / 90);
#endif
    spi_disp->disp = disp;

    return 0;

err:
    if (spi_disp)
        lv_free(spi_disp);
    if (spi_disp->buf1 && use_frame_buffer == 0)
        aicos_free_align(MEM_CMA, spi_disp->buf1);
    if (spi_disp->buf2 && use_frame_buffer == 0)
        aicos_free_align(MEM_CMA, spi_disp->buf2);

    LV_LOG_ERROR("create lv spi display failed");
    return -1;
}
