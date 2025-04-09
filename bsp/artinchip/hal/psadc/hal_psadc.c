/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include "aic_core.h"
#include "hal_psadc.h"
#include <string.h>

/* Register definition of PSADC Controller */
#define PSADC_MCR               0x000
#define PSADC_TCR               0x004
#define PSADC_NODE1             0x008
#define PSADC_NODE2             0x00C
#define PSADC_MSR               0x010
#define PSADC_CALCSR            0x014
#define PSADC_FILTER            0x01C
#define PSADC_Q1FCR             0x020
#define PSADC_Q2FCR             0x024
#define PSADC_Q1FDR             0x040
#define PSADC_Q2FDR             0x080
#define PSADC_VERSION           0xFFC

#define PSADC_MCR_Q1_TRIGS      BIT(22)
#define PSADC_MCR_Q1_INTE       BIT(18)
#define PSADC_MCR_QUE_COMB      BIT(1)
#define PSADC_MCR_EN            BIT(0)

#define PSADC_TCR_Q2_TRIG_CNT_SHIFT 4
#define PSADC_QX_NODE_MASK          4

#define PSADC_MSR_QX_CNT_MASK       GENMASK(27, 24)
#define PSADC_MSR_Q2_FERR           BIT(6)
#define PSADC_MSR_Q2_INT            BIT(4)
#define PSADC_MSR_Q1_FERR           BIT(2)
#define PSADC_MSR_Q1_INT            BIT(0)

#define PSADC_Q1FCR_FIFO_DRTH_SHIFT 11
#define PSADC_Q1FCR_UF_STS          BIT(17)
#define PSADC_Q1FCR_OF_STS          BIT(16)
#define PSADC_Q1FCR_FIFO_ERRIE      BIT(3)
#define PSADC_Q1FCR_FIFO_FLUSH      BIT(0)

#define PSADC_Q2FCR_FIFO_DRTH_SHIFT 11
#define PSADC_Q2FCR_UF_STS          BIT(17)
#define PSADC_Q2FCR_OF_STS          BIT(16)
#define PSADC_Q2FCR_FIFO_ERRIE      BIT(3)
#define PSADC_Q2FCR_FIFO_FLUSH      BIT(0)

#define PSADC_Q1FDR_CHNUM_SHIFT     12
#define PSADC_Q1FDR_CHNUM_MASK      GENMASK(15, 12)
#define PSADC_Q1FDR_DATA_MASK       GENMASK(11, 0)
#define PSADC_Q1FDR_DATA            BIT(0)

#define PSADC_Q2FDR_CHNUM_SHIFT     12
#define PSADC_Q2FDR_CHNUM_MASK      GENMASK(15, 12)
#define PSADC_Q2FDR_DATA_MASK       GENMASK(11, 0)
#define PSADC_Q2FDR_DATA            BIT(0)

extern struct aic_psadc_ch aic_psadc_chs[];
extern struct aic_psadc_queue aic_psadc_queues[];
static u32 aic_psadc_ch_num = 0; // the number of available channel
static u32 aic_psadc_ch_data[AIC_PSADC_CH_NUM];

static inline void psadc_writel(u32 val, int reg)
{
    writel(val, PSADC_BASE + reg);
}

static inline u32 psadc_readl(int reg)
{
    return readl(PSADC_BASE + reg);
}

static void psadc_reg_enable(int offset, int bit, int enable)
{
    int tmp = psadc_readl(offset);

    if (enable)
        tmp |= bit;
    else
        tmp &= ~bit;

    psadc_writel(tmp, offset);
}

void hal_psadc_enable(int enable)
{
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_EN, enable);
}

void hal_psadc_single_queue_mode(int enable)
{
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_QUE_COMB, enable);
}

void hal_psadc_qc_irq_enable(int enable)
{
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_Q1_INTE, enable);
}

static void psadc_fifo1_flush()
{
    u32 val = psadc_readl(PSADC_Q1FCR);

    if (val & PSADC_Q1FCR_UF_STS)
        pr_err("FIFO is Underflow!%#x\n", val);
    if (val & PSADC_Q1FCR_OF_STS)
        pr_err("FIFO is Overflow!%#x\n", val);

    psadc_writel(val | PSADC_Q1FCR_FIFO_FLUSH, PSADC_Q1FCR);
}

static void psadc_fifo_init(void)
{
    u32 val = 0;

    val = 1 << PSADC_Q1FCR_FIFO_DRTH_SHIFT;
    psadc_writel(val, PSADC_Q1FCR);
    psadc_writel(val, PSADC_Q2FCR);
}

