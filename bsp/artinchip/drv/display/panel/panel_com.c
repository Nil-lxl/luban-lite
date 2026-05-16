/*
 * Copyright (C) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal.h>

#include "panel_com.h"
#include "disp_gpio.h"

static struct aic_panel *panels[] = {
#if defined(AIC_DISP_RGB) && defined(AIC_SIMPLE_PANEL)
    &aic_panel_rgb,
#endif
#if defined(AIC_DISP_LVDS) && defined(AIC_SIMPLE_PANEL)
    &aic_panel_lvds,
#endif
#ifdef AIC_DSI_SIMPLE_PANEL
    &dsi_simple,
#endif
#ifdef AIC_PANEL_DSI_EDP_LT9811EXB
    &dsi_edp_lt9811exb,
#endif
#ifdef AIC_PANEL_DSI_ST7703
    &dsi_st7703,
#endif
#ifdef AIC_PANEL_DSI_H013A08
    &dsi_h013a08,
#endif
#ifdef AIC_PANEL_DSI_H016A01
    &dsi_h016a01,
#endif
#ifdef AIC_PANEL_DSI_H024B12
    &dsi_h024b12,
#endif
#ifdef AIC_PANEL_DSI_H028B23
    &dsi_h028b23,
#endif
#ifdef AIC_PANEL_DSI_H028A29
    &dsi_h028a29,
#endif
#ifdef AIC_PANEL_DSI_H030B07
    &dsi_h030b07,
#endif
#ifdef AIC_PANEL_DSI_H032A06
    &dsi_h032a06,
#endif
#ifdef AIC_PANEL_DSI_H034A01
    &dsi_h034a01,
#endif
#ifdef AIC_PANEL_DSI_H035A27
    &dsi_h035a27,
#endif
#ifdef AIC_PANEL_DSI_H035B16
    &dsi_h035b16,
#endif
#ifdef AIC_PANEL_DSI_H035B22
    &dsi_h035b22,
#endif
#ifdef AIC_PANEL_DSI_H042A01
    &dsi_h042a01,
#endif
#ifdef AIC_PANEL_DSI_H043A8
    &dsi_h043a8,
#endif
#ifdef AIC_PANEL_DSI_H040B24
    &dsi_h040b24,
#endif
#ifdef AIC_PANEL_DSI_H050A20
    &dsi_h050a20,
#endif
#ifdef AIC_PANEL_DSI_H055A03
    &dsi_h055a03,
#endif
#ifdef AIC_PANEL_DSI_H055A05
    &dsi_h055a05,
#endif
#ifdef AIC_PANEL_DSI_H070B13
    &dsi_h070b13,
#endif
#ifdef AIC_PANEL_DSI_H070A21
    &dsi_h070a21,
#endif
#ifdef AIC_PANEL_DSI_H070A27
    &dsi_h070a27,
#endif
#ifdef AIC_PANEL_DSI_H078B06
    &dsi_h078b06,
#endif
#ifdef AIC_PANEL_DSI_H080A11
    &dsi_h080a11,
#endif
#ifdef AIC_PANEL_DSI_H080D16
    &dsi_h080d16,
#endif
#ifdef AIC_PANEL_DSI_H103W01
    &dsi_h103w01,
#endif


#ifdef AIC_PANEL_DBI_ILI9488
    &dbi_ili9488,
#endif
#ifdef AIC_PANEL_DBI_ST7789
    &dbi_st7789,
#endif
#ifdef AIC_PANEL_DBI_H040A12
    &dbi_h040a12,
#endif
#ifdef AIC_PANEL_DBI_H040A27
    &dbi_h040a27,
#endif
#ifdef AIC_PANEL_DBI_ST7789V
    &dbi_st7789v,
#endif
#ifdef AIC_PANEL_SPI_GENERAL
    &spi_general,
#endif

#ifdef AIC_PANEL_RGB_ST7701S
    &rgb_st7701s,
#endif
#ifdef AIC_PANEL_RGB_H024A13
    &rgb_h024a13,
#endif
#ifdef AIC_PANEL_RGB_H028A19
    &rgb_h028a19,
#endif
#ifdef AIC_PANEL_RGB_H030A8
    &rgb_h030a8,
#endif
#ifdef AIC_PANEL_RGB_H030A10
    &rgb_h030a10,
#endif
#ifdef AIC_PANEL_RGB_H034A02
    &rgb_h034a02,
#endif
#ifdef AIC_PANEL_RGB_H035A17
    &rgb_h035a17,
#endif
#ifdef AIC_PANEL_RGB_H035A24
    &rgb_h035a24,
#endif
#ifdef AIC_PANEL_RGB_H040A18
    &rgb_h040a18,
#endif
#ifdef AIC_PANEL_RGB_H043A7
    &rgb_h043a7,
#endif
#ifdef AIC_PANEL_RGB_H043A34
    &rgb_h043a34,
#endif
#ifdef AIC_PANEL_RGB_H043B32
    &rgb_h043b32,
#endif
#ifdef AIC_PANEL_RGB_H048A02
    &rgb_h048a02,
#endif
#ifdef AIC_PANEL_RGB_H050A12
    &rgb_h050a12,
#endif
#ifdef AIC_PANEL_RGB_H050A18
    &rgb_h050a18,
#endif
#ifdef AIC_PANEL_RGB_H052A01
    &rgb_h052a01,
#endif
#ifdef AIC_PANEL_RGB_H062A01
    &rgb_h062a01,
#endif
#ifdef AIC_PANEL_RGB_H068A04
    &rgb_h068a04,
#endif
#ifdef AIC_PANEL_RGB_H068B06
    &rgb_h068b06,
#endif
#ifdef AIC_PANEL_RGB_H070A18
    &rgb_h070a18,
#endif
#ifdef AIC_PANEL_RGB_ST7102
    &rgb_st7102,
#endif
#ifdef AIC_PANEL_SRGB_HX8238
    &srgb_hx8238,
#endif
#ifdef AIC_PANEL_BRIDGE_LT8911
    &bridge_lt8911,
#endif

};

struct aic_panel *aic_find_panel(u32 connector_type)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(panels); i++) {
        if (panels[i]->connector_type == connector_type) {
            break;
        }
    }

    if (i >= ARRAY_SIZE(panels))
        return NULL;

    pr_debug("find panel driver : %s\n", panels[i]->name);

    return panels[i];
}

/* The follow functions are defined for panel driver */

