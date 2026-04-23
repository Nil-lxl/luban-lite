/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Huahui.mai <huahui.mai@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "aic_spi_lcd.h"

/**
 * DOC: overview
 *
 * This set of APIs uses a generic SPI controller to refresh SPI display peripherals.
 *
 * The ArtInChip MIPI-DBI panel driver utilizes the display subsystem to refresh
 * SPI display peripherals.
 *
 * When an ArtInChip chip is connected to multiple SPI display peripherals,
 * this interface can be used to initialize and refresh SPI displays via a
 * generic SPI controller. It provides flexibility for managing multiple displays
 * without relying on the dedicated display subsystem.
 */

static rt_err_t lv_qspi_transfer_data(struct rt_qspi_device *device, const void *send_buf,
                               rt_size_t length, uint32_t content, int data_lines)
{
    struct rt_qspi_message message = {0};
    rt_err_t result = RT_EOK;

    RT_ASSERT(send_buf);

    if (length > 0)
        message.qspi_data_lines = data_lines;

    /* send qspi code */
    message.address.content = content;
    message.address.size = sizeof(content);
    message.address.qspi_lines = 1;

    /* set send buf and send size */
    message.parent.send_buf = send_buf;
    message.parent.recv_buf = RT_NULL;
    message.parent.length = length;
    message.parent.cs_take = 1;
    message.parent.cs_release = 1;

    result = rt_qspi_transfer_message(device, &message);
    if (result != length)
        return -RT_EIO;
    else
        return length;
}

static int spi_command(aic_spi_lcd_dev_t *spi_dev, unsigned int cmd, unsigned int len, const u8 *data)
{
    struct rt_spi_device *spi = (struct rt_spi_device *)spi_dev->dev;

    rt_spi_take_bus(spi);

    rt_pin_write(spi_dev->rs_pin, PIN_LOW);
    rt_spi_transfer(spi, (u8[]){ cmd }, NULL, 1);

    rt_pin_write(spi_dev->rs_pin, PIN_HIGH);
    if (len != 0) {
        int ret = rt_spi_transfer(spi, (void *)data, NULL, len);
        if (ret != len) {
            pr_err("Send spi data failed. ret 0x%x\n", (int)ret);
            return -1;
        }
    }

    rt_spi_release_bus(spi);

    return len;
}

static int qspi_command(aic_spi_lcd_dev_t *spi_dev, unsigned int cmd, unsigned int len, const u8 *data)
{
    struct rt_qspi_device *qspi = (struct rt_qspi_device *)spi_dev->dev;
    const struct qspi_disp_priv *priv = spi_dev->qspi_cfg->priv;
    int ret;

    if (!priv)
        return -1;

    rt_spi_take_bus((struct rt_spi_device *)qspi);

    uint32_t content = (priv->instruction.cmd_content & 0xFFFF00FF) | (cmd << 8);
    ret = lv_qspi_transfer_data(qspi, (void *)data, len, content, priv->instruction.qspi_lines);
    if (ret != len)
            pr_err("Send spi data failed. ret 0x%x\n", (int)ret);

    rt_spi_release_bus((struct rt_spi_device *)qspi);

    return ret;
}

static void spi_wait_completion(aic_spi_lcd_dev_t *spi_dev)
{
    struct rt_spi_device *spi = (struct rt_spi_device *)spi_dev->dev;

    rt_spi_wait_completion(spi);
}

static void qspi_wait_completion(aic_spi_lcd_dev_t *spi_dev)
{
    struct rt_qspi_device *qspi = (struct rt_qspi_device *)spi_dev->dev;

    rt_spi_wait_completion((struct rt_spi_device *)&qspi->parent);
}