int hal_psadc_set_queue_node(int queue, int ch, int node_ordinal)
{
    int val;
    int node_chan = 0;
    u32 node_regs = 0;

    if (queue == AIC_PSADC_Q1)
        node_regs = PSADC_NODE1;
    if (queue == AIC_PSADC_Q2)
        node_regs = PSADC_NODE2;

    node_chan = ch << (node_ordinal * PSADC_QX_NODE_MASK);
    val = psadc_readl(node_regs);
    val |= node_chan;
    psadc_writel(val, node_regs);

    return 0;
}

int  hal_psadc_ch_init()
{
    int val = 0;
    struct aic_psadc_queue *queue  = &aic_psadc_queues[0];

    psadc_fifo_init();
    if (queue->type == AIC_PSADC_QC) {
        val = queue->nodes_num;
        psadc_writel(val, PSADC_TCR);
    }

    psadc_reg_enable(PSADC_MCR, PSADC_MCR_Q1_INTE, 1);

    return 0;
}

void aich_psadc_status_show(void)
{
    int version = psadc_readl(PSADC_VERSION);

    printf("In PSADC V%d.%02d\n"
           "enabled %d channels: ",
           version >> 8, version & 0xff,
           aic_psadc_ch_num);

    for (int i = 0; i < aic_psadc_ch_num; i++) {
        if (aic_psadc_chs[i].available)
            printf("[%d] ", aic_psadc_chs[i].id);
    }
    printf("\n");
    return;
}

static void psadc_read_ch(int cnt)
{
    u32 data;
    int val;

    val = psadc_readl(PSADC_Q1FDR);
    data = val & PSADC_Q1FDR_DATA_MASK;
    aic_psadc_ch_data[cnt] = data;
    pr_debug("val %#x\n", val);
}

struct aic_psadc_ch *hal_psadc_ch_is_valid(u32 ch)
{
    s32 i;
    if (ch >= AIC_PSADC_CH_NUM) {
        pr_err("Invalid channel %d\n", ch);
        return NULL;
    }

    for (i = 0; i < aic_psadc_ch_num; i++) {
        if (aic_psadc_chs[i].id != ch)
            continue;

        if (aic_psadc_chs[i].available)
            return &aic_psadc_chs[i];
        else
            break;
    }
    pr_debug("Ch%d is unavailable!\n", ch);
    return NULL;
}

int hal_psadc_read(u32 *val, u32 timeout)
{
    int ret = 0;
    struct aic_psadc_queue *queue  = &aic_psadc_queues[AIC_PSADC_QC];

    psadc_fifo1_flush();
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_Q1_TRIGS, 1);
    ret = aicos_sem_take(queue->complete, timeout);
    if (ret < 0) {
        hal_log_err("Queue%d read timeout!\n", queue->id);
        hal_psadc_qc_irq_enable(0);

        return -ETIMEDOUT;
    }
    if (val)
        memcpy(val, aic_psadc_ch_data, sizeof(aic_psadc_ch_data));

    return 0;
}

int hal_psadc_read_poll(u32 *val, u32 timeout)
{
    u32 q_flag = 0;
    int get_data_flag = 0;
    int time_count = 0;
    struct aic_psadc_queue *queue  = &aic_psadc_queues[AIC_PSADC_QC];

    psadc_fifo1_flush();
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_Q1_TRIGS, 1);

    while (1) {
        time_count++;
        q_flag = psadc_readl(PSADC_MSR);
        psadc_writel(q_flag, PSADC_MSR);

        if (q_flag & PSADC_MSR_Q1_INT) {
            get_data_flag++;
            for(int i = 0; i < queue->nodes_num; i++)
                psadc_read_ch(i);
        }

        if (q_flag & PSADC_MSR_Q1_FERR)
            psadc_fifo1_flush();
        if (get_data_flag)
            break;
        if (time_count > timeout) {
            hal_psadc_qc_irq_enable(0);
            return -ETIMEDOUT;
        }
    }

    if (val)
        memcpy(val, aic_psadc_ch_data,
               sizeof(aic_psadc_ch_data[0]) * queue->nodes_num);

    return 0;
}

irqreturn_t hal_psadc_isr(int irq, void *arg)
{
    u32 q_flag = 0;
    struct aic_psadc_queue *queue  = &aic_psadc_queues[AIC_PSADC_QC];

    q_flag = psadc_readl(PSADC_MSR);
    psadc_writel(q_flag, PSADC_MSR);
    if (q_flag & PSADC_MSR_Q1_INT) {
        for(int i = 0; i < queue->nodes_num; i++)
            psadc_read_ch(i);
        aicos_sem_give(queue->complete);
    }

    if (q_flag & PSADC_MSR_Q1_FERR)
        psadc_fifo1_flush();

    return IRQ_HANDLED;
}

void hal_psadc_set_ch_num(u32 num)
{
    aic_psadc_ch_num = num;
}
