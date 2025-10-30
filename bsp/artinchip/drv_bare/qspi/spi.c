/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiji.chen <jiji.chen@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sfud.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_soc.h>
#include <aic_log.h>
#include <aic_hal.h>
#include <hal_qspi.h>
#include <spinor_port.h>
#include <hal_dma.h>
#include <aic_dma_id.h>
#include <aic_clk_id.h>

static struct aic_qspi_bus qspi_bus_arr[] = {
#if defined(AIC_USING_QSPI0) && defined(AIC_QSPI0_DEVICE_SPINOR)
    {
        .name = "qspi0",
        .idx = 0,
        .clk_id = CLK_QSPI0,
        .clk_in_hz = AIC_DEV_QSPI0_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI0_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SPI0,
        .irq_num = QSPI0_IRQn,
        .dl_width = AIC_QSPI0_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI0_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_QSPI0_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dylmode = AIC_DEV_QSPI0_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_QSPI0_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI1) && defined(AIC_QSPI1_DEVICE_SPINOR)
    {
        .name = "qspi1",
        .idx = 1,
        .clk_id = CLK_QSPI1,
        .clk_in_hz = AIC_DEV_QSPI1_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI1_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SPI1,
        .irq_num = QSPI1_IRQn,
        .dl_width = AIC_QSPI1_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI1_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_QSPI1_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dylmode = AIC_DEV_QSPI1_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_QSPI1_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI2) && defined(AIC_QSPI2_DEVICE_SPINOR)
    {
        .name = "qspi2",
        .idx = 2,
        .clk_id = CLK_QSPI2,
        .clk_in_hz = AIC_DEV_QSPI2_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI2_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SPI2,
        .irq_num = QSPI2_IRQn,
        .dl_width = AIC_QSPI2_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI2_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_QSPI2_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dylmode = AIC_DEV_QSPI2_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_QSPI2_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI3)
    {
        .name = "qspi3",
        .idx = 3,
        .clk_id = CLK_QSPI3,
        .clk_in_hz = AIC_DEV_QSPI3_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI3_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SPI3,
        .irq_num = QSPI3_IRQn,
        .dl_width = AIC_QSPI3_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI3_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_QSPI3_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dylmode = AIC_DEV_QSPI3_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_QSPI3_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_SE_SPI) && defined(AIC_SE_SPI_DEVICE_SPINOR)
    {
        .name = "sespi",
        .idx = 5,
        .clk_id = CLK_SE_SPI,
        .clk_in_hz = AIC_DEV_SE_SPI_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_SE_SPI_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SE_SPI,
        .irq_num = SE_SPI_IRQn,
        .dl_width = AIC_SE_SPI_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_SE_SPI_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_SE_SPI_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dylmode = AIC_DEV_SE_SPI_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_SE_SPI_TX_CLK_DELAY_MODE,
#endif
    },
#endif
};

int spi_write_read(struct aic_qspi_bus *qspi,
                               const uint8_t *write_buf, size_t write_size,
                               uint8_t *read_buf, size_t read_size)
{
    struct qspi_transfer t;
    int ret = 0;
    u32 cs_num = 0;

    hal_qspi_master_set_bus_width(&qspi->handle, HAL_QSPI_BUS_WIDTH_SINGLE);
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
    cs_num = qspi->cs_num;
#endif
    hal_qspi_master_set_cs(&qspi->handle, cs_num, true);
    if (write_size) {
        t.rx_data = NULL;
        t.tx_data = (uint8_t *)write_buf;
        t.data_len = write_size;
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret < 0)
            goto out;
    }
    if (read_size) {
        t.rx_data = read_buf;
        t.tx_data = NULL;
        t.data_len = read_size;
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
    }
out:
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
    cs_num = qspi->cs_num;
#endif
    hal_qspi_master_set_cs(&qspi->handle, cs_num, false);
    return ret;
}

static struct aic_qspi_bus *get_qspi_by_index(u32 idx)
{
    struct aic_qspi_bus *qspi;
    u32 i;

    qspi = NULL;
    for (i = 0; i < ARRAY_SIZE(qspi_bus_arr); i++) {
        if (qspi_bus_arr[i].idx == idx) {
            qspi = &qspi_bus_arr[i];
            break;
        }
    }

    return qspi;
}

struct aic_qspi_bus *qspi_probe(u32 spi_bus)
{

    struct aic_qspi_bus *qspi;
    int ret;
    struct qspi_master_config cfg = {0};

    qspi = get_qspi_by_index(spi_bus);
    if (!qspi) {
        pr_err("spi bus is invalid: %d\n", spi_bus);
        return NULL;
    }

    if (qspi->probe_flag)
        return qspi;

    memset(&cfg, 0, sizeof(cfg));
    cfg.idx = qspi->idx;
    cfg.clk_in_hz = qspi->clk_in_hz;
    cfg.clk_id = qspi->clk_id;
    cfg.cpol = HAL_QSPI_CPOL_ACTIVE_HIGH;
    cfg.cpha = HAL_QSPI_CPHA_FIRST_EDGE;
    cfg.cs_polarity = HAL_QSPI_CS_POL_VALID_LOW;
    cfg.rx_dlymode = qspi->rxd_dylmode;
    cfg.tx_dlymode = aic_convert_tx_dlymode(qspi->txc_dylmode, qspi->txd_dylmode);

    ret = hal_qspi_master_init(&qspi->handle, &cfg);
    if (ret) {
        pr_err("hal_qspi_master_init failed. ret %d\n", ret);
        return NULL;
    }

#ifdef AIC_DMA_DRV
    struct qspi_master_dma_config dmacfg;
    memset(&dmacfg, 0, sizeof(dmacfg));
    dmacfg.port_id = qspi->dma_port_id;

    ret = hal_qspi_master_dma_config(&qspi->handle, &dmacfg);
    if (ret) {
        pr_err("qspi dma config failed.\n");
        return NULL;
    }

#endif

    qspi->probe_flag = true;
    return qspi;
}
