/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao Li <siyao.li@artinchip.com>
 */

#include "hal_gpai.h"
#include "aic_dma_id.h"

struct aic_gpai_ch aic_gpai_chs[] = {
#ifdef AIC_USING_GPAI0
    {
        .id = 0,
        .available = 1,
        .adc_acq = AIC_GPAI0_ADC_ACQ,
#ifdef AIC_GPAI0_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI0,
#endif
        .obtain_data_mode = AIC_GPAI0_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI0_MODE,
#ifdef AIC_GPAI0_MODE_PERIOD
        .smp_period = AIC_GPAI0_PERIOD_TIME,
#endif
#ifdef AIC_GPAI_DRV_V11
        .fifo_depth = 8,
#else
        .fifo_depth = 32,
#endif
    },
#endif
#ifdef AIC_USING_GPAI1
    {
        .id = 1,
        .available = 1,
        .adc_acq = AIC_GPAI1_ADC_ACQ,
#ifdef AIC_GPAI1_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI1,
#endif
        .obtain_data_mode = AIC_GPAI1_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI1_MODE,
#ifdef AIC_GPAI1_MODE_PERIOD
        .smp_period = AIC_GPAI1_PERIOD_TIME,
#endif
#ifdef AIC_GPAI_DRV_V11
        .fifo_depth = 8,
#else
        .fifo_depth = 32,
#endif
    },
#endif
#ifdef AIC_USING_GPAI2
    {
        .id = 2,
        .available = 1,
        .adc_acq = AIC_GPAI2_ADC_ACQ,
#ifdef AIC_GPAI2_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI2,
#endif
        .obtain_data_mode = AIC_GPAI2_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI2_MODE,
#ifdef AIC_GPAI0_MODE_PERIOD
        .smp_period = AIC_GPAI2_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI3
    {
        .id = 3,
        .available = 1,
        .adc_acq = AIC_GPAI3_ADC_ACQ,
#ifdef AIC_GPAI3_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI3,
#endif
        .obtain_data_mode = AIC_GPAI3_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI3_MODE,
#ifdef AIC_GPAI3_MODE_PERIOD
        .smp_period = AIC_GPAI3_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI4
    {
        .id = 4,
        .available = 1,
        .adc_acq = AIC_GPAI4_ADC_ACQ,
#ifdef AIC_GPAI4_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI4,
#endif
        .obtain_data_mode = AIC_GPAI4_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI4_MODE,
#ifdef AIC_GPAI4_MODE_PERIOD
        .smp_period = AIC_GPAI4_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI5
    {
        .id = 5,
        .available = 1,
        .adc_acq = AIC_GPAI5_ADC_ACQ,
#ifdef AIC_GPAI5_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI5,
#endif
        .obtain_data_mode = AIC_GPAI5_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI5_MODE,
#ifdef AIC_GPAI5_MODE_PERIOD
        .smp_period = AIC_GPAI5_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI6
    {
        .id = 6,
        .available = 1,
        .adc_acq = AIC_GPAI6_ADC_ACQ,
#ifdef AIC_GPAI6_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI6,
#endif
        .obtain_data_mode = AIC_GPAI6_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI6_MODE,
#ifdef AIC_GPAI6_MODE_PERIOD
        .smp_period = AIC_GPAI6_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI7
    {
        .id = 7,
        .available = 1,
        .adc_acq = AIC_GPAI7_ADC_ACQ,
#ifdef AIC_GPAI7_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI7,
#endif
        .obtain_data_mode = AIC_GPAI7_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI7_MODE,
#ifdef AIC_GPAI7_MODE_PERIOD
        .smp_period = AIC_GPAI7_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI8
    {
        .id = 8,
        .available = 1,
        .adc_acq = AIC_GPAI8_ADC_ACQ,
#ifdef AIC_GPAI8_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI8,
#endif
        .obtain_data_mode = AIC_GPAI8_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI8_MODE,
#ifdef AIC_GPAI8_MODE_PERIOD
        .smp_period = AIC_GPAI8_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI9
    {
        .id = 9,
        .available = 1,
        .adc_acq = AIC_GPAI9_ADC_ACQ,
#ifdef AIC_GPAI9_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI9,
#endif
        .obtain_data_mode = AIC_GPAI9_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI9_MODE,
#ifdef AIC_GPAI9_MODE_PERIOD
        .smp_period = AIC_GPAI9_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI10
    {
        .id = 10,
        .available = 1,
        .adc_acq = AIC_GPAI10_ADC_ACQ,
#ifdef AIC_GPAI10_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI10,
#endif
        .obtain_data_mode = AIC_GPAI10_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI10_MODE,
#ifdef AIC_GPAI10_MODE_PERIOD
        .smp_period = AIC_GPAI10_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI11
    {
        .id = 11,
        .available = 1,
        .adc_acq = AIC_GPAI11_ADC_ACQ,
#ifdef AIC_GPAI11_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI11,
#endif
        .obtain_data_mode = AIC_GPAI11_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI11_MODE,
#ifdef AIC_GPAI11_MODE_PERIOD
        .smp_period = AIC_GPAI11_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI12
    {
        .id = 12,
        .available = 1,
        .adc_acq = AIC_GPAI12_ADC_ACQ,
#ifdef AIC_GPAI12_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI12,
#endif
        .obtain_data_mode = AIC_GPAI12_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI12_MODE,
#ifdef AIC_GPAI12_MODE_PERIOD
        .smp_period = AIC_GPAI12_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI13
    {
        .id = 13,
        .available = 1,
        .adc_acq = AIC_GPAI13_ADC_ACQ,
#ifdef AIC_GPAI13_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI13,
#endif
        .obtain_data_mode = AIC_GPAI13_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI13_MODE,
#ifdef AIC_GPAI13_MODE_PERIOD
        .smp_period = AIC_GPAI13_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI14
    {
        .id = 14,
        .available = 1,
        .adc_acq = AIC_GPAI14_ADC_ACQ,
#ifdef AIC_GPAI14_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI14,
#endif
        .obtain_data_mode = AIC_GPAI14_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI14_MODE,
#ifdef AIC_GPAI14_MODE_PERIOD
        .smp_period = AIC_GPAI14_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI15
    {
        .id = 15,
        .available = 1,
        .adc_acq = AIC_GPAI15_ADC_ACQ,
#ifdef AIC_GPAI15_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI15,
#endif
        .obtain_data_mode = AIC_GPAI15_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI15_MODE,
#ifdef AIC_GPAI15_MODE_PERIOD
        .smp_period = AIC_GPAI15_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI16
    {
        .id = 16,
        .available = 1,
        .adc_acq = AIC_GPAI16_ADC_ACQ,
#ifdef AIC_GPAI16_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI16,
#endif
        .obtain_data_mode = AIC_GPAI16_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI16_MODE,
#ifdef AIC_GPAI16_MODE_PERIOD
        .smp_period = AIC_GPAI16_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI17
    {
        .id = 17,
        .available = 1,
        .adc_acq = AIC_GPAI17_ADC_ACQ,
#ifdef AIC_GPAI17_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI17,
#endif
        .obtain_data_mode = AIC_GPAI17_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI17_MODE,
#ifdef AIC_GPAI17_MODE_PERIOD
        .smp_period = AIC_GPAI17_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI18
    {
        .id = 18,
        .available = 1,
        .adc_acq = AIC_GPAI18_ADC_ACQ,
#ifdef AIC_GPAI18_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI18,
#endif
        .obtain_data_mode = AIC_GPAI18_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI18_MODE,
#ifdef AIC_GPAI18_MODE_PERIOD
        .smp_period = AIC_GPAI18_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI19
    {
        .id = 19,
        .available = 1,
        .adc_acq = AIC_GPAI19_ADC_ACQ,
#ifdef AIC_GPAI19_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI19,
#endif
        .obtain_data_mode = AIC_GPAI19_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI19_MODE,
#ifdef AIC_GPAI19_MODE_PERIOD
        .smp_period = AIC_GPAI19_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI20
    {
        .id = 20,
        .available = 1,
        .adc_acq = AIC_GPAI20_ADC_ACQ,
#ifdef AIC_GPAI20_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI20,
#endif
        .obtain_data_mode = AIC_GPAI20_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI20_MODE,
#ifdef AIC_GPAI20_MODE_PERIOD
        .smp_period = AIC_GPAI20_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI21
    {
        .id = 21,
        .available = 1,
        .adc_acq = AIC_GPAI21_ADC_ACQ,
#ifdef AIC_GPAI21_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI21,
#endif
        .obtain_data_mode = AIC_GPAI21_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI21_MODE,
#ifdef AIC_GPAI21_MODE_PERIOD
        .smp_period = AIC_GPAI21_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI22
    {
        .id = 22,
        .available = 1,
        .adc_acq = AIC_GPAI22_ADC_ACQ,
#ifdef AIC_GPAI22_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI22,
#endif
        .obtain_data_mode = AIC_GPAI22_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI22_MODE,
#ifdef AIC_GPAI22_MODE_PERIOD
        .smp_period = AIC_GPAI22_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI23
    {
        .id = 23,
        .available = 1,
        .adc_acq = AIC_GPAI23_ADC_ACQ,
#ifdef AIC_GPAI23_OBTAIN_DATA_BY_DMA
        .dma_port_id = DMA_ID_GPAI23,
#endif
        .obtain_data_mode = AIC_GPAI23_OBTAIN_DATA_MODE,
        .mode = AIC_GPAI23_MODE,
#ifdef AIC_GPAI23_MODE_PERIOD
        .smp_period = AIC_GPAI23_PERIOD_TIME,
#endif
        .fifo_depth = 8,
    },
#endif
};

const int aic_gpai_chs_size = ARRAY_SIZE(aic_gpai_chs);
