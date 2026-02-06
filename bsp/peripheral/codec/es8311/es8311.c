/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "aic_hal_gpio.h"
#include "es8311.h"

#define DBG_TAG "es8311"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* es8311 address */
#define ES8311_ADDR     0x18
#define ARG_UNUSED(x)   ((void)x)

struct es8311_device
{
    struct rt_i2c_bus_device *i2c;
    uint32_t pin;
};

static struct es8311_device es8311_dev = {0};

static rt_size_t es8311_voice_fade(uint8_t fade);
static rt_size_t es8311_microphone_fade(uint8_t fade);

/* codec hifi mclk clock divider coefficients */
static const struct coeff_div coeff_div[] = {
    /*!<mclk     rate   pre_div  mult  adc_div dac_div fs_mode lrch  lrcl  bckdiv osr */
    /* 8k */
    {12288000, 8000, 0x06, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 8000, 0x03, 0x01, 0x03, 0x03, 0x00, 0x05, 0xff, 0x18, 0x10, 0x10},
    {16384000, 8000, 0x08, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 8000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 8000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000, 8000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 8000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000, 8000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 8000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1024000, 8000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 11.025k */
    {11289600, 11025, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 11025, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 11025, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 11025, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 12k */
    {12288000, 12000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 12000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 12000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 12000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 16k */
    {12288000, 16000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 16000, 0x03, 0x01, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
    {16384000, 16000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 16000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 16000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000, 16000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000, 16000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 16000, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1024000, 16000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 22.05k */
    {11289600, 22050, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 22050, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 22050, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 22050, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {705600, 22050, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 24k */
    {12288000, 24000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 24000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 24000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 24000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 24000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 32k */
    {12288000, 32000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 32000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
    {16384000, 32000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 32000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 32000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000, 32000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 32000, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000, 32000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 32000, 0x03, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
    {1024000, 32000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 44.1k */
    {11289600, 44100, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 44100, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 44100, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 44100, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 48k */
    {12288000, 48000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 48000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 48000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 64k */
    {12288000, 64000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 64000, 0x03, 0x02, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {16384000, 64000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 64000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 64000, 0x01, 0x02, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {4096000, 64000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 64000, 0x01, 0x03, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {2048000, 64000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 64000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0xbf, 0x03, 0x18, 0x18},
    {1024000, 64000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

    /* 88.2k */
    {11289600, 88200, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 88200, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 88200, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 88200, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

    /* 96k */
    {12288000, 96000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 96000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 96000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 96000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 96000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
};

static rt_size_t reg_read(uint8_t addr, uint8_t *reg_val)
{
    struct rt_i2c_msg msg[2] = {0};

    msg[0].addr  = ES8311_ADDR;
    msg[0].flags = RT_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = &addr;

    msg[1].addr  = ES8311_ADDR;
    msg[1].flags = RT_I2C_RD;
    msg[1].len   = 1;
    msg[1].buf   = reg_val;

    if (rt_i2c_transfer(es8311_dev.i2c, msg, 2) != 2) {
        LOG_E("I2C read data failed, reg = 0x%02x. \n", addr);
        return RT_ERROR;
    }

    return RT_EOK;
}

static rt_size_t reg_write(uint8_t addr, uint8_t val)
{
    struct rt_i2c_msg msgs[1] = {0};
    uint8_t buff[2] = {0};

    buff[0] = addr;
    buff[1] = val;

    msgs[0].addr  = ES8311_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = buff;
    msgs[0].len   = 2;

    if (rt_i2c_transfer(es8311_dev.i2c, msgs, 1) != 1) {
        LOG_E("I2C write data failed, reg = 0x%2x. \n", addr);
        return RT_ERROR;
    }

    return RT_EOK;
}

static int get_coeff(uint32_t mclk, uint32_t rate)
{
    for (int i = 0; i < (sizeof(coeff_div) / sizeof(coeff_div[0])); i++) {
        if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk) {
            LOG_D("mclk:%d,rate:%d,i:%d\n", mclk, rate, i);
            return i;
        }
    }

    return -1;
}

rt_size_t es8311_sample_frequency_config(int mclk_frequency, int sample_frequency)
{
    uint8_t regv;
    /* Get clock coefficients from coefficient table */
    int coeff = get_coeff(mclk_frequency, sample_frequency);

    if (coeff < 0) {
        LOG_E("Unable to configure sample rate %dHz with %dHz MCLK", sample_frequency, mclk_frequency);
        return RT_EINVAL;
    }

    const struct coeff_div *const selected_coeff = &coeff_div[coeff];

    /* register 0x02 */
    reg_read(ES8311_CLK_MANAGER_REG02, &regv);
    regv &= 0x07;
    regv |= (selected_coeff->pre_div - 1) << 5;
    regv |= selected_coeff->pre_multi << 3;
    reg_write(ES8311_CLK_MANAGER_REG02, regv);
    /* register 0x03 */
    const uint8_t reg03 = (selected_coeff->fs_mode << 6) | selected_coeff->adc_osr;
    reg_write(ES8311_CLK_MANAGER_REG03, reg03);
    /* register 0x04 */
    reg_write(ES8311_CLK_MANAGER_REG04, selected_coeff->dac_osr);
    /* register 0x05 */
    const uint8_t reg05 = ((selected_coeff->adc_div - 1) << 4) | (selected_coeff->dac_div - 1);
    reg_write(ES8311_CLK_MANAGER_REG05, reg05);
    /* register 0x06 */
    reg_read(ES8311_CLK_MANAGER_REG06, &regv);
    regv &= 0xE0;

    if (selected_coeff->bclk_div < 19) {
        regv |= (selected_coeff->bclk_div - 1) << 0;
    } else {
        regv |= (selected_coeff->bclk_div) << 0;
    }

    reg_write(ES8311_CLK_MANAGER_REG06, regv);
    /* register 0x07 */
    reg_read(ES8311_CLK_MANAGER_REG07, &regv);
    regv &= 0xC0;
    regv |= selected_coeff->lrck_h << 0;
    reg_write(ES8311_CLK_MANAGER_REG07, regv);
    /* register 0x08 */
    reg_write(ES8311_CLK_MANAGER_REG08, selected_coeff->lrck_l);

    return RT_EOK;
}

static rt_size_t es8311_clock_config(const es8311_clock_config_t *const clk_cfg, uint32_t res)
{
    uint8_t reg06;
    uint8_t reg01 = 0x3F; // Enable all clocks
    int mclk_hz;

    /* Select clock source for internal MCLK and determine its frequency */
    if (clk_cfg->mclk_from_mclk_pin) {
        mclk_hz = clk_cfg->mclk_frequency;
    } else {
        mclk_hz = clk_cfg->sample_frequency * (int)res * 2;
        reg01 |= BIT(7); // Select BCLK (a.k.a. SCK) pin
    }

    if (clk_cfg->mclk_inverted) {
        reg01 |= BIT(6); // Invert MCLK pin
    }
    reg_write(ES8311_CLK_MANAGER_REG01, reg01);

    reg_read(ES8311_CLK_MANAGER_REG06, &reg06);
    if (clk_cfg->sclk_inverted) {
        reg06 |= BIT(5);
    } else {
        reg06 &= ~BIT(5);
    }
    reg_write(ES8311_CLK_MANAGER_REG06, reg06);

    /* Configure clock dividers */
    return es8311_sample_frequency_config(mclk_hz, clk_cfg->sample_frequency);
}

rt_size_t es8311_microphone_config(bool digital_mic)
{
    uint8_t reg14 = 0x1a;

    /* PDM digital microphone enable or disable */
    if (digital_mic)
        reg14 |= BIT(6);

    reg_write(ES8311_ADC_REG17, 0xc0); // Set ADC gain @todo move this to ADC config section

    return reg_write(ES8311_SYSTEM_REG14, reg14);
}

rt_size_t es8311_voice_mute(bool mute)
{
    uint8_t reg31;

    reg_read(ES8311_DAC_REG31, &reg31);

    if (mute) {
        reg31 |= BIT(6) | BIT(5);
    } else {
        reg31 &= ~(BIT(6) | BIT(5));
    }

    return reg_write(ES8311_DAC_REG31, reg31);
}

static rt_size_t es8311_voice_fade(uint8_t fade)
{
    uint8_t reg37;

    reg_read(ES8311_DAC_REG37, &reg37);
    reg37 &= 0x0F;
    reg37 |= (fade << 4);

    return reg_write(ES8311_DAC_REG37, reg37);
}

static rt_size_t es8311_microphone_fade(uint8_t fade)
{
    uint8_t reg15;

    reg_read(ES8311_ADC_REG15, &reg15);
    reg15 &= 0x0F;
    reg15 |= (fade << 4);

    return reg_write(ES8311_ADC_REG15, reg15);
}

static uint32_t es8388_get_mclk_by_sample(int sample_rate, int mclk_nfs)
{
    uint32_t module_rate, mclk;
    uint8_t div = 0;

    switch (sample_rate) {
    case I2S_SAMPLE_RATE_88200:
    case I2S_SAMPLE_RATE_44100:
    case I2S_SAMPLE_RATE_22050:
    case I2S_SAMPLE_RATE_11025:
        module_rate = 22579200;
        break;
    case I2S_SAMPLE_RATE_96000:
    case I2S_SAMPLE_RATE_48000:
    case I2S_SAMPLE_RATE_32000:
    case I2S_SAMPLE_RATE_24000:
    case I2S_SAMPLE_RATE_16000:
    case I2S_SAMPLE_RATE_12000:
    case I2S_SAMPLE_RATE_8000:
    default:
        module_rate = 24576000;
        break;
    }

    switch (mclk_nfs)
    {
    case 512:
        div = 1;
        break;
    case 128:
        div = 4;
        break;
    case 64:
        div = 8;
        break;
    case 32:
        div = 16;
        break;
    case 16:
        div = 32;
        break;
    case 256:
    default:
        div = 2;
        break;
    }

    mclk = module_rate / div;

    return mclk;
}

int es8311_init(struct codec *codec)
{
    es8311_dev.i2c = rt_i2c_bus_device_find(codec->i2c_name);
    if (es8311_dev.i2c == RT_NULL)
    {
        rt_kprintf("%s bus not found\n", codec->i2c_name);
        return RT_ERROR;
    }

    es8311_dev.pin = codec->pa;
    rt_pin_mode(es8311_dev.pin, PIN_MODE_OUTPUT);

    /* Reset ES8311 to its default */
    reg_write(ES8311_RESET_REG00, 0x1F);
    rt_thread_mdelay(20);
    reg_write(ES8311_RESET_REG00, 0x00);
    reg_write(ES8311_RESET_REG00, 0x80); // Power-on command

    reg_write(ES8311_SYSTEM_REG0D, 0x01); // Power up analog circuitry - NOT default
    reg_write(ES8311_SYSTEM_REG0E, 0x02); // Enable analog PGA, enable ADC modulator - NOT default
    reg_write(ES8311_SYSTEM_REG12, 0x00); // power-up DAC - NOT default
    reg_write(ES8311_SYSTEM_REG13, 0x10); // Enable output to HP drive - NOT default
    reg_write(ES8311_ADC_REG1C, 0x6A); // ADC Equalizer bypass, cancel DC offset in digital domain
    reg_write(ES8311_DAC_REG37, 0x18); // Bypass DAC equalizer - NOT default
    reg_write(ES8311_DAC_REG32, 0xBF); // DAC set default vol

    return RT_EOK;
}

int es8311_start(struct codec *codec, i2s_stream_t stream)
{
    uint8_t reg09, reg0a;
    ARG_UNUSED(codec);

    reg_read(ES8311_SDPIN_REG09, &reg09);
    reg_read(ES8311_SDPOUT_REG0A, &reg0a);
    reg_write(ES8311_SYSTEM_REG0D, 0x2);

    if (!stream) {      //Playback
        reg0a &= ~(1 << 6);
        reg_write(ES8311_SDPOUT_REG0A, reg0a);
        es8311_voice_fade(ES8311_FADE_4LRCK);
    } else {
        es8311_microphone_config(false);
        es8311_microphone_fade(ES8311_FADE_4LRCK);
    }

    return RT_EOK;
}

int es8311_set_protocol(struct codec *codec, i2s_format_t *format)
{
    uint8_t reg_val;

    if (!format->stream)    // Playback
        reg_read(ES8311_SDPOUT_REG0A, &reg_val);
    else   // Record
        reg_read(ES8311_SDPIN_REG09, &reg_val);


    switch (format->protocol)
    {
    case I2S_PROTOCOL_LEFT_J:
        reg_val |= (1 << 0);
        break;
    case I2S_PCM_SHORT:
    case I2S_PCM_LONG:
        reg_val |= (3 << 0);
        break;
    case I2S_PROTOCOL_I2S:
        reg_val &= 0xFC;
    default:
        break;
    }

    if (!format->stream)
        reg_write(ES8311_SDPOUT_REG0A, reg_val);
    else
        reg_write(ES8311_SDPIN_REG09, reg_val);

    return 0;
}

int es8311_set_polarity(struct codec *codec, i2s_format_t *format)
{
    uint8_t reg_val;

    if (!format->stream)    // Playback
        reg_read(ES8311_SDPOUT_REG0A, &reg_val);
    else   // Record
        reg_read(ES8311_SDPIN_REG09, &reg_val);

    reg_val &= ~(1 << 5);
    reg_val |= (format->polarity << 5);

    if (!format->stream)
        reg_write(ES8311_SDPOUT_REG0A, reg_val);
    else
        reg_write(ES8311_SDPIN_REG09, reg_val);

    return RT_EOK;
}

int es8311_set_sample_width(struct codec *codec, i2s_format_t *format)
{
    uint8_t reg00;
    uint8_t reg_val;
    uint32_t mclk;
    uint8_t width = format->width;

    es8311_clock_config_t clk_cfg = {0};

    LOG_D("ES8311 in Slave mode and I2S format\n");
    reg_read(ES8311_RESET_REG00, &reg00);
    reg00 &= 0xBF;  // Slave serial port - default
    reg_write(ES8311_RESET_REG00, reg00);

    if (!format->stream)    // Playback
        reg_read(ES8311_SDPOUT_REG0A, &reg_val);
    else   // Record
        reg_read(ES8311_SDPIN_REG09, &reg_val);

    switch (width) {
    case ES8311_RESOLUTION_16:
        reg_val |= (3 << 2);
        break;
    case ES8311_RESOLUTION_18:
        reg_val |= (2 << 2);
        break;
    case ES8311_RESOLUTION_20:
        reg_val |= (1 << 2);
        break;
    case ES8311_RESOLUTION_24:
        reg_val |= (0 << 2);
        break;
    case ES8311_RESOLUTION_32:
        reg_val |= (4 << 2);
        break;
    default:
        return RT_EINVAL;
    }

    if (!format->stream)
        reg_write(ES8311_SDPOUT_REG0A, reg_val);
    else
        reg_write(ES8311_SDPIN_REG09, reg_val);

    mclk = es8388_get_mclk_by_sample(format->rate, format->mclk_nfs);

    clk_cfg.mclk_inverted = format->polarity;
    clk_cfg.sclk_inverted = format->polarity;
    clk_cfg.mclk_from_mclk_pin = 1;
    clk_cfg.mclk_frequency = mclk;
    clk_cfg.sample_frequency = format->rate;

    es8311_clock_config(&clk_cfg, format->width);

    return RT_EOK;
}

void es8311_volume_set(struct codec *codec, uint8_t volume)
{
    int reg32;

    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }

    if (volume == 0) {
        reg32 = 0;
    } else {
        reg32 = ((volume) * 256 / 100) - 1;
    }

    reg_write(ES8311_DAC_REG32, reg32);
}

uint8_t es8311_volume_get(struct codec *codec)
{
    uint8_t reg32, volume;

    reg_read(ES8311_DAC_REG32, &reg32);

    if (reg32 == 0) {
        volume = 0;
    } else {
        volume = ((reg32 * 100) / 256) + 1;
    }
    return volume;
}

void es8311_pa_power(struct codec *codec, uint8_t enable)
{
    if (enable)
        rt_pin_write(es8311_dev.pin, PIN_HIGH);
    else
        rt_pin_write(es8311_dev.pin, PIN_LOW);
}

void es8311_dump_reg(struct codec *codec)
{
    uint8_t value;
    int i;

    for (i = 0; i < 0x4A; i++) {
        reg_read(i, &value);

        if (i % 16 == 0)
            rt_kprintf("0x%02x: ", i);

        rt_kprintf("0x%02x   ", value);

        if ((i + 1) % 16 == 0)
            rt_kprintf("\n");
    }
}

struct codec_ops es8311_ops =
{
    .init = es8311_init,
    .start = es8311_start,
    .stop = NULL,
    .set_protocol = es8311_set_protocol,
    .set_polarity = es8311_set_polarity,
    .set_channel = NULL,
    .set_sample_width = es8311_set_sample_width,
    .set_sample_rate = NULL,
    .set_sclk = NULL,
    .set_mclk = NULL,
    .set_volume = es8311_volume_set,
    .get_volume = es8311_volume_get,
    .pa_power = es8311_pa_power,
    .dump_reg = es8311_dump_reg,
};

static struct codec es8311 =
{
    .name = "es8311",
    .i2c_name = AIC_I2S_CODEC_ES8311_I2C,
    .addr = ES8311_ADDR,
    .pa_name = AIC_I2S_CODEC_PA_PIN,
    .ops = &es8311_ops,
};

int rt_hw_es8311_init(void)
{
    es8311.pa = hal_gpio_name2pin(es8311.pa_name);
    codec_register(&es8311);
    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_es8311_init);
