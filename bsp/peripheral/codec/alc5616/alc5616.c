/*
 * Copyright (c) 2026-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xyg <yiguan.xu@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "aic_hal_gpio.h"
#include "alc5616.h"

#define ALC5616_ADDR 0x1B

struct alc5616_device {
    struct rt_i2c_bus_device *i2c;
};

typedef struct {
    uint8_t reg_addr;
    uint16_t reg_value;
    const char *description;
} alc5616_reg_init_t;

static struct alc5616_device alc5616_dev = { 0 };

static const uint8_t g_sample_width[] = { 16, 20, 24, 8 };

static const alc5616_reg_init_t alc5616_init_table[] = {

    { ALC5616_SW_RESET, 0x0000, "Software Reset" },
    { ALC5616_GLOBAL_CLK_CTRL, 0x0000, "Global Clock: SYSCLK from MCLK" },

    /* I2S slave, 16bit, I2S format */
    { ALC5616_I2S1_CTRL, 0x8000, "I2S1: Slave Mode, 16-bit, I2S Format" },

    /* Power */
    { ALC5616_PWR_MGMT1, 0x9806, "Power Management 1: I2S1 + DACL/R + ADCL/R" },
    { ALC5616_PWR_MGMT2, 0x8800, "Power Management 2: ADC/DAC digital filter" },
    { ALC5616_PWR_MGMT3, 0xB800, "Power Management 3: VREF + MBIAS + LOUT" },
    { ALC5616_PWR_MGMT4, 0x8820, "Power Management 4: BST1 + MICBIAS1 + MIC1 single-ended" },
    { ALC5616_PWR_MGMT5, 0xCC00, "Power Management 5: OUTMIXL/R + RECMIXL/R" },
    { ALC5616_PWR_MGMT6, 0x0000, "Power Management 6" },

    /* Private register: clock generator */
    { ALC5616_PR_INDEX, 0x003D, "PR Index: 0x3D" },
    { ALC5616_PR_DATA, 0x3600, "PR Data: ADC/DAC clock generator enable" },

    /* Playback path: DAC direct to differential LOUT */
    { ALC5616_LOUTMIX_CTRL, 0x3000, "LOUTMIX: DACL/R to LOUT, mute OUTVOL path" },
    { ALC5616_LINE_OUT_CTRL1, 0xC8C8, "Line Out mute, 0dB" },
    { ALC5616_LINE_OUT_CTRL2, 0x8000, "Differential Line Output Enable" },
    { ALC5616_DAC_DIG_MIXER, 0x0000, "DAC Digital Mixer: normal DAC path" },
    { ALC5616_DAC_DIG_VOL, 0xAFAF, "DAC Digital Volume: 0dB" },

    /* Record path: MIC1 single-ended */
    { ALC5616_IN1_IN2, 0x3000, "MIC1 Boost Control: +30dB" },
    { ALC5616_IN1_VOL_CTL, 0x0808, "Input Volume Control: 0dB" },
    { ALC5616_RECMIXL_CTRL2, 0x006D, "RECMIXL: Unmute BST1" },
    { ALC5616_RECMIXR_CTRL2, 0x006D, "RECMIXR: Unmute BST1" },
    { ALC5616_ADC_DIG_MIXER, 0x3820, "ADC Mixer: Unmute ADC L/R" },
    { ALC5616_ADC_DIG_VOL, 0x2F2F, "ADC Digital Volume: 0dB" },

    /* Clock / pop noise control */
    { ALC5616_SOFT_VOL_ZCD, 0x8809, "Soft Volume & ZCD Enable" },
    { ALC5616_ADC_DAC_CLK1, 0x1104, "ADC & DAC Clock 1" },
    { ALC5616_ADC_DAC_CLK2, 0x0C00, "ADC & DAC Clock 2" },

    { ALC5616_I2S1_GNL_CTL, 0x0001, "General Control: MCLK input enable" },
};