/* Enable the display interface */
void panel_di_enable(struct aic_panel *panel, u32 ms)
{
    if (panel && panel->callbacks.di_enable)
        panel->callbacks.di_enable();

    if (ms)
        aic_delay_ms(ms);
}

/* Disable the display interface */
void panel_di_disable(struct aic_panel *panel, u32 ms)
{
    if (panel && panel->callbacks.di_disable)
        panel->callbacks.di_disable();

    if (ms)
        aic_delay_ms(ms);
}

void panel_de_timing_enable(struct aic_panel *panel, u32 ms)
{
    if (panel && panel->callbacks.timing_enable)
        panel->callbacks.timing_enable();

    if (ms)
        aic_delay_ms(ms);
}

void panel_de_timing_disable(struct aic_panel *panel, u32 ms)
{
    if (panel && panel->callbacks.timing_disable)
        panel->callbacks.timing_disable();

    if (ms)
        aic_delay_ms(ms);
}

void panel_backlight_enable(struct aic_panel *panel, u32 ms)
{
#ifdef AIC_PM_INDEPENDENT_POWER_KEY
    if (panel && panel->independent_pwkey)
        return;
#endif

/* Use BL Enable Gpio */
#ifdef AIC_BACKLIGHT_ENABLE_GPIO
    unsigned int g, p;
    long pin;
    pin = hal_gpio_name2pin(AIC_BACKLIGHT_ENABLE_GPIO);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_direction_output(g, p);

#ifndef AIC_BACKLIGHT_ENABLE_GPIO_LOW
    hal_gpio_set_output(g, p);
#else
    hal_gpio_clr_output(g, p);
#endif

#endif /* AIC_PANEL_ENABLE_GPIO */

#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");

#ifndef AIC_PWM_BACKLIGHT_BYPASS
    /* pwm frequency: 1KHz = 1000000ns */
    rt_pwm_set(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL, 10000, 100 * AIC_PWM_BRIGHTNESS_LEVEL);
#endif
    rt_pwm_enable(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL);
#endif
}