static int spi_init(aic_spi_lcd_dev_t *spi_dev)
{
    const aic_spi_lcd_cfg_t *config = spi_dev->config;
    struct rt_spi_configuration spi_cfg = {0};
    struct rt_spi_device *spi_device = NULL;
    struct rt_device *dev = NULL;
    int result = 0;

    if (!spi_dev->rs_pin) {
        pr_err("spi device miss a rs pin.\n");
        return -RT_ERROR;
    }

    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (spi_device == RT_NULL) {
        pr_err("rt malloc spi device failed.\n");
        return -RT_ERROR;
    }

    result = rt_spi_bus_attach_device(spi_device, config->dev_name, config->bus_name, NULL);
    if (result != RT_EOK && spi_device != NULL) {
        pr_err("rt spi bus %s attach device  %s failed.\n", config->bus_name, config->dev_name);
        result = -RT_ERROR;
        goto err;
    }

    spi_device = (struct rt_spi_device *)rt_device_find(config->dev_name);
    if (!spi_device) {
        pr_err("Failed to get device in %s\n", config->dev_name);
        result = -RT_ERROR;
        goto err;
    }

    dev = (struct rt_device *)spi_device;
    if (dev->type != RT_Device_Class_SPIDevice) {
        spi_device = NULL;
        pr_err("%s is not SPI device.\n", config->dev_name);
        result = -RT_ERROR;
        goto err;
    }

    spi_cfg.mode = config->mode;
    spi_cfg.max_hz = config->max_hz;
    result = rt_spi_configure(spi_device, &spi_cfg);
    if (result < 0) {
        pr_err("qspi configure failure.\n");
        result = -RT_ERROR;
        goto err;
    }

    spi_dev->dev = spi_device;
    return result;

err:
    if (spi_dev)
        rt_free(spi_dev);

    return result;
}

static int qspi_init(aic_spi_lcd_dev_t *spi_dev)
{
    const aic_spi_lcd_cfg_t *config = spi_dev->config;
    struct rt_qspi_configuration qspi_cfg = {0};
    struct rt_qspi_device *qspi_device = NULL;
    struct rt_device *dev = NULL;
    int result = 0;

    if (!config->qspi_cfg->priv) {
        pr_err("aic_spi_lcd_dev_t qspi_cfg miss priv\n");
        return -RT_ERROR;
    }

    qspi_device = (struct rt_qspi_device *)rt_malloc(sizeof(struct rt_qspi_device));
    if (qspi_device == RT_NULL) {
        pr_err("rt malloc spi device failed.\n");
        return -RT_ERROR;
    }

    result = aic_qspi_bus_attach_device(config->bus_name, config->dev_name, 0, 4, NULL, NULL);
    if (result != RT_EOK) {
        pr_err("rt qspi bus %s attach device %s failed.\n", config->bus_name, config->dev_name);
        result = -RT_ERROR;
        goto err;
    }

    qspi_device = (struct rt_qspi_device *)rt_device_find(config->dev_name);
    if (!qspi_device) {
        pr_err("Failed to get device in %s\n", config->dev_name);
        result = -RT_ERROR;
        goto err;
    }

    dev = (struct rt_device *)qspi_device;
    if (dev->type != RT_Device_Class_SPIDevice) {
        qspi_device = NULL;
        pr_err("%s is not SPI device.\n", config->dev_name);
        result = -RT_ERROR;
        goto err;
    }

    qspi_cfg.parent.mode = config->mode;
    qspi_cfg.parent.max_hz = config->max_hz;
    result = rt_qspi_configure(qspi_device, &qspi_cfg);
    if (result < 0) {
        pr_err("qspi configure failure.\n");
        result = -RT_ERROR;
        goto err;
    }

    spi_dev->qspi_cfg = config->qspi_cfg;
    spi_dev->dev = qspi_device;

    return result;

err:
    if (spi_dev)
        rt_free(spi_dev);

    return result;
}

static int spi_flush(aic_spi_lcd_dev_t *spi_dev, u8 *data, unsigned int len)
{
    struct rt_spi_device *spi = (struct rt_spi_device *)spi_dev->dev;

    rt_spi_nonblock_set(spi, 0);
    rt_pin_write(spi_dev->rs_pin, PIN_LOW);
    aic_spi_lcd_write_seq(spi_dev, 0x2c);

    rt_spi_nonblock_set(spi, 1);
    rt_pin_write(spi_dev->rs_pin, PIN_HIGH);
    int ret = rt_spi_transfer(spi, data, NULL, len);
    if (ret < 0)
        return ret;

    return 0;
}

