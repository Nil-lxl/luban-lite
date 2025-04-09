/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji Chen <jiji.chen@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <aic_hal.h>
#include <hal_qspi.h>
#include <hal_dma.h>
#include <aic_dma_id.h>
#include <aic_clk_id.h>

#define ASYNC_DATA_SIZE 64

struct aic_qspi {
    struct rt_spi_bus dev;
    char *name;
    uint32_t idx;
    uint32_t clk_id;
    uint32_t clk_in_hz;
    uint32_t dma_port_id;
    uint32_t irq_num;
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
    uint32_t cs_num;
#endif
    qspi_master_handle handle;
    struct rt_qspi_configuration configuration;
    rt_sem_t xfer_sem;
    bool inited;
    uint32_t rxd_dlymode;
    uint32_t txd_dlymode;
    uint32_t txc_dlymode;
    bool nonblock;
};

static struct aic_qspi spi_controller[] = {
#if defined(AIC_USING_QSPI1) && defined(AIC_QSPI1_BUS_SPI)
    {
        .name = "spi1",
        .idx = 1,
        .clk_id = CLK_QSPI1,
        .clk_in_hz = AIC_DEV_QSPI1_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI1,
        .irq_num = QSPI1_IRQn,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI1_CS_NUM,
#endif
        .rxd_dlymode = AIC_DEV_QSPI1_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dlymode = AIC_DEV_QSPI1_TXD_DELAY_MODE,
        .txc_dlymode = AIC_DEV_QSPI1_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI2) && defined(AIC_QSPI2_BUS_SPI)
    {
        .name = "spi2",
        .idx = 2,
        .clk_id = CLK_QSPI2,
        .clk_in_hz = AIC_DEV_QSPI2_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI2,
        .irq_num = QSPI2_IRQn,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI2_CS_NUM,
#endif
        .rxd_dlymode = AIC_DEV_QSPI2_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dlymode = AIC_DEV_QSPI2_TXD_DELAY_MODE,
        .txc_dlymode = AIC_DEV_QSPI2_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI3) && defined(AIC_QSPI3_BUS_SPI)
    {
        .name = "spi3",
        .idx = 3,
        .clk_id = CLK_QSPI3,
        .clk_in_hz = AIC_DEV_QSPI3_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI3,
        .irq_num = QSPI3_IRQn,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI3_CS_NUM,
#endif
        .rxd_dlymode = AIC_DEV_QSPI3_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dlymode = AIC_DEV_QSPI3_TXD_DELAY_MODE,
        .txc_dlymode = AIC_DEV_QSPI3_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI4) && defined(AIC_QSPI4_BUS_SPI)
    {
        .name = "spi4",
        .idx = 4,
        .clk_id = CLK_QSPI4,
        .clk_in_hz = AIC_DEV_QSPI4_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI4,
        .irq_num = QSPI4_IRQn,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI4_CS_NUM,
#endif
        .rxd_dlymode = AIC_DEV_QSPI4_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dlymode = AIC_DEV_QSPI4_TXD_DELAY_MODE,
        .txc_dlymode = AIC_DEV_QSPI4_TX_CLK_DELAY_MODE,
#endif
    },
#endif
};

static const u32 bit_mode_table[] = {
#ifdef AIC_QSPI1_BIT_MODE
    1,
#endif
#ifdef AIC_QSPI2_BIT_MODE
    2,
#endif
#ifdef AIC_QSPI3_BIT_MODE
    3,
#endif
};

static bool spi_support_bit_mode(struct aic_qspi *qspi)
{
    u32 i;
    for (i = 0; i < ARRAY_SIZE(bit_mode_table); i++) {
        if (bit_mode_table[i] == qspi->idx)
            return true;
    }
    return false;
}

#ifdef AIC_QSPI_DRV_DEBUG
void dump_cmd(uint8_t *data, uint32_t len)
{
    uint32_t i;

    printf("CMD: (%lu) ", (unsigned long)len);
    for (i = 0; i < len; i++)
        printf("%02X ", data[i]);
    printf("\n");
}
#endif

static rt_uint32_t drv_spi_send(struct aic_qspi *qspi,
                                 struct rt_spi_message *message,
                                 const uint8_t *tx, uint32_t size)
{
    struct qspi_transfer t;
    u32 ret = 0;

    t.rx_data = NULL;
    t.tx_data = (uint8_t *)tx;
    t.data_len = size;
    ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
    return ret;
}

static rt_uint32_t drv_spi_receive(struct aic_qspi *qspi,
                                 struct rt_spi_message *message,
                                 const uint8_t *tx, uint32_t size)
{
    struct qspi_transfer t;
    u32 ret = 0;

    t.tx_data = NULL;
    t.rx_data = (uint8_t *)tx;
    t.data_len = size;
    ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
    return ret;
}