void panel_backlight_disable(struct aic_panel *panel, u32 ms)
{
#ifdef AIC_PM_INDEPENDENT_POWER_KEY
    if (panel && panel->independent_pwkey)
        return;
#endif
#ifdef AIC_PANEL_ENABLE_GPIO
    unsigned int g, p;
    long pin;

    pin = hal_gpio_name2pin(AIC_PANEL_ENABLE_GPIO);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

#ifndef AIC_PANEL_ENABLE_GPIO_LOW
    hal_gpio_clr_output(g, p);
#else
    hal_gpio_set_output(g, p);
#endif
#endif /* AIC_PANEL_ENABLE_GPIO */

#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    rt_pwm_disable(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL);
#endif
}

int panel_default_prepare(void)
{
#ifdef AIC_SIMPLE_PANEL_POWER_GPIO
    unsigned int g, p;
    long pin;

    pin = hal_gpio_name2pin(AIC_SIMPLE_PANEL_POWER_GPIO_PIN);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    hal_gpio_set_bias_pull(g, p, 0);
    hal_gpio_set_drive_strength(g, p, 3);
    hal_gpio_direction_output(g, p);

#ifdef AIC_SIMPLE_PANEL_POWER_GPIO_LOW_ACTIVE
    hal_gpio_clr_output(g, p);
#else
    hal_gpio_set_output(g, p);
#endif

    aic_delay_ms(AIC_SIMPLE_PANEL_POWER_DELAY_MS);
#endif /* AIC_SIMPLE_PANEL_POWER_GPIO */
    return 0;
}

int panel_default_unprepare(void)
{
#ifdef AIC_SIMPLE_PANEL_POWER_GPIO
    unsigned int g, p;
    long pin;

    pin = hal_gpio_name2pin(AIC_SIMPLE_PANEL_POWER_GPIO_PIN);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

#ifdef AIC_SIMPLE_PANEL_POWER_GPIO_LOW_ACTIVE
    hal_gpio_set_output(g, p);
#else
    hal_gpio_clr_output(g, p);
#endif
#endif /* AIC_SIMPLE_PANEL_POWER_GPIO */
    return 0;
}

int panel_default_enable(struct aic_panel *panel)
{
    static struct gpio_desc reset_gpio;
    panel_get_gpio(&reset_gpio, RESET_PIN);

    aic_delay_ms(1);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 60);
    panel_backlight_enable(panel, 0);
    return 0;
}

int panel_default_disable(struct aic_panel *panel)
{
    panel_backlight_disable(panel, 0);
    panel_di_disable(panel, 0);
    panel_de_timing_disable(panel, 0);
    return 0;
}

int panel_register_callback(struct aic_panel *panel,
                struct aic_panel_callbacks *pcallback)
{
    panel->callbacks.di_enable = pcallback->di_enable;
    panel->callbacks.di_disable = pcallback->di_disable;
    panel->callbacks.di_send_cmd = pcallback->di_send_cmd;
    panel->callbacks.di_set_videomode = pcallback->di_set_videomode;
    panel->callbacks.timing_enable = pcallback->timing_enable;
    panel->callbacks.timing_disable = pcallback->timing_disable;
    return 0;
}