static int qspi_flush(aic_spi_lcd_dev_t *spi_dev, u8 *data, unsigned int len)
{
    struct rt_qspi_device *qspi = (struct rt_qspi_device *)spi_dev->dev;
    const struct qspi_disp_priv *priv = spi_dev->qspi_cfg->priv;
    int ret = 0;

    rt_spi_nonblock_set((struct rt_spi_device *)&qspi->parent, 1);

    if (spi_dev->qspi_cfg->disp_mode && priv->spi_disp) {
        struct drv_spi_trans_data display_trans = {
            .tx_buf = data,
            .en = 0,
            .frm_cnt = 1,
        };

        rt_spi_display_set((struct rt_spi_device *)qspi, priv->spi_disp);
        rt_spi_display_trans((struct rt_spi_device *)qspi, &display_trans);
    } else {
        ret = lv_qspi_transfer_data(qspi, data, len, priv->instruction.flush_content, priv->data_lines);
        if (ret < 0)
            pr_err("qspi flush transfer data failed.\n");
    }

    return ret;
}

static const struct spi_ops spi_ops = {
    .command         = spi_command,
    .flush           = spi_flush,
    .init            = spi_init,
    .wait_completion = spi_wait_completion,
};

static const struct spi_ops qspi_ops = {
    .command         = qspi_command,
    .flush           = qspi_flush,
    .init            = qspi_init,
    .wait_completion = qspi_wait_completion,
};

aic_spi_lcd_dev_t *aic_spi_lcd_init(const aic_spi_lcd_cfg_t *config)
{
    aic_spi_lcd_dev_t *dev;
    int result;

    dev = rt_malloc(sizeof(aic_spi_lcd_dev_t));
    if (!dev) {
        pr_err("malloc spi dev failed\n");
        return NULL;
    }
    rt_memset(dev, 0, sizeof(*dev));

    if (config->rs_pin) {
        dev->rs_pin = rt_pin_get(config->rs_pin);
        if (dev->rs_pin < 0) {
            pr_err("get spi rs pin %s failed\n", config->rs_pin);
            rt_free(dev);
            return NULL;
        }

        rt_pin_mode(dev->rs_pin, PIN_MODE_OUTPUT);
        rt_pin_write(dev->rs_pin, PIN_LOW);
    }

    if (config->bus_width == 1)
        dev->ops = &spi_ops;
    else
        dev->ops = &qspi_ops;

    dev->config = config;

    if (dev->ops->init)
        result = dev->ops->init(dev);
    else
        result = -1;

    if (result < 0) {
        rt_free(dev);
        return NULL;
    }

    return dev;
}

int aic_spi_lcd_write_cmd(aic_spi_lcd_dev_t *spi_dev, unsigned int cmd, unsigned int len, const u8 *data)
{
    if (spi_dev->ops->command)
        return spi_dev->ops->command(spi_dev, cmd, len, data);

    return -1;
}

int aic_spi_lcd_flush(aic_spi_lcd_dev_t *spi_dev, u8 *data, unsigned int len)
{
    if (spi_dev->ops->flush)
        return spi_dev->ops->flush(spi_dev, data, len);

    return -1;
}

int aic_spi_lcd_config_win(aic_spi_lcd_dev_t *spi_dev,
                          unsigned int x_start, unsigned int x_end,
                          unsigned int y_start, unsigned int y_end)
{
    unsigned char row[4] = {0};
    unsigned char column[4] = {0};

    column[0] = (x_start >> 8) & 0xff;
    column[1] = (x_start) & 0xff;
    column[2] = (x_end >> 8) & 0xff;
    column[3] = (x_end) & 0xff;

    row[0] = (y_start >> 8) & 0xff;
    row[1] = (y_start) & 0xff;
    row[2] = (y_end >> 8) & 0xff;
    row[3] = (y_end) & 0xff;

    if (aic_spi_lcd_write_cmd(spi_dev, 0x2A, sizeof(column), column) < 0)
        return -1;

    if (aic_spi_lcd_write_cmd(spi_dev, 0x2B, sizeof(row), row) < 0)
        return -1;

    return 0;
}

