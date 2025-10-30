/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "aic_hal_gpio.h"
#include "tlv320aic3101.h"

#define TLV320_ADDR     0x18

static const uint8_t sample_width[] = {16, 20, 24, 32};

struct tlv320_device
{
    struct rt_i2c_bus_device *i2c;
    uint32_t pin;
};

static struct tlv320_device tlv320_dev = {0};

static uint8_t reg_read(uint8_t addr)
{
    struct rt_i2c_msg msg[2] = {0};
    uint8_t val = 0xff;

    msg[0].addr  = TLV320_ADDR;
    msg[0].flags = RT_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = &addr;

    msg[1].addr  = TLV320_ADDR;
    msg[1].flags = RT_I2C_RD;
    msg[1].len   = 1;
    msg[1].buf   = &val;

    if (rt_i2c_transfer(tlv320_dev.i2c, msg, 2) != 2)
    {
        rt_kprintf("I2C read data failed, reg = 0x%02x. \n", addr);
        return 0xff;
    }

    return val;
}

static void reg_write(uint8_t addr, uint8_t val)
{
    struct rt_i2c_msg msgs[1] = {0};
    uint8_t buff[2] = {0};

    buff[0] = addr;
    buff[1] = val;

    msgs[0].addr  = TLV320_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = buff;
    msgs[0].len   = 2;

    if (rt_i2c_transfer(tlv320_dev.i2c, msgs, 1) != 1)
    {
        rt_kprintf("I2C write data failed, reg = 0x%2x. \n", addr);
        return;
    }
}

int tlv320_init(struct codec *codec)
{
    tlv320_dev.i2c = rt_i2c_bus_device_find(codec->i2c_name);
    if (tlv320_dev.i2c == RT_NULL)
    {
        rt_kprintf("%s bus not found\n", codec->i2c_name);
        return -RT_ERROR;
    }

    //reset pin
    tlv320_dev.pin = codec->pa;
    rt_pin_mode(tlv320_dev.pin, PIN_MODE_OUTPUT);
    rt_pin_write(tlv320_dev.pin, 0);
    rt_thread_delay(1);
    rt_pin_write(tlv320_dev.pin, 1);
    rt_thread_delay(2);

    reg_write(TLV320_PAGE0_PSR, 0x0);
    /* Software Reset */
    reg_write(TLV320_PAGE0_SRR, 0x80);
    /* CODEC_CLKIN Uses CLKDIV_OUT */
    reg_write(TLV320_PAGE0_CR, 0x1);
    /* CLKDIV_IN uses MCLK. PLLCLK_IN uses MCLK */
    reg_write(TLV320_PAGE0_CGCR, 0x2);
    /* DAC_L1 is routed to LEFT_LOP/M */
    reg_write(TLV320_PAGE0_DACL1L, 0x80);
    /* DAC_R1 is routed to RIGHT_LOP/M */
    reg_write(TLV320_PAGE0_DACR1R, 0x80);
    /* Left Right data path */
    reg_write(TLV320_PAGE0_CDPSR, 0xA);
    /* I2S Mode 16bit  */
    reg_write(TLV320_PAGE0_ASDICRB, 0x00);
    /* Power On DAC*/
    reg_write(TLV320_PAGE0_DACPODCR, 0xC0);
    /* DAC R3/L3 select */
    reg_write(TLV320_PAGE0_DACOSCR, 0x50);
    /* 1bit clock offset -> I2S Mode */
    reg_write(TLV320_PAGE0_ASDICRC, 0x1);
    /* Power UP Left ADC Channel */
    reg_write(TLV320_PAGE0_MIC1LPLADC, 0x4);
    /* Power UP Right ADC Channel */
    reg_write(TLV320_PAGE0_MIC1RPRADC, 0x4);
    /* MICBIAS Output 2V */
    reg_write(TLV320_PAGE0_MICBIAS, 0x40);

    return RT_EOK;
}

int tlv320_start(struct codec *codec, i2s_stream_t stream)
{
    if (!stream) {
        /* Not Muted RIGHT_LOP/M and Powered Up */
        reg_write(TLV320_PAGE0_RIGHTOL, 0x0b);
        /* Not Muted LEFT_LOP/M and Powered Up */
        reg_write(TLV320_PAGE0_LEFTOL, 0x0b);
        /* Left DAC Not Mute & -24db */
        reg_write(TLV320_PAGE0_LDACDVCR, 0x30);
        /* Right DAC Not Mute & -24db */
        reg_write(TLV320_PAGE0_RDACDVCR, 0x30);
    } else {
        /* 31db & Left ADC PGA Not Mute*/
        reg_write(TLV320_PAGE0_LADCPGCR, 0x4f);
        /* 31db & Right ADC PGA Not Mute*/
        reg_write(TLV320_PAGE0_RADCPGCR, 0x4f);
    }

    return RT_EOK;
}

int tlv320_stop(struct codec *codec, i2s_stream_t stream)
{
    return RT_EOK;
}

int tlv320_set_protocol(struct codec *codec, i2s_format_t *format)
{
    uint8_t reg_val;
    reg_val = reg_read(TLV320_PAGE0_ASDICRB);

    switch (format->protocol)
    {
    case I2S_PROTOCOL_LEFT_J:
        reg_val |= (2 << 6);
        reg_write(TLV320_PAGE0_ASDICRC, 0);
        break;
    case I2S_PROTOCOL_RIGHT_J:
        reg_val |= (3 << 6);
        reg_write(TLV320_PAGE0_ASDICRC, 2);
        break;
    case I2S_PROTOCOL_I2S:
    default:
        break;
    }
    reg_write(TLV320_PAGE0_ASDICRB, reg_val);

    return RT_EOK;
}

int tlv320_set_sample_width(struct codec *codec, i2s_format_t *format)
{
    uint8_t reg_val = 0, i;

    for (i = 0; i < ARRAY_SIZE(sample_width); i++)
    {
        if (sample_width[i] == format->width)
            break;
    }

    if (i == ARRAY_SIZE(sample_width))
    {
        hal_log_err("tlv320 not support sample width\n");
        return -1;
    }

    reg_val = reg_read(TLV320_PAGE0_ASDICRB);
    reg_val |= (i << 4);
    reg_write(TLV320_PAGE0_ASDICRB, reg_val);

    return 0;
}

void tlv320_dump_reg(struct codec *codec)
{
    int i;
    uint8_t reg_val = 0;

    for (i = 0; i < TLV320_REG_MAX; i++) {
        reg_val = reg_read(i);

        if (i % 16 == 0)
            rt_kprintf("0x%02x: ", i);

        rt_kprintf("0x%02x   ", reg_val);

        if ((i + 1) % 16 == 0)
            rt_kprintf("\n");
    }
}


struct codec_ops tlv320_ops =
{
    .init = tlv320_init,
    .start = tlv320_start,
    .stop = tlv320_stop,
    .set_protocol = tlv320_set_protocol,
    .set_sample_width = tlv320_set_sample_width,
    .dump_reg = tlv320_dump_reg,
};

static struct codec tlv320 =
{
    .name = "tlv320",
    .i2c_name = AIC_I2S_CODEC_TLV320_I2C,
    .addr = TLV320_ADDR,
    .pa_name = AIC_I2S_CODEC_PA_PIN,
    .ops = &tlv320_ops,
};

int rt_hw_tlv320_init(void)
{
    tlv320.pa = hal_gpio_name2pin(tlv320.pa_name);
    codec_register(&tlv320);
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_tlv320_init);