void panel_send_command(u8 *para_cmd, u32 size, struct aic_panel *panel)
{
    u8 *p;
    u8 num, code;

    for (p = para_cmd; p < (para_cmd + size);) {
        num  = *p++ - 1;
        code = *p++;

        if (num == 0)
            aic_delay_ms((u32)code);
        else
            panel->callbacks.di_send_cmd((u32)code, 0, p, num);

        p += num;
    }
}

void panel_get_gpio(struct gpio_desc *desc, char *name)
{
    long pin;

    if (!desc || !name) {
        pr_err("Invalid parameter\n");
        return;
    }

    pin = hal_gpio_name2pin(name);
    if (pin < 0) {
        pr_err("Failed to get GPIO %s\n", name);
        return;
    }

    desc->g = GPIO_GROUP(pin);
    desc->p = GPIO_GROUP_PIN(pin);

    hal_gpio_direction_output(desc->g, desc->p);
}

void panel_gpio_set_value(struct gpio_desc *desc, u32 value)
{
    if (!desc) {
        pr_err("Invalid parameter\n");
        return;
    }

    if (value)
        hal_gpio_set_output(desc->g, desc->p);
    else
        hal_gpio_clr_output(desc->g, desc->p);
}

#ifdef AIC_PANEL_SPI_EMULATION
static struct panel_spi_device spi = { 0 };
static bool panel_spi_emulation = false;

static inline void panel_spi_set_scl(u32 value)
{
    panel_gpio_set_value(&spi.scl, value);
}

static inline void panel_spi_set_sdi(u32 value)
{
    panel_gpio_set_value(&spi.sdi, value);
}

static inline void panel_spi_set_cs(u32 value)
{
    panel_gpio_set_value(&spi.cs, value);
}

static inline void panel_spi_set_dc(u32 value)
{
    panel_gpio_set_value(&spi.dc, value);
}

#ifndef AIC_SPI_EMULATION_WITH_DC

/**
 * @brief spi发送初始化命令序列，适用于不区分命令/数据的SPI设备,有效数据8位
 * @param data 要发送的数据序列
 */
void panel_spi_write_byte(u8 data) {
    for (int i = 0;i < 8;i++) {
        if (data & 0x80)
            panel_spi_set_sdi(1);
        else
            panel_spi_set_sdi(0);

        panel_spi_set_scl(0);
        aic_delay_us(1);
        panel_spi_set_scl(1);
        data <<= 1;
    }
    panel_spi_set_scl(0);
    aic_delay_us(1);
}

/**
 * @brief 单次写入单个字节数据到SPI设备
 * @param data 要写入的数据
 */
void panel_spi_write_single(u8 data) {
    panel_spi_set_cs(0);
    aic_delay_us(1);

    panel_spi_write_byte(data);

    aic_delay_us(1);
    panel_spi_set_cs(1);
}

/**
 * @brief 连续发送多个字节数据到SPI设备
 * @param data 要写入的数据序列
 * @param len 数据序列的长度
 */
void panel_spi_write_multi(const u8 *data, size_t len) {
    panel_spi_set_cs(0);
    aic_delay_us(1);

    for (size_t i = 0; i < len; i++) {
        panel_spi_write_byte(data[i]);
    }

    aic_delay_us(1);
    panel_spi_set_cs(1);
}

/**
 * @brief spi发送单字节命令,适用于区分命令/数据的SPI设备,有效数据9位
 * @param cmd 要发送的命令字节
 */
void panel_spi_cmd_wr(u8 cmd)
{
    u32 i;

    if (!panel_spi_emulation)
        return;

    panel_spi_set_cs(0);

    panel_spi_set_sdi(0);
    panel_spi_set_scl(0);

    aic_delay_us(1);
    panel_spi_set_scl(1);

    aic_delay_us(1);
    panel_spi_set_scl(0);

    for (i = 0; i < 8; i++) {
        if ((cmd & 0x80) == 0x80)
            panel_spi_set_sdi(1);
        else
            panel_spi_set_sdi(0);

        aic_delay_us(1);
        panel_spi_set_scl(1);
        aic_delay_us(1);
        panel_spi_set_scl(0);
        aic_delay_us(1);
        cmd = cmd << 1;
    }

    panel_spi_set_cs(1);
    panel_spi_set_sdi(0);
    panel_spi_set_scl(0);
    aic_delay_us(1);
}