#define ALC5616_INIT_TABLE_SIZE (ARRAY_SIZE(alc5616_init_table))

static int reg_read(uint8_t addr)
{
    struct rt_i2c_msg msg[2] = { 0 };
    uint8_t val_buf[2] = { 0 };
    int val = -RT_ERROR;

    msg[0].addr = ALC5616_ADDR;
    msg[0].flags = RT_I2C_WR;
    msg[0].len = 1;
    msg[0].buf = &addr;

    msg[1].addr = ALC5616_ADDR;
    msg[1].flags = RT_I2C_RD;
    msg[1].len = 2;
    msg[1].buf = val_buf;

    if (rt_i2c_transfer(alc5616_dev.i2c, msg, 2) != 2) {
        rt_kprintf("I2C read failed, reg = 0x%02x\n", addr);
        return -RT_ERROR;
    }
    val = (val_buf[0] << 8) | val_buf[1];
    return val;
}

static int reg_write(uint8_t addr, uint16_t val)
{
    struct rt_i2c_msg msgs[1] = { 0 };
    uint8_t buff[3] = { 0 };

    buff[0] = addr;
    buff[1] = (val >> 8) & 0xFF;
    buff[2] = val & 0xFF;

    msgs[0].addr = ALC5616_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = buff;
    msgs[0].len = 3;

    if (rt_i2c_transfer(alc5616_dev.i2c, msgs, 1) != 1) {
        rt_kprintf("I2C write failed, reg = 0x%02x\n", addr);
        return -RT_ERROR;
    }
    return RT_EOK;
}

int alc5616_init(struct codec *codec)
{
    int ret = 0;

    alc5616_dev.i2c = rt_i2c_bus_device_find(codec->i2c_name);
    if (alc5616_dev.i2c == RT_NULL) {
        rt_kprintf("%s bus not found\n", codec->i2c_name);
        return -RT_ERROR;
    }
    /* Define power pins */
    uint32_t ivdd = hal_gpio_name2pin("PE.5");
    uint32_t dbvdd = hal_gpio_name2pin("PE.6");
    uint32_t micvdd = hal_gpio_name2pin("PE.7");
    /* Configure ivdd and dbvdd as output */
    rt_pin_mode(ivdd, PIN_MODE_OUTPUT);
    rt_pin_mode(dbvdd, PIN_MODE_OUTPUT);
    /* Enable ivdd and dbvdd first */
    rt_pin_write(ivdd, 1);
    rt_pin_write(dbvdd, 1);
    rt_thread_delay(50);
    /* Enable micvdd after core power is stable */
    rt_pin_mode(micvdd, PIN_MODE_OUTPUT);
    rt_pin_write(micvdd, 1);

    for (int i = 0; i < ALC5616_INIT_TABLE_SIZE; i++) {
        const alc5616_reg_init_t *entry = &alc5616_init_table[i];

        ret = reg_write(entry->reg_addr, entry->reg_value);
        if (ret == (-RT_ERROR)) {
            rt_kprintf("Reg 0x%02X write failed\n", entry->reg_addr);
            return -RT_ERROR;
        }

        if (entry->reg_addr == ALC5616_PWR_MGMT3) {
            rt_thread_delay(50);
        }

        if (entry->reg_addr == ALC5616_HP_AMP_CTRL1) {
            if (entry->reg_value == 0x000C) {
                rt_thread_delay(10);
            } else if (entry->reg_value == 0x001C) {
                rt_thread_delay(5);
            }
        }
#if 0
        uint16_t read_val = reg_read(entry->reg_addr);
        int match = (read_val == entry->reg_value);
        int pass_count = 0;
        int fail_count = 0;
        if (match) {
            pass_count++;
        } else {
            fail_count++;
        }

        rt_kprintf("  [%2d] 0x%02X <- 0x%04X, Read: 0x%04X %s  // %s\n",
                                                                        i + 1,
                                                                        entry->reg_addr,
                                                                        entry->reg_value,
                                                                        read_val,
                                                                        match ? "[OK]" : "[FAIL]",
                                                                        entry->description);
#endif
    }
    return RT_EOK;
}