static rt_uint32_t drv_spi_transfer(struct aic_qspi *qspi,
                             struct rt_spi_message *message)
{
    u32 ret = 0;
    struct qspi_transfer t;

    t.tx_data = (u8 *)message->send_buf;
    t.rx_data = message->recv_buf;
    t.data_len = message->length;
    ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
    return ret;
}

static void spi_set_cs_before(struct rt_spi_device *device,
                             struct rt_spi_message *message)
{
    struct rt_qspi_configuration *cfg;
    struct aic_qspi *qspi;
    u32 cs_num = 0;

    qspi = (struct aic_qspi *)device->bus;
    cfg = &qspi->configuration;

    if (message->cs_take && !(cfg->parent.mode & RT_SPI_NO_CS)) {
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        cs_num = qspi->cs_num;
#endif
        hal_qspi_master_set_cs(&qspi->handle, cs_num, true);
    }
}

static void spi_set_cs_after(struct rt_spi_device *device,
                             struct rt_spi_message *message)
{
    struct rt_qspi_configuration *cfg;
    struct aic_qspi *qspi;
    u32 cs_num = 0;

    qspi = (struct aic_qspi *)device->bus;
    cfg = &qspi->configuration;

    if (message->cs_release && !(cfg->parent.mode & RT_SPI_NO_CS)) {
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        cs_num = qspi->cs_num;
#endif
        if (!qspi->nonblock)
            hal_qspi_master_set_cs(&qspi->handle, cs_num, false);
    }
}

static rt_uint32_t spi_xfer(struct rt_spi_device *device,
                             struct rt_spi_message *message)
{
    struct aic_qspi *qspi;
    rt_uint32_t ret = 0;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    qspi = (struct aic_qspi *)device->bus;

    spi_set_cs_before(device, message);

    if (message->recv_buf && message->send_buf)
        ret = drv_spi_transfer(qspi, message);
    else if (message->recv_buf)
        ret = drv_spi_receive(qspi, message, message->recv_buf, message->length);
    else if (message->send_buf)
        ret = drv_spi_send(qspi, message, message->send_buf, message->length);

    spi_set_cs_after(device, message);

    if (ret)
        return ret;
    return message->length;
}

static void qspi_master_async_callback(qspi_master_handle *h, void *priv)
{
    struct aic_qspi *qspi = priv;
    u32 cs_num = 0;

#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        cs_num = qspi->cs_num;
#endif
    rt_sem_release(qspi->xfer_sem);
    hal_qspi_master_set_cs(&qspi->handle, cs_num, false);
}

static irqreturn_t qspi_irq_handler(int irq_num, void *arg)
{
    qspi_master_handle *h = arg;

    rt_interrupt_enter();
    hal_qspi_master_irq_handler(h);
    rt_interrupt_leave();

    return IRQ_HANDLED;
}

static rt_err_t spi_configure(struct rt_spi_device *device,
                               struct rt_spi_configuration *configuration)
{
    struct qspi_master_config cfg;
    struct aic_qspi *qspi;
    rt_uint32_t bus_hz;
    rt_err_t ret = RT_EOK;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(configuration != RT_NULL);

    qspi = (struct aic_qspi *)device->bus;

    if (spi_support_bit_mode(qspi))
        qspi->handle.bit_mode = true;
    else
        qspi->handle.bit_mode = false;

