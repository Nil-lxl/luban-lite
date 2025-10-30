/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <string.h>
#include <getopt.h>

#include <drivers/touch.h>

#define LOG_TAG             "RTP"
#include "aic_core.h"

#include "hal_rtp.h"

#ifdef AIC_GPAI_USING_RTP_CHAN
#define AIC_RTP_DEFAULT_MODE    RTP_MODE_MANUAL
#else
#define AIC_RTP_DEFAULT_MODE    RTP_MODE_AUTO2
#endif

static struct rt_touch_device g_rt_rtp_dev = {0};
static struct aic_rtp_dev g_rtp_dev = {0};
static int g_last_count =1;

rt_err_t aic_rtp_init(void)
{
    if (g_rtp_dev.sem) {
        LOG_I("RTP is already inited!");
        return RT_EOK;
    }
    if (hal_rtp_clk_init())
        return -RT_ERROR;

    aicos_request_irq(RTP_IRQn, hal_rtp_isr, 0, NULL, NULL);

    memset(&g_rtp_dev, 0, sizeof(struct aic_rtp_dev));
    g_rtp_dev.x_plate = AIC_RTP_X_PLATE;
    if (AIC_RTP_Y_PLATE > 0)
        g_rtp_dev.y_plate = AIC_RTP_Y_PLATE;
    LOG_I("x_plate %d y_plate %d", g_rtp_dev.x_plate, g_rtp_dev.y_plate);

    g_rtp_dev.mode = AIC_RTP_DEFAULT_MODE;
    g_rtp_dev.max_press = AIC_RTP_MAX_PRESSURE;
    g_rtp_dev.smp_period = AIC_RTP_PERIOD_MS;
    g_rtp_dev.delay = AIC_RTP_DELAY;
    if (g_rtp_dev.mode != RTP_MODE_AUTO1)
        g_rtp_dev.pressure_det = 1;

    hal_rtp_enable(&g_rtp_dev, 1);

    return RT_EOK;
}

static int drv_rtp_plate_check(struct aic_rtp_dev *rtp,
                               struct rt_touch_data *data)
{
    int tmp = 0;

    if (rtp->x_plate < rtp->y_plate) {
            tmp = data->x_coordinate;
            data->x_coordinate = data->y_coordinate;
            data->y_coordinate = tmp;
    }

    return 0;
}

static rt_size_t drv_rtp_read_point(struct rt_touch_device *touch, void *buf,
                                   rt_size_t touch_num)
{
    s32 ret = 0;
    struct aic_rtp_event e = {0};
    struct rt_touch_data *data = (struct rt_touch_data *)buf;

    RT_ASSERT(buf);
    RT_ASSERT(read_num == 1);

    if (g_last_count != touch_num) {
        hal_rtp_ebuf_sync(&g_rtp_dev.ebuf);
        g_last_count = touch_num;
    }

    ret = hal_rtp_ebuf_read(&g_rtp_dev.ebuf, &e);
    if (ret < 0) {
        rt_kprintf("Failed to get touch data");
        return 0;
    }

    data->x_coordinate = e.x;
    data->y_coordinate = e.y;
    data->pressure = e.pressure;
    data->timestamp = e.timestamp;
    if (e.down)
        data->event = RT_TOUCH_EVENT_DOWN;
    else
        data->event = RT_TOUCH_EVENT_UP;

    return touch_num;
}

static rt_err_t drv_rtp_control(struct rt_touch_device *touch,
                                int cmd, void *arg)
{
    u16 ret = 0;

    switch (cmd) {
    case RT_TOUCH_CTRL_ENABLE_INT:
        hal_rtp_int_enable(&g_rtp_dev, 1);
        hal_rtp_ebuf_init(&g_rtp_dev.ebuf);
        hal_rtp_auto_mode(&g_rtp_dev);
        break;

    case RT_TOUCH_CTRL_DISABLE_INT:
        hal_rtp_int_enable(&g_rtp_dev, 0);
        break;

#if 0
    case RT_TOUCH_CTRL_PDEB_VALID_CHECK:
        u32 val = hal_rtp_pdeb_valid_check(&g_rtp_dev);
        if (val != RT_EOK)
            LOG_I("PDEB should be configured as 0xff%xff%x\n", val, val);
        break;
#endif
    case RT_TOUCH_CTRL_GET_ADC:
        ret = hal_rtp_adc_soft_trigger(&g_rtp_dev, ((struct aic_rtp_adc_info *)arg)->ch);
        ((struct aic_rtp_adc_info *)arg)->data = ret;
        break;

    case RT_TOUCH_CTRL_SET_X_TO_Y:
        drv_rtp_plate_check(&g_rtp_dev, arg);
        break;

    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }

    return RT_EOK;
}

static int rtp_touch_isr(void)
{
    struct rt_touch_device *dev = &g_rt_rtp_dev;
    rt_hw_touch_isr(dev);
    return 0;
}

static struct rt_touch_ops aic_rtp_ops = {
    .touch_readpoint = drv_rtp_read_point,
    .touch_control   = drv_rtp_control,
};

static int drv_rtp_init(void)
{
    rt_int8_t ret = RT_EOK;
    struct rt_touch_device *dev = &g_rt_rtp_dev;

    if (aic_rtp_init()) {
        LOG_E("Failed to init RTP controller");
        return -RT_ERROR;
    }

    dev->info.type = RT_TOUCH_TYPE_RESISTANCE;
    dev->info.point_num = 1;
    dev->info.range_x = AIC_RTP_MAX_VAL;
    dev->info.range_y = AIC_RTP_MAX_VAL;
    dev->config.dev_name = AIC_RTP_NAME;
    dev->ops = &aic_rtp_ops;
    ret = rt_hw_touch_register(dev, "aic-rtp", RT_DEVICE_FLAG_INT_RX, RT_NULL);
    if (ret != RT_EOK)
        LOG_E("Failed to register RTP, return %d", ret);

    hal_rtp_register_callback(rtp_touch_isr);
    LOG_I("ArtInChip RTP loaded");
    return ret;
}
INIT_DEVICE_EXPORT(drv_rtp_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_rtp_status(int argc, char **argv)
{
     hal_rtp_status_show(&g_rtp_dev);
}

MSH_CMD_EXPORT_ALIAS(cmd_rtp_status, rtp_status, Show the status of RTP);

#endif
