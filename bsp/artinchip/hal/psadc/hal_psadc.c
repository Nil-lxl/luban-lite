/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include "aic_core.h"
#include "hal_psadc.h"
#include "hal_dma.h"
#include "aic_dma_id.h"
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
#define PSADC_MCR_Q1_TRIGA      BIT(20)
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

#define PSADC_FCR_DCNT_MASK         GENMASK(28, 24)
#define PSADC_FCR_DCNT_SHIFT        24
#define PSADC_FCR_DRTH_SHIFT        8
#define PSADC_FCR_UF_STS            BIT(17)
#define PSADC_FCR_OF_STS            BIT(16)
#define PSADC_FCR_FIFO_ERRIE        BIT(3)
#define PSADC_FCR_FIFO_DRQE         BIT(2)
#define PSADC_FCR_FIFO_RDYIE        BIT(1)
#define PSADC_FCR_FIFO_FLUSH        BIT(0)

#define PSADC_Q1FDR_CHNUM_SHIFT     12
#define PSADC_Q1FDR_CHNUM_MASK      GENMASK(15, 12)
#define PSADC_Q1FDR_DATA_MASK       GENMASK(11, 0)
#define PSADC_Q1FDR_DATA            BIT(0)

#define PSADC_Q2FDR_CHNUM_SHIFT     12
#define PSADC_Q2FDR_CHNUM_MASK      GENMASK(15, 12)
#define PSADC_Q2FDR_DATA_MASK       GENMASK(11, 0)
#define PSADC_Q2FDR_DATA            BIT(0)

extern struct aic_psadc_ch aic_psadc_chs[];
static u32 g_psadc_ch_num = 0; // the number of available channel
static u32 g_psadc_ch_data[AIC_PSADC_CH_NUM] = {0};

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

void hal_psadc_qc_irq_enable(bool enable)
{
#ifdef AIC_PSADC_TRIG_BY_EPWM
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_Q1_TRIGA, enable);
#endif
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_Q1_INTE, enable);
}

static void psadc_fifo1_flush(void)
{
    u32 val = psadc_readl(PSADC_Q1FCR);

    if (val & PSADC_FCR_UF_STS)
        pr_err("FIFO is Underflow! FCR: 0x%#x\n", val);
    if (val & PSADC_FCR_OF_STS)
        pr_err("FIFO is Overflow! FCR: 0x%#x\n", val);

    psadc_writel(val | PSADC_FCR_FIFO_FLUSH, PSADC_Q1FCR);
}