    if ((rt_memcmp(&qspi->configuration, configuration,
                   sizeof(struct rt_spi_configuration)) != 0) ||
        (qspi->handle.bit_mode == true)) {
        rt_memcpy(&qspi->configuration, configuration,
                  sizeof(struct rt_spi_configuration));
        rt_memset(&cfg, 0, sizeof(cfg));
        cfg.idx = qspi->idx;
        cfg.clk_id = qspi->clk_id;

        bus_hz = configuration->max_hz;
        if (bus_hz < HAL_QSPI_MIN_FREQ_HZ)
            bus_hz = HAL_QSPI_MIN_FREQ_HZ;
        if (bus_hz > HAL_QSPI_MAX_FREQ_HZ)
            bus_hz = HAL_QSPI_MAX_FREQ_HZ;
        cfg.clk_in_hz = qspi->clk_in_hz;
        if (cfg.clk_in_hz > HAL_QSPI_MAX_FREQ_HZ)
            cfg.clk_in_hz = HAL_QSPI_MAX_FREQ_HZ;
        if (qspi->clk_in_hz % bus_hz)
            cfg.clk_in_hz = bus_hz;
        if (cfg.clk_in_hz < HAL_QSPI_INPUT_MIN_FREQ_HZ)
            cfg.clk_in_hz = HAL_QSPI_INPUT_MIN_FREQ_HZ;

        if (configuration->mode & RT_SPI_MSB)
            cfg.lsb_en = false;
        else
            cfg.lsb_en = true;
        if (configuration->mode & RT_SPI_CPHA)
            cfg.cpha = HAL_QSPI_CPHA_SECOND_EDGE;
        else
            cfg.cpha = HAL_QSPI_CPHA_FIRST_EDGE;

        if (configuration->mode & RT_SPI_CPOL)
            cfg.cpol = HAL_QSPI_CPOL_ACTIVE_LOW;
        else
            cfg.cpol = HAL_QSPI_CPOL_ACTIVE_HIGH;
        if (configuration->mode & RT_SPI_CS_HIGH)
            cfg.cs_polarity = HAL_QSPI_CS_POL_VALID_HIGH;
        else
            cfg.cs_polarity = HAL_QSPI_CS_POL_VALID_LOW;
        cfg.rx_dlymode = qspi->rxd_dlymode;
        cfg.tx_dlymode = aic_convert_tx_dlymode(qspi->txc_dlymode, qspi->txd_dlymode);

        ret = hal_qspi_master_init(&qspi->handle, &cfg);
        if (ret) {
            pr_err("spi init failed.\n");
            return ret;
        }

        if (!qspi->inited) {
#ifdef AIC_DMA_DRV
            struct qspi_master_dma_config dmacfg;
            rt_memset(&dmacfg, 0, sizeof(dmacfg));
            dmacfg.port_id = qspi->dma_port_id;

            ret = hal_qspi_master_dma_config(&qspi->handle, &dmacfg);
            if (ret) {
                pr_err("qspi dma config failed.\n");
                return ret;
            }
#endif
            ret = hal_qspi_master_register_cb(&qspi->handle,
                                              qspi_master_async_callback, qspi);
            if (ret) {
                pr_err("qspi register async callback failed.\n");
                return ret;
            }
            aicos_request_irq(qspi->irq_num, qspi_irq_handler, 0, NULL,
                              (void *)&qspi->handle);
            aicos_irq_enable(qspi->irq_num);
        }
        bus_hz = configuration->max_hz;
        if (bus_hz > HAL_QSPI_MAX_FREQ_HZ)
            bus_hz = HAL_QSPI_MAX_FREQ_HZ;
        hal_qspi_master_set_bus_freq(&qspi->handle, bus_hz);
        if (ret) {
            pr_err("qspi set bus frequency failed.\n");
            return ret;
        }
    }
    qspi->inited = true;
    return ret;
}

static rt_err_t spi_nonblock_set(struct rt_spi_device *device,
                               rt_uint32_t nonblock)
{
    struct aic_qspi *qspi;

    qspi = (struct aic_qspi *)device->bus;
    if (nonblock == 1)
        qspi->nonblock = true;
    else
        qspi->nonblock = false;

    return RT_EOK;
}

static rt_uint32_t spi_get_transfer_status(struct rt_spi_device *device)
{
    struct aic_qspi *qspi;

    qspi = (struct aic_qspi *)device->bus;

    return hal_qspi_master_get_status(&qspi->handle);
}

rt_err_t aic_spi_bus_attach_device(const char *bus_name,
                                    const char *device_name)
{
    struct rt_spi_device *spi_device = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(device_name != RT_NULL);

    spi_device =
        (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));

    result = rt_spi_bus_attach_device(spi_device, device_name,
                                      bus_name, RT_NULL);

    if (result != RT_EOK && spi_device != NULL)
        rt_free(spi_device);

    return result;
}

static const struct rt_spi_ops aic_spi_ops = {
    .configure = spi_configure,
    .xfer = spi_xfer,
    .nonblock = spi_nonblock_set,
    .gstatus = spi_get_transfer_status,
};

static int rt_hw_spi_bus_init(void)
{
    char sem_name[RT_NAME_MAX];
    rt_err_t ret = RT_EOK;
    int i;

    for (i = 0; i < ARRAY_SIZE(spi_controller); i++) {
        ret = rt_spi_bus_register(&spi_controller[i].dev,
                                   spi_controller[i].name, &aic_spi_ops);
        if (ret != RT_EOK)
            break;
        rt_sprintf(sem_name, "%s_s", spi_controller[i].name);
        spi_controller[i].xfer_sem =
            rt_sem_create(sem_name, 0, RT_IPC_FLAG_PRIO);
        if (!spi_controller[i].xfer_sem) {
            ret = RT_ENOMEM;
            break;
        }
    }

    return ret;
}

INIT_BOARD_EXPORT(rt_hw_spi_bus_init);
