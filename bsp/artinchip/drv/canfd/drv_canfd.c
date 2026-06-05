/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao Li <siyao.li@artinchip.com>
 */
#include <stdio.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <string.h>
#include <aic_osal.h>
#include <getopt.h>
#include <drivers/pm.h>

#include "hal_canfd.h"

struct aic_canfd
{
    struct rt_can_device can;
    struct canfd_handle canfdHandle;
    char *name;
    volatile int pm_ref;
};

struct aic_canfd aic_canfd_arr[] =
{
#ifdef AIC_USING_CANFD0
    {
        .name = "canfd0",
        .canfdHandle = {
            .idx = 0,
            .canfd_base = CANFD0_BASE,
            .irq_num = CANFD0_IRQn,
            .clk_id = CLK_CANFD0
        }
    },
#endif
#ifdef AIC_USING_CANFD1
    {
        .name = "canfd1",
        .canfdHandle = {
            .idx = 1,
            .canfd_base = CANFD1_BASE,
            .irq_num = CANFD1_IRQn,
            .clk_id = CLK_CANFD1
        }
    },
#endif
#ifdef AIC_USING_R_CANFD0
    {
        .name = "r_canfd0",
        .canfdHandle = {
            .idx = 2,
            .canfd_base = R_CANFD0_BASE,
            .irq_num = R_CANFD0_IRQn,
            .clk_id = CLK_R_CANFD0
        }
    },
#endif
#ifdef AIC_USING_R_CANFD1
    {
        .name = "r_canfd1",
        .canfdHandle = {
            .idx = 3,
            .canfd_base = R_CANFD1_BASE,
            .irq_num = R_CANFD1_IRQn,
            .clk_id = CLK_R_CANFD1
        }
    },
#endif
};

static int aic_canfd_sw_filter_to_hw_filter(struct rt_can_filter_config *sw_filter,
                                            canfd_filter_config_t *hw_filter)
{
    RT_ASSERT(sw_filter);
    int i;

    /*
     * If using one hardware filter, use Single Filter Mode.
     * If using two hardware filters, use Dual Filter Mode.
     * If sw_filter->count == 0, close hardware filter, receiving all frames.
     */
    hw_filter->filter_chan = sw_filter->count;

    for (i = 0;i < sw_filter->count; i++) {
        pr_debug("[%d]\n", i);
        hw_filter->is_eff = sw_filter->items[i].ide;
        hw_filter->items[i].mask = sw_filter->items[i].mask;
        hw_filter->items[i].id = sw_filter->items[i].id;
    }

    return 0;
}