static void psadc_fifo_init(void)
{
    u32 val = PSADC_FCR_FIFO_ERRIE;

#if defined(AIC_PSADC_DRV_DMA)
    val |= PSADC_FCR_FIFO_DRQE;
#elif defined(AIC_PSADC_DRV_INT)
    val |= PSADC_FCR_FIFO_RDYIE;
#endif
    val |= g_psadc_ch_num << PSADC_FCR_DRTH_SHIFT;
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

#ifdef AIC_PSADC_DRV_DMA
int hal_psadc_active_dma(struct aic_psadc_dev *queue,
                         struct aic_psadc_dma_info *dma_info)
{
    int ret = 0;

    if (queue->dma_chan == NULL) {
        pr_err("Has not been config dma yet.");
        return -EINVAL;
    }

    hal_dma_chan_register_cb(queue->dma_chan, dma_info->callback,
                             (void *)queue);
    ret = hal_dma_chan_prep_device(queue->dma_chan, (ulong)dma_info->buf,
                                   PSADC_BASE + PSADC_Q1FDR,
                                   dma_info->buf_size,
                                   DMA_DEV_TO_MEM);
    if (ret) {
        pr_err("Failed to prepare dma.\n");
        return ret;
    }
#ifdef AIC_PSADC_TRIG_BY_SOFT
    hal_psadc_sw_trigger();
#endif
    return hal_dma_chan_start(queue->dma_chan);
}

static int psadc_request_dma(struct aic_psadc_dev *queue)
{
    struct dma_slave_config config = {0};
    u32 dma_id = DMA_ID_PSADC_Q1;

    if (queue->type == AIC_PSADC_Q2)
        dma_id = DMA_ID_PSADC_Q2;
    config.direction = DMA_DEV_TO_MEM;
    config.src_addr = PSADC_BASE + PSADC_Q1FDR;
    config.slave_id = dma_id;
    config.src_maxburst = 1;
    config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

    queue->dma_chan = hal_request_dma_chan();
    if (!queue->dma_chan) {
        pr_err("PSADC request dma channel error\n");
        return -ENODEV;
    }
    hal_dma_chan_config(queue->dma_chan, &config);
    return 0;
}

static int psadc_release_dma(struct aic_psadc_dev *queue)
{
    if (queue->dma_chan) {
        hal_dma_chan_stop(queue->dma_chan);
        hal_release_dma_chan(queue->dma_chan);
    }
    queue->dma_chan = NULL;
    return 0;
}
#endif

int hal_psadc_init(struct aic_psadc_dev *queue)
{
    int val = 0;

#ifdef AIC_PSADC_DRV_DMA
    psadc_request_dma(queue);
#endif
    psadc_fifo_init();
    if (queue->type == AIC_PSADC_QC) {
        val = queue->nodes_num - 1;
        psadc_writel(val, PSADC_TCR);
    }

    hal_psadc_qc_irq_enable(true);
    return 0;
}

void hal_psadc_deinit(struct aic_psadc_dev *queue)
{
    hal_psadc_qc_irq_enable(false);
#ifdef AIC_PSADC_DRV_DMA
    psadc_release_dma(queue);
#endif
}

void aich_psadc_status_show(void)
{
    int version = psadc_readl(PSADC_VERSION);

    printf("In PSADC V%s, trigger source: %s\n", EXPAND_BCD_VER(version),
#ifdef AIC_PSADC_TRIG_BY_EPWM
           "EPWM"
#else
           "software"
#endif
           );

    printf("Enabled %d channels: ", g_psadc_ch_num);
    for (int i = 0; i < g_psadc_ch_num; i++) {
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
    g_psadc_ch_data[cnt] = data;
    pr_debug("[%d]: ADC value %d\n", cnt, val);
}

struct aic_psadc_ch *hal_psadc_ch_is_valid(u32 ch)
{
    s32 i;
    if (ch >= AIC_PSADC_CH_NUM) {
        pr_err("Invalid channel %d\n", ch);
        return NULL;
    }

    for (i = 0; i < g_psadc_ch_num; i++) {
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

void hal_psadc_sw_trigger(void)
{
    psadc_fifo1_flush();
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_Q1_TRIGS, 1);
}

int hal_psadc_read(struct aic_psadc_dev *queue, u32 *val, u32 timeout)
{
    int ret = 0;

    if (queue->complete == NULL)
        return -EINVAL;

#ifdef AIC_PSADC_TRIG_BY_SOFT
    hal_psadc_sw_trigger();
#endif

    ret = aicos_sem_take(queue->complete, timeout);
    if (ret < 0) {
        hal_log_err("Queue%d read timeout!\n", queue->id);
        hal_psadc_qc_irq_enable(false);
        return -ETIMEDOUT;
    }

    if (val) {
        unsigned long state;

        aicos_local_irq_save(&state);
        memcpy(val, g_psadc_ch_data, sizeof(u32) * queue->nodes_num);
        aicos_local_irq_restore(state);
    }

    return 0;
}

int hal_psadc_read_poll(struct aic_psadc_dev *queue, u32 *val, u32 timeout)
{
    u32 q_flag = 0;
    int get_data_flag = 0;
    int time_count = 0;

#ifdef AIC_PSADC_TRIG_BY_SOFT
    hal_psadc_sw_trigger();
#endif

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
            hal_psadc_qc_irq_enable(false);
            return -ETIMEDOUT;
        }
    }

    if (val)
        memcpy(val, g_psadc_ch_data, sizeof(u32) * queue->nodes_num);

    return 0;
}

irqreturn_t hal_psadc_isr(int irq, void *arg)
{
    u32 q_flag = 0, fifo_cnt = 0;
    struct aic_psadc_dev *queue = (struct aic_psadc_dev *)arg;

    q_flag = psadc_readl(PSADC_MSR);
    if (q_flag & PSADC_MSR_Q1_INT) {
        fifo_cnt = (psadc_readl(PSADC_Q1FCR) & PSADC_FCR_DCNT_MASK) >> PSADC_FCR_DCNT_SHIFT;
        for (int j = 0; j < fifo_cnt / queue->nodes_num; j++)
            for (int i = 0; i < queue->nodes_num; i++)
                psadc_read_ch(i);

        if ((queue->nodes_num > 1) && (fifo_cnt % queue->nodes_num)) {
            pr_warn("The is data residue in FIFO: %d/%d\n",
                    fifo_cnt, queue->nodes_num);
            psadc_fifo1_flush();
        }

        if (queue->complete)
            aicos_sem_give(queue->complete);
    }

    if (q_flag & PSADC_MSR_Q1_FERR)
        psadc_fifo1_flush();

    psadc_writel(q_flag, PSADC_MSR);

    pr_debug("Q flag: 0x%x\n", q_flag);
    return IRQ_HANDLED;
}

void hal_psadc_set_ch_num(u32 num)
{
    if (num >= AIC_PSADC_CH_NUM) {
        pr_err("Channel number %d is out of range\n", num);
        return;
    }

    g_psadc_ch_num = num;
}
