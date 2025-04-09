/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "cs4344.h"

static uint32_t g_power_pin = 0;

int cs4344_init(struct codec *codec)
{
    return 0;
}

int cs4344_start(struct codec *codec, i2s_stream_t stream)
{
    return 0;
}

int cs4344_stop(struct codec *codec, i2s_stream_t stream)
{
    return 0;
}

void cs4344_pa_power(struct codec *codec, uint8_t enable)
{
    rt_pin_mode(g_power_pin, PIN_MODE_OUTPUT);

    if (enable)
        rt_pin_write(g_power_pin, PIN_HIGH);
    else
        rt_pin_write(g_power_pin, PIN_LOW);
}

struct codec_ops cs4344_ops =
{
    .init = cs4344_init,
    .start = cs4344_start,
    .stop = cs4344_stop,
    .pa_power = cs4344_pa_power,
};

static struct codec cs4344 =
{
    .name = "cs4344",
    .pa_name = AIC_I2S_CODEC_PA_PIN,
    .ops = &cs4344_ops,
};

int rt_hw_cs4344_init(void)
{
    g_power_pin = rt_pin_get(cs4344.pa_name);
    codec_register(&cs4344);
    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_cs4344_init);