static rt_err_t aic_canfd_control(struct rt_can_device *can, int cmd, void *arg)
{
    RT_ASSERT(can != RT_NULL);
    struct aic_canfd *p_aic_can = (struct aic_canfd *)can;
    canfd_handle *phandle = &p_aic_can->canfdHandle;
    struct rt_can_status *status;
    struct rt_can_filter_config *sw_filter;
    int ret;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_SET_INT:
        hal_canfd_enable_int(phandle, 1);
        break;
    case RT_DEVICE_CTRL_CLR_INT:
        hal_canfd_enable_int(phandle, 0);
        break;
    case RT_CAN_CMD_SET_BAUD_FD:
        hal_canfd_ioctl(phandle, CANFD_IOCTL_SET_BAUDRATE, arg);
        break;
    case RT_CAN_CMD_GET_STATUS:
        status = (struct rt_can_status *)arg;
        status->rcvpkg = can->status.rcvpkg;
        status->dropedrcvpkg = can->status.dropedrcvpkg;
        status->sndpkg = can->status.sndpkg;
        status->dropedsndpkg = can->status.dropedsndpkg;
        status->rcverrcnt = phandle->status.recverrcnt;
        status->snderrcnt = phandle->status.snderrcnt;
        status->bitpaderrcnt = phandle->status.stufferrcnt;
        status->formaterrcnt = phandle->status.formaterrcnt;
        status->biterrcnt = phandle->status.biterrcnt;
        status->errcode = phandle->status.current_state;
        break;
    case RT_CAN_CMD_SET_FILTER:
        sw_filter = (struct rt_can_filter_config *)arg;
        if (sw_filter->count > AIC_CANFD_FILTER_MAX_CNT) {
                LOG_E("CANFD supports up to 16 filters!!!\n");
                return -RT_EINVAL;
        }

        if (phandle->hw_filter.filter_chan != sw_filter->count ||
            phandle->hw_filter.items == NULL) {
            if (phandle->hw_filter.items)
                free(phandle->hw_filter.items);
            rt_memset(&phandle->hw_filter, 0, sizeof(canfd_filter_config_t));
            phandle->hw_filter.items = malloc(sw_filter->count * sizeof(struct canfd_filter_item));
            if (phandle->hw_filter.items == NULL) {
                LOG_E("Memory allocation failed\n");
                return -RT_ENOMEM;
            }
            rt_memset(phandle->hw_filter.items, 0, sw_filter->count * sizeof(struct canfd_filter_item));
        }

        ret = aic_canfd_sw_filter_to_hw_filter(sw_filter, &phandle->hw_filter);
        if (ret) {
            free(phandle->hw_filter.items);
            phandle->hw_filter.items = NULL;
            return ret;
        }

        hal_canfd_ioctl(phandle, CANFD_IOCTL_SET_FILTER, (void *)&phandle->hw_filter);
        break;
    case RT_CAN_CMD_SET_MODE:
        memcpy(&phandle->mode, arg, sizeof(struct aic_canfd_mode_info));
        hal_canfd_ioctl(phandle, CANFD_IOCTL_SET_MODE, (void *)&phandle->mode);
        break;
#ifdef AIC_CANFD_GET_DATA_BY_DMA
    case RT_CAN_CONFIG_DMA_TX:
        phandle->obtain_data_mode = CANFD_OBTAIN_DATA_BY_DMA;
        struct canfd_dma_transfer_info *chan_info_tx;
        chan_info_tx = (struct canfd_dma_transfer_info *)arg;

        phandle->dma_tx_info.buf = chan_info_tx->buf;
        phandle->dma_tx_info.buf_size = chan_info_tx->buf_size;
        phandle->dma_tx_info.callback = chan_info_tx->callback;
        phandle->dma_tx_info.callback_param = chan_info_tx->callback_param;
        break;
    case RT_CAN_CONFIG_DMA_RX:
        phandle->obtain_data_mode = CANFD_OBTAIN_DATA_BY_DMA;
        struct canfd_dma_transfer_info *chan_info_rx;
        chan_info_rx = (struct canfd_dma_transfer_info *)arg;

        phandle->dma_rx_info.buf = chan_info_rx->buf;
        phandle->dma_rx_info.buf_size = chan_info_rx->buf_size;
        phandle->dma_rx_info.callback = chan_info_rx->callback;
        phandle->dma_rx_info.callback_param = chan_info_rx->callback_param;
        break;
    case RT_CAN_STOP_DMA:
        hal_canfd_stop_dma(phandle);
        break;
#endif
    default:
        LOG_E("cmd not support\n");
        break;
    }

    return RT_EOK;
}

static void aic_canfd_pm_get(struct aic_canfd *dev)
{
#ifdef RT_USING_PM
    rt_base_t level = rt_hw_interrupt_disable();
    if (dev->pm_ref++ == 0)
        rt_pm_module_request(PM_CAN_ID, PM_SLEEP_MODE_NONE);
    rt_hw_interrupt_enable(level);
#endif
}