int aic_spi_lcd_flush_win(aic_spi_lcd_dev_t *spi_dev,
                          unsigned int x_start, unsigned int x_end,
                          unsigned int y_start, unsigned int y_end,
                          u8 *data, unsigned int len)
{
    if (aic_spi_lcd_config_win(spi_dev, x_start, x_end, y_start, y_end) < 0)
        return -1;

    return aic_spi_lcd_flush(spi_dev, data, len);
}

void aic_spi_lcd_wait_completion(aic_spi_lcd_dev_t *spi_dev)
{
    if (spi_dev->ops->wait_completion)
        spi_dev->ops->wait_completion(spi_dev);
}

void aic_spi_lcd_draw_rgb565_swap(void *buf, u32 buf_size_px)
{
    u32 u32_cnt = buf_size_px / 2;
    u16 *buf16 = buf;
    u32 *buf32 = buf;

    while(u32_cnt >= 8) {
        buf32[0] = ((buf32[0] & 0xff00ff00) >> 8) | ((buf32[0] & 0x00ff00ff) << 8);
        buf32[1] = ((buf32[1] & 0xff00ff00) >> 8) | ((buf32[1] & 0x00ff00ff) << 8);
        buf32[2] = ((buf32[2] & 0xff00ff00) >> 8) | ((buf32[2] & 0x00ff00ff) << 8);
        buf32[3] = ((buf32[3] & 0xff00ff00) >> 8) | ((buf32[3] & 0x00ff00ff) << 8);
        buf32[4] = ((buf32[4] & 0xff00ff00) >> 8) | ((buf32[4] & 0x00ff00ff) << 8);
        buf32[5] = ((buf32[5] & 0xff00ff00) >> 8) | ((buf32[5] & 0x00ff00ff) << 8);
        buf32[6] = ((buf32[6] & 0xff00ff00) >> 8) | ((buf32[6] & 0x00ff00ff) << 8);
        buf32[7] = ((buf32[7] & 0xff00ff00) >> 8) | ((buf32[7] & 0x00ff00ff) << 8);
        buf32 += 8;
        u32_cnt -= 8;
    }

    while(u32_cnt) {
        *buf32 = ((*buf32 & 0xff00ff00) >> 8) | ((*buf32 & 0x00ff00ff) << 8);
        buf32++;
        u32_cnt--;
    }

    if(buf_size_px & 0x1) {
        u32 e = buf_size_px - 1;
        buf16[e] = ((buf16[e] & 0xff00) >> 8) | ((buf16[e] & 0x00ff) << 8);
    }
}

void aic_spi_lcd_backlight_gpio_enable(const char *gpio_name, bool low_active, bool enable)
{
    unsigned int g, p;
    long pin;

    pin = hal_gpio_name2pin(gpio_name);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    hal_gpio_set_bias_pull(g, p, 0);
    hal_gpio_set_drive_strength(g, p, 3);
    hal_gpio_direction_output(g, p);

    if (enable ^ low_active)
        hal_gpio_set_output(g, p);
    else
        hal_gpio_clr_output(g, p);
}

void aic_spi_lcd_backlight_pwm_enable(int channel, int level, bool enable)
{
#ifdef RT_USING_PWM
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    if (!pwm_dev) {
        pr_err("pwm device not found.\n");
        return;
    }

    if (enable) {
        rt_pwm_set(pwm_dev, channel, 1000000, 10000 * level);
        rt_pwm_enable(pwm_dev, channel);
    } else {
        rt_pwm_disable(pwm_dev, channel);
    }
#endif
}
