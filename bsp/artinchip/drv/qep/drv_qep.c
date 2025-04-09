/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include "drivers/pulse_encoder.h"
#include <string.h>

#define LOG_TAG         "QEP"
#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_qep.h"

struct aic_qep {
    struct rt_pulse_encoder_device rtdev;
    struct aic_qep_data *data;
};

static struct aic_qep *g_qep[AIC_QEP_CH_NUM];
static struct aic_qep_data g_qep_info[] = {
#ifdef AIC_USING_QEP0
    {.id = 0,},
#endif
#ifdef AIC_USING_QEP1
    {.id = 1,},
#endif
#ifdef AIC_USING_QEP2
    {.id = 2,},
#endif
#ifdef AIC_USING_QEP3
    {.id = 3,},
#endif
};

static rt_err_t aic_qep_init(struct rt_pulse_encoder_device *qep)
{
    struct aic_qep *aicqep;

    RT_ASSERT(qep != RT_NULL);

    aicqep = (struct aic_qep *)qep;

    hal_qep_config(aicqep->data->id);

    return RT_EOK;
}

static rt_int32_t aic_qep_get_count(struct rt_pulse_encoder_device *qep)
{
    struct aic_qep *aicqep;

    RT_ASSERT(qep != RT_NULL);

    aicqep = (struct aic_qep *)qep;

    rt_int32_t get_cnt = 0;

    get_cnt = hal_qep_get_cnt(aicqep->data->id);

    return get_cnt;
}

static rt_err_t aic_qep_clear_count(struct rt_pulse_encoder_device *qep)
{
    struct aic_qep *aicqep;

    RT_ASSERT(qep != RT_NULL);

    aicqep = (struct aic_qep *)qep;

    LOG_I("%s enter!, id:%d\n", __FUNCTION__, aicqep->data->id);

    return RT_EOK;
}

static rt_err_t aic_qep_control(struct rt_pulse_encoder_device *qep, rt_uint32_t cmd, void *args)
{
    rt_err_t result = RT_EOK;
    struct aic_qep *aicqep;
    rt_uint32_t *p;;

    RT_ASSERT(qep != RT_NULL);

    aicqep = (struct aic_qep *)qep;

    hal_qep_set_cnt_ep(aicqep->data->id, 0xFFFFFFFF);
    hal_qep_set_cnt_cmp(aicqep->data->id, 0xFFFFFFFF);

    switch (cmd) {
    case PULSE_ENCODER_CMD_ENABLE:
        hal_qep_int_enable(aicqep->data->id, 1);
        hal_qep_enable(aicqep->data->id, 1);
        break;
    case PULSE_ENCODER_CMD_DISABLE:
        hal_qep_int_enable(aicqep->data->id, 0);
        hal_qep_enable(aicqep->data->id, 0);
        break;
    case PULSE_ENCODER_CMD_SET_COUNT:
        p = (rt_uint32_t *)args;
        hal_qep_set_cnt_ep(aicqep->data->id, *p - 1);
        hal_qep_set_cnt_cmp(aicqep->data->id, *p - 1);
        break;
    default:
        result = -RT_ENOSYS;
        break;
    }

    return result;
}


static struct rt_pulse_encoder_ops aic_qep_ops =
{
    .init = aic_qep_init,
    .get_count = aic_qep_get_count,
    .clear_count = aic_qep_clear_count,
    .control = aic_qep_control,
};

irqreturn_t aic_qep_irq(int irq, void *arg)
{
    u32 stat;
    u32 ch = 0;

    for (int i = 0; i < AIC_QEP_CH_NUM; i++) {
        stat = hal_qep_int_stat(i);
        if (stat & QEP_POS_CMP_INT_FLG) {
            ch = i;
            g_qep[i]->rtdev.parent.tx_complete(&g_qep[i]->rtdev.parent, &ch);
            hal_qep_clr_int(i, QEP_POS_CMP_INT_FLG);
        }
    }

    return IRQ_HANDLED;
}

static rt_err_t aic_qep_probe(struct aic_qep_data *pdata)
{
    struct aic_qep *qep;
    rt_err_t ret = RT_EOK;
    char aic_qep_device_name[10] = "";

    qep = (struct aic_qep *)malloc(sizeof(struct aic_qep));
    if (!qep) {
        LOG_E("Failed to malloc(%d)\n", (u32)sizeof(struct aic_qep));
        return -RT_ENOMEM;
    }

    qep->data = pdata;
    qep->rtdev.ops = &aic_qep_ops;

    /* store the pointer */
    g_qep[qep->data->id] = qep;

    snprintf(aic_qep_device_name, 10, "qep%d", qep->data->id);

    ret = rt_device_pulse_encoder_register(&qep->rtdev, aic_qep_device_name, NULL);
    if (ret == RT_EOK) {
        LOG_I("ArtInChip %s loaded", aic_qep_device_name);
    } else {
        LOG_E("%s register failed", aic_qep_device_name);
        goto err;
    }

    return ret;

err:
    if (qep)
        free(qep);

    return ret;
}

static int drv_qep_init(void)
{
    rt_err_t ret = RT_EOK;
    u32 i;

    for (i = 0; i < ARRAY_SIZE(g_qep_info); i++) {
        ret = aic_qep_probe(&g_qep_info[i]);
        if (ret)
            return ret;
    }

    aicos_request_irq(PWMCS_QEP_IRQn, aic_qep_irq, 0, NULL, NULL);

    hal_qep_init();

    return ret;
}
INIT_DEVICE_EXPORT(drv_qep_init);
