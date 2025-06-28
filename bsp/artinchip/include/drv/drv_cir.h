/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#ifndef _DRV_CIR_H_
#define _DRV_CIR_H_

#include "hal_cir.h"


typedef enum cir_protocol {
        CIR_PROTOCOL_NEC = 0U,
        CIR_PROTOCOL_RC5 = 1U,
} cir_protocol_t;

typedef struct cir_config {
        cir_protocol_t      protocol;
        uint32_t            tx_duty;
        uint32_t            rx_level;  /* Indicates the idle level of RX */
} cir_config_t;

typedef struct aic_cir {
        struct rt_device    dev;
        aic_cir_ctrl_t      aic_cir_ctrl;
        cir_config_t        config;
        struct rt_mutex     lock;
} aic_cir_t;

typedef enum cir_command {   //遥控器发送指令
        CIR_CMD_SHUTDOWN = 0xFF45,
        CIR_CMD_MENU = 0xFF47,
        CIR_CMD_TEST = 0xFF44,
        CIR_CMD_BACK = 0xFF43,
        CIR_CMD_PLUS = 0xFF40,
        CIR_CMD_MINUS = 0xFF19,
        CIR_CMD_NEXT = 0xFF09,
        CIR_CMD_PREV = 0xFF07,
        CIR_CMD_PLAY = 0xFF15,
        CIR_CMD_ZERO = 0xFF16,
        CIR_CMD_CLEAR = 0xFF0D,
        CIR_CMD_ONE = 0xFF0C,
        CIR_CMD_TWO = 0xFF18,
        CIR_CMD_THREE = 0xFF5E,
        CIR_CMD_FOUR = 0xFF08,
        CIR_CMD_FIVE = 0xFF1C,
        CIR_CMD_SIX = 0xFF5A,
        CIR_CMD_SEVEN = 0xFF42,
        CIR_CMD_EIGHT = 0xFF52,
        CIR_CMD_NINE = 0xFF4A,
}cir_command_t;

#define IOC_CIR_CONFIGURE		1

#define CIR_RX_DONE 			1
#define CIR_RX_ERROR			0

#endif