int alc5616_start(struct codec *codec, i2s_stream_t stream)
{
    if (!stream) {
        /* Playback: Unmute Line Output, 0dB Volume */
        reg_write(ALC5616_LINE_OUT_CTRL1, 0x0808);
    } else {
        reg_write(ALC5616_ADC_DIG_VOL, 0x2F2F); /* 0dB, Unmute */
    }

    return RT_EOK;
}

int alc5616_stop(struct codec *codec, i2s_stream_t stream)
{
    if (!stream) {
        /* Playback: Mute Line Output */
        reg_write(ALC5616_LINE_OUT_CTRL1, 0xC8C8);
    } else {
        /* Record: Mute ADC Digital Volume */
        reg_write(ALC5616_ADC_DIG_VOL, 0xAFAF);
    }

    return RT_EOK;
}

int alc5616_set_protocol(struct codec *codec, i2s_format_t *format)
{
    uint16_t reg_val = reg_read(ALC5616_I2S1_CTRL);

    reg_val &= ~(0x3 << 0);
    switch (format->protocol) {
        case I2S_PROTOCOL_LEFT_J:
            reg_val |= (1 << 0);
            break;

        case I2S_PCM_SHORT:
        case I2S_PCM_LONG:
        default:
            reg_val |= (0 << 0); /* 00: I2S Format */
            break;
    }
    reg_write(ALC5616_I2S1_CTRL, reg_val);

    return RT_EOK;
}

int alc5616_set_sample_width(struct codec *codec, i2s_format_t *format)
{
    uint16_t reg_val = 0, i;

    for (i = 0; i < ARRAY_SIZE(g_sample_width); i++) {
        if (g_sample_width[i] == format->width)
            break;
    }

    if (i == ARRAY_SIZE(g_sample_width)) {
        rt_kprintf("alc5616 not support sample width\n");
        return -RT_ERROR;
    }

    reg_val = reg_read(ALC5616_I2S1_CTRL);
    reg_val &= ~(0x3 << 2);

    switch (format->width) {
        case 16:
            reg_val |= (0 << 2);
            break;
        case 20:
            reg_val |= (1 << 2);
            break;
        case 24:
            reg_val |= (2 << 2);
            break;
        default:
            reg_val |= (0 << 2);
            break;
    }
    reg_write(ALC5616_I2S1_CTRL, reg_val);

    return RT_EOK;
}

void alc5616_dump_reg(struct codec *codec)
{
    rt_kprintf("\n");
    rt_kprintf("========================================\n");
    rt_kprintf("  ALC5616 Register Dump\n");
    rt_kprintf("========================================\n\n");
    rt_kprintf("Addr   Value    Description\n");
    rt_kprintf("---------------------------\n");

    for (int i = 0; i < ALC5616_INIT_TABLE_SIZE; i++) {
        const alc5616_reg_init_t *entry = &alc5616_init_table[i];
        uint16_t reg_val = reg_read(entry->reg_addr);
        rt_kprintf("0x%02X   0x%04X   %s\n", entry->reg_addr, reg_val, entry->description);
    }
    rt_kprintf("\n========================================\n");
}

struct codec_ops alc5616_ops = {
    .init = alc5616_init,
    .start = alc5616_start,
    .stop = alc5616_stop,
    .set_protocol = alc5616_set_protocol,
    .set_sample_width = alc5616_set_sample_width,
    .dump_reg = alc5616_dump_reg,
};

static struct codec g_alc5616 = {
    .name = "alc5616",
    .i2c_name = AIC_I2S_CODEC_ALC5616_I2C,
    .addr = ALC5616_ADDR,
    .ops = &alc5616_ops,
};

int rt_hw_alc5616_init(void)
{
    codec_register(&g_alc5616);
    return RT_EOK;
}

INIT_DEVICE_EXPORT(rt_hw_alc5616_init);