/**
 * @brief spi发送单字节数据,适用于区分命令/数据的SPI设备,有效数据9位
 * @param data 要发送的数据字节
 */
void panel_spi_data_wr(u8 data)
{
    u32 i;

    if (!panel_spi_emulation)
        return;

    panel_spi_set_cs(0);
    panel_spi_set_scl(0);
    panel_spi_set_sdi(1);

    aic_delay_us(1);
    panel_spi_set_scl(1);

    aic_delay_us(1);
    panel_spi_set_scl(0);

    for (i = 0; i < 8; i++) {
        if ((data & 0x80) == 0x80)
            panel_spi_set_sdi(1);
        else
            panel_spi_set_sdi(0);

        aic_delay_us(1);
        panel_spi_set_scl(1);
        aic_delay_us(1);
        panel_spi_set_scl(0);
        aic_delay_us(1);
        data = data << 1;
    }
    panel_spi_set_cs(1);
    panel_spi_set_scl(0);
    panel_spi_set_sdi(0);
    aic_delay_us(1);
}

void panel_spi_device_emulation(char *cs, char *sdi, char *scl)
{
    panel_get_gpio(&spi.cs, cs);
    panel_get_gpio(&spi.scl, scl);
    panel_get_gpio(&spi.sdi, sdi);

    panel_spi_set_cs(1);
    panel_spi_set_scl(0);
    panel_spi_set_sdi(0);
    aic_delay_us(1);

    panel_spi_emulation = true;
}
#else
void panel_spi_cmd_wr(u8 cmd)
{
    u32 i;

    if (!panel_spi_emulation)
        return;

    aic_delay_us(1);
    panel_spi_set_cs(0);
    aic_delay_us(1);
    panel_spi_set_dc(0);

    for (i = 0; i < 8; i++) {
        if ((cmd & 0x80) == 0x80)
            panel_spi_set_sdi(1);
        else
            panel_spi_set_sdi(0);

        cmd = cmd << 1;
        aic_delay_us(1);
        panel_spi_set_scl(1);
        aic_delay_us(1);
        panel_spi_set_scl(0);
    }

    aic_delay_us(1);
    panel_spi_set_cs(1);
    aic_delay_us(1);
}

void panel_spi_data_wr(u8 data)
{
    u32 i;

    if (!panel_spi_emulation)
        return;

    aic_delay_us(1);
    panel_spi_set_cs(0);
    aic_delay_us(1);
    panel_spi_set_dc(1);

    for (i = 0; i < 8; i++) {
        if ((data & 0x80) == 0x80)
            panel_spi_set_sdi(1);
        else
            panel_spi_set_sdi(0);

        data = data << 1;
        aic_delay_us(1);
        panel_spi_set_scl(1);
        aic_delay_us(1);
        panel_spi_set_scl(0);
    }

    aic_delay_us(1);
    panel_spi_set_cs(1);
    aic_delay_us(1);
}
void panel_spi_device_emulation(char *cs, char *sdi, char *scl, char *dc)
{
    panel_get_gpio(&spi.cs, cs);
    panel_get_gpio(&spi.scl, scl);
    panel_get_gpio(&spi.sdi, sdi);
    panel_get_gpio(&spi.dc, dc);

    panel_spi_set_cs(1);
    panel_spi_set_scl(0);
    panel_spi_set_sdi(1);
    panel_spi_set_dc(1);
    aic_delay_us(1);

    panel_spi_emulation = true;
}
#endif
#endif
