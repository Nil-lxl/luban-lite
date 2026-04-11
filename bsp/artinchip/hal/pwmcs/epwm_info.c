/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include "aic_core.h"
#include "aic_hal_clk.h"
#include "hal_epwm.h"

struct aic_epwm_arg epwm_pdata[] = {
#ifdef AIC_USING_EPWM0
    {
        .sync_mode = AIC_EPWM0_SYNC,
        .id = 0,
        .mode = EPWM_MODE_UP_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_HIGH},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 0,
    },
#endif
#ifdef AIC_USING_EPWM1
        {
        .sync_mode = AIC_EPWM1_SYNC,
        .id = 1,
        .mode = EPWM_MODE_UP_DOWN_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_HIGH,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_NONE},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_HIGH, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_NONE},
        .def_level = 0,
    },
#endif
#ifdef AIC_USING_EPWM2
    {
        .sync_mode = AIC_EPWM2_SYNC,
        .id = 2,
        .mode = EPWM_MODE_DOWN_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_HIGH,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 1,
    },
#endif
#ifdef AIC_USING_EPWM3
    {
        .sync_mode = AIC_EPWM3_SYNC,
        .id = 3,
        .mode = EPWM_MODE_UP_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_HIGH},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 0,
    },
#endif
#ifdef AIC_USING_EPWM4
    {
        .sync_mode = AIC_EPWM4_SYNC,
        .id = 4,
        .mode = EPWM_MODE_UP_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_HIGH},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 0,
    },
#endif
#ifdef AIC_USING_EPWM5
    {
        .sync_mode = AIC_EPWM5_SYNC,
        .id = 5,
        .mode = EPWM_MODE_UP_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_HIGH},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 0,
    },
#endif
#ifdef AIC_USING_EPWM6
    {
        .sync_mode = AIC_EPWM6_SYNC,
        .id = 6,
        .mode = EPWM_MODE_UP_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_HIGH},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 0,
    },
#endif
#ifdef AIC_USING_EPWM7
    {
        .sync_mode = AIC_EPWM7_SYNC,
        .id = 7,
        .mode = EPWM_MODE_UP_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_HIGH},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 0,
    },
#endif
#ifdef AIC_USING_EPWM8
    {
        .sync_mode = AIC_EPWM8_SYNC,
        .id = 8,
        .mode = EPWM_MODE_UP_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_HIGH},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 0,
    },
#endif
#ifdef AIC_USING_EPWM9
    {
        .sync_mode = AIC_EPWM9_SYNC,
        .id = 9,
        .mode = EPWM_MODE_UP_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_HIGH},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 0,
    },
#endif
#ifdef AIC_USING_EPWM10
    {
        .sync_mode = AIC_EPWM10_SYNC,
        .id = 10,
        .mode = EPWM_MODE_UP_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_HIGH},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 0,
    },
#endif
#ifdef AIC_USING_EPWM11
    {
        .sync_mode = AIC_EPWM11_SYNC,
        .id = 11,
        .mode = EPWM_MODE_UP_COUNT,
        .action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_NONE, EPWM_ACT_HIGH},
        .action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_LOW, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_NONE, EPWM_ACT_NONE,  EPWM_ACT_HIGH},
        .def_level = 0,
    },
#endif
};

const int epwm_pdata_size = ARRAY_SIZE(epwm_pdata);