static void aic_canfd_pm_put(struct aic_canfd *dev)
{
#ifdef RT_USING_PM
    rt_base_t level = rt_hw_interrupt_disable();
    if (--dev->pm_ref == 0)
        rt_pm_module_release(PM_CAN_ID, PM_SLEEP_MODE_NONE);
    rt_hw_interrupt_enable(level);
#endif
}

static int aic_canfd_send(struct rt_can_device *can, const void *buf,
                        rt_uint32_t boxno)
{
    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(buf != RT_NULL);
    RT_UNUSED(boxno);
    struct aic_canfd *p_aic_can = (struct aic_canfd *)can;
    struct rt_can_msg *pmsg = (struct rt_can_msg *)buf;
    canfd_handle *phandle = &p_aic_can->canfdHandle;
    canfd_msg_t msg;
    int i;

    msg.id = pmsg->id;
    msg.rtr = pmsg->rtr;
    msg.ide = pmsg->ide;
    msg.len = pmsg->len;
    msg.fdf = pmsg->fd_frame;
    msg.dlc = hal_canfd_len2dlc(pmsg->len);

    for (i = 0; i < msg.len; i++)
        msg.data[i] = pmsg->data[i];

    aic_canfd_pm_get(p_aic_can);
    hal_canfd_tx_frame(&p_aic_can->canfdHandle, &msg);
#ifdef AIC_CANFD_GET_DATA_BY_DMA
    hal_canfd_set_dma_request(&p_aic_can->canfdHandle, msg.len);
    hal_canfd_config_dma_tx(phandle);
#endif

#ifdef AIC_CANFD_GET_DATA_BY_CPU
    if (phandle->mode.tx_type != CANFD_TXTYPE_TTCAN) {
        if (phandle->mode.tx_mode != CANFD_TXMODE_PTB)
            canfd_reg_enable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_TSNEXT_FLAG);
        canfd_tx_active(phandle);
    } else {
        canfd_reg_enable(phandle->canfd_base, CANFD_TCMD_REG, CANFD_TCMD_TSALL_FLAG);
    }
#endif

    return 0;
}

static int aic_canfd_recv(struct rt_can_device *can, void *buf, rt_uint32_t boxno)
{
    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(buf != RT_NULL);
    RT_UNUSED(boxno);
    struct aic_canfd *p_aic_can = (struct aic_canfd *)can;
    struct rt_can_msg *pmsg = (struct rt_can_msg *)buf;
    canfd_handle *phandle = &p_aic_can->canfdHandle;
    int i;

    pmsg->id = phandle->msg.id;
    pmsg->fd_frame = phandle->msg.fdf;
    pmsg->rtr = phandle->msg.rtr;
    pmsg->ide = phandle->msg.ide;
    pmsg->len = hal_canfd_dlc2len(phandle->msg.dlc);
    pmsg->hdr = 0;

    for (i = 0; i < pmsg->len; i++) {
#ifdef AIC_CANFD_GET_DATA_BY_CPU
        pmsg->data[i] = phandle->msg.data[i];
#endif
#ifdef AIC_CANFD_GET_DATA_BY_DMA
        pmsg->data[i] = *(u8*)(phandle->dma_rx_info.buf + 8 + i);
#endif
    }

    aic_canfd_pm_put(p_aic_can);

    return 0;
}

static rt_err_t aic_canfd_configure(struct rt_can_device *can,
                              struct can_configure *cfg)
{
    return RT_EOK;
}

static const struct rt_can_ops aic_canfd_ops =
{
    aic_canfd_configure,
    aic_canfd_control,
    aic_canfd_send,
    aic_canfd_recv,
};

void aic_canfd_callback(canfd_handle * phandle, void *arg)
{
    struct aic_canfd *p_aic_can;
    struct rt_can_device *pcan;
    unsigned long event = (unsigned long)arg;

    p_aic_can = rt_container_of(phandle, struct aic_canfd, canfdHandle);
    pcan = (struct rt_can_device *)p_aic_can;

    switch (event) {
    case CAN_EVENT_RX_IND:
        aic_canfd_pm_get(p_aic_can);
        rt_hw_can_isr(pcan, RT_CAN_EVENT_RX_IND);
        break;
    case CAN_EVENT_TX_DONE:
        aic_canfd_pm_put(p_aic_can);
        rt_hw_can_isr(pcan, RT_CAN_EVENT_TX_DONE);
        break;
    case CAN_EVENT_RXOF_IND:
        rt_hw_can_isr(pcan, RT_CAN_EVENT_RXOF_IND);
        break;
    case CAN_EVENT_TX_FAIL:
        aic_canfd_pm_put(p_aic_can);
        rt_hw_can_isr(pcan, RT_CAN_EVENT_TX_FAIL);
        break;
    default:
        break;
    }
}

#ifdef RT_USING_PM
static int aic_canfd_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_canfd *p_aic_canfd = (struct aic_canfd *)device;
    canfd_handle *phandle = &p_aic_canfd->canfdHandle;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
#ifdef AIC_PM_DRV_V15
        hal_canfd_uninit(phandle);
#else
        hal_clk_disable(phandle->clk_id);
#endif
        break;
    default:
        break;
    }

    return 0;
}

static void aic_canfd_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_canfd *p_aic_canfd = (struct aic_canfd *)device;
    canfd_handle *phandle = &p_aic_canfd->canfdHandle;
    struct aic_canfd_allbaud_info set_baud = {0};

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
#ifdef AIC_PM_DRV_V15
        hal_canfd_init(phandle);

        /* Reset baudrate */
        set_baud.baud_type = phandle->can_type;
        set_baud.slow_baud.baudrate = phandle->s_baud;
        set_baud.fast_baud.baudrate = phandle->f_baud;
        hal_canfd_ioctl(phandle, CANFD_IOCTL_SET_BAUDRATE, &set_baud);

        /* Reset filter */
        hal_canfd_ioctl(phandle, CANFD_IOCTL_SET_FILTER, (void *)&phandle->hw_filter);

        /* Reset mode */
        hal_canfd_ioctl(phandle, CANFD_IOCTL_SET_MODE, (void*)&phandle->mode);

        /* Reset int */
        if (phandle->running)
            hal_canfd_enable_int(phandle, 1);
#else
        hal_clk_enable(phandle->clk_id);
#endif
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_canfd_pm_ops =
{
    SET_DEVICE_PM_OPS(aic_canfd_suspend, aic_canfd_resume)
    NULL,
};
#endif

int rt_hw_aic_canfd_init(void)
{
    int i, ret = 0;
    struct can_configure canfd_cfig = CANDEFAULTCONFIG;
    canfd_cfig.privmode = RT_CAN_MODE_NOPRIV;
    canfd_cfig.ticks = 50;
#ifdef RT_CAN_USING_HDR
    canfd_cfig.maxhdr = 1;
#endif

    for (i = 0; i < ARRAY_SIZE(aic_canfd_arr); i++)
    {
        hal_canfd_init(&aic_canfd_arr[i].canfdHandle);
        hal_canfd_attach_callback(&aic_canfd_arr[i].canfdHandle,
                                aic_canfd_callback, NULL);
        aicos_request_irq(aic_canfd_arr[i].canfdHandle.irq_num, hal_canfd_isr_handler,
                          0, NULL, (void *)&aic_canfd_arr[i].canfdHandle);

        aic_canfd_arr[i].can.config = canfd_cfig;
        ret = rt_hw_can_register(&aic_canfd_arr[i].can, aic_canfd_arr[i].name,
                                 &aic_canfd_ops, NULL);
#ifdef RT_USING_PM
        rt_pm_device_register(&aic_canfd_arr[i].can.parent, &aic_canfd_pm_ops);
#endif
    }

    return ret;
}
INIT_DEVICE_EXPORT(rt_hw_aic_canfd_init);
