/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: weilin.peng@artinchip.com
 */

#include <aic_core.h>
#include <aic_hal.h>
#include "board.h"
#include <aic_utils.h>

struct aic_pinmux aic_pinmux_config[] = {
#ifdef AIC_PRGB_16BIT_HD
    {2, PIN_PULL_DIS, 3, "PD.0"},//R
    {2, PIN_PULL_DIS, 3, "PD.1"},
    {2, PIN_PULL_DIS, 3, "PD.2"},
    {2, PIN_PULL_DIS, 3, "PD.3"},
    {2, PIN_PULL_DIS, 3, "PD.4"},

    {2, PIN_PULL_DIS, 3, "PD.5"},//G
    {2, PIN_PULL_DIS, 3, "PD.6"},
    {2, PIN_PULL_DIS, 3, "PD.7"},
    {2, PIN_PULL_DIS, 3, "PD.8"},
    {2, PIN_PULL_DIS, 3, "PD.9"},
    {2, PIN_PULL_DIS, 3, "PD.10"},

    {2, PIN_PULL_DIS, 3, "PD.11"},//B
    {2, PIN_PULL_DIS, 3, "PD.12"},
    {2, PIN_PULL_DIS, 3, "PD.13"},
    {2, PIN_PULL_DIS, 3, "PD.14"},
    {2, PIN_PULL_DIS, 3, "PD.15"},

    // {2, PIN_PULL_DIS, 3, "PD.16"},
    // {2, PIN_PULL_DIS, 3, "PD.17"},
    // {2, PIN_PULL_DIS, 3, "PD.18"},
    // {2, PIN_PULL_DIS, 3, "PD.19"},
    // {2, PIN_PULL_DIS, 3, "PD.20"},
    // {2, PIN_PULL_DIS, 3, "PD.21"},
    // {2, PIN_PULL_DIS, 3, "PD.22"},
    // {2, PIN_PULL_DIS, 3, "PD.23"},

    {2, PIN_PULL_DIS, 3, "PD.24"}, //pclk
    {2, PIN_PULL_DIS, 3, "PD.25"}, //hsd
    {2, PIN_PULL_DIS, 3, "PD.26"}, //vsd
    {2, PIN_PULL_DIS, 3, "PD.27"}, //de
#endif  
#ifdef AIC_USING_UART0
    /* uart0 */
    {5, PIN_PULL_DIS, 3, "PA.0"},
    {5, PIN_PULL_UP, 3, "PA.1"},
#ifdef AIC_DEV_UART0_MODE_RS485_SIMULATION
    {1, PIN_PULL_DIS, 3, AIC_UART0_RTS_NAME},
#endif
#ifdef AIC_DEV_UART0_MODE_RS232_UNAUTO_FLOW_CTRL
    #ifdef AIC_UART0_RTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART0_RTS_NAME},   // BT_UART2_RTS
    #endif
    #ifdef AIC_UART0_CTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART0_CTS_NAME},   // BT_UART2_CTS
    #endif
#elif defined AIC_DEV_UART0_MODE_RS232_SW_HW_FLOW_CTRL
    #ifdef AIC_UART0_RTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART0_RTS_NAME},   // BT_UART2_RTS
    #endif
    #ifdef AIC_UART0_CTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART0_CTS_NAME},   // BT_UART2_CTS
    #endif
#endif

#endif
#ifdef AIC_USING_UART1
    /* uart1 */
    {5, PIN_PULL_DIS, 3, "PD.2"},
    {5, PIN_PULL_UP, 3, "PD.3"},
#ifdef AIC_DEV_UART1_MODE_RS485_SIMULATION
    {1, PIN_PULL_DIS, 3, AIC_UART1_RTS_NAME},
#endif
#ifdef AIC_DEV_UART1_MODE_RS232_UNAUTO_FLOW_CTRL
    #ifdef AIC_UART1_RTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART1_RTS_NAME},   // BT_UART2_RTS
    #endif
    #ifdef AIC_UART1_CTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART1_CTS_NAME},   // BT_UART2_CTS
    #endif
#endif
#ifdef AIC_DEV_UART1_MODE_RS232_SW_HW_FLOW_CTRL
    #ifdef AIC_UART1_RTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART1_RTS_NAME},   // BT_UART2_RTS
    #endif
    #ifdef AIC_UART1_CTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART1_CTS_NAME},   // BT_UART2_CTS
    #endif
#endif

#endif
#ifdef AIC_USING_UART2
    /* uart2 */
    {5, PIN_PULL_DIS, 3, "PD.4"},   // BT_UART2_TX
    {5, PIN_PULL_UP, 3, "PD.5"},   // BT_UART2_RX
#ifdef AIC_DEV_UART2_MODE_RS485_SIMULATION
    {1, PIN_PULL_DIS, 3, AIC_UART2_RTS_NAME},
#elif defined AIC_DEV_UART2_MODE_RS232_UNAUTO_FLOW_CTRL
    #ifdef AIC_UART2_RTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART2_RTS_NAME},   // BT_UART2_RTS
    #endif
    #ifdef AIC_UART2_CTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART2_CTS_NAME},   // BT_UART2_CTS
    #endif
#elif defined AIC_DEV_UART2_MODE_RS232_SW_HW_FLOW_CTRL
    #ifdef AIC_UART2_RTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART2_RTS_NAME},   // BT_UART2_RTS
    #endif
    #ifdef AIC_UART2_CTS_ENABLE
        {1, PIN_PULL_DIS, 3, AIC_UART2_CTS_NAME},   // BT_UART2_CTS
    #endif
#else
   {8, PIN_PULL_DIS, 3, "PA.2"},   // BT_UART2_CTS
   {8, PIN_PULL_DIS, 3, "PA.3"},   // BT_UART2_RTS
   {1, PIN_PULL_DIS, 3, "PD.6"},   // BT_PWR_ON

#endif
#endif
#ifdef AIC_USING_CAN0
    /* can0 */
    {4, PIN_PULL_DIS, 3, "PA.4"},
    {4, PIN_PULL_DIS, 3, "PA.5"},
#endif
#ifdef AIC_USING_AUDIO
#ifdef AIC_AUDIO_DMIC
    {4, PIN_PULL_DIS, 3, "PD.16"},
    {4, PIN_PULL_DIS, 3, "PD.17"},
#endif
#ifdef AIC_AUDIO_PLAYBACK
    {5, PIN_PULL_DIS, 3, "PE.12"},
    {1, PIN_PULL_DIS, 3, AIC_AUDIO_PA_ENABLE_GPIO},
#endif
#endif
#ifdef AIC_USING_I2S0
    {4, PIN_PULL_DIS, 3, "PD.11"},
    {4, PIN_PULL_DIS, 3, "PD.12"},
    {4, PIN_PULL_DIS, 3, "PD.13"},
    {4, PIN_PULL_DIS, 3, "PD.14"},
    {4, PIN_PULL_DIS, 3, "PD.15"},
#endif
#ifdef AIC_USING_RTP
    {2, PIN_PULL_DIS, 3, "PA.8"},
    {2, PIN_PULL_DIS, 3, "PA.9"},
    {2, PIN_PULL_DIS, 3, "PA.10"},
    {2, PIN_PULL_DIS, 3, "PA.11"},
#endif
#ifdef AIC_USING_I2C2
    {4, PIN_PULL_DIS, 3, "PA.8"},  // SCK
    {4, PIN_PULL_DIS, 3, "PA.9"},  // SDA
#endif
#if defined(AIC_USING_QSPI0) && !defined(AIC_SYSCFG_SIP_FLASH_ENABLE)
    /* qspi0 */
    {2, PIN_PULL_DIS, 3, "PB.0"},
    {2, PIN_PULL_DIS, 3, "PB.1"},
    {2, PIN_PULL_DIS, 3, "PB.2"},
    {2, PIN_PULL_DIS, 3, "PB.3"},
    {2, PIN_PULL_DIS, 3, "PB.4"},
    {2, PIN_PULL_DIS, 3, "PB.5"},
#endif
#if defined(AIC_USING_QSPI2) && defined(AIC_SYSCFG_SIP_FLASH_ENABLE) && !defined(AIC_USING_SDMC0)
    /* qspi2 */
    {1, PIN_PULL_DIS, 3, "PE.13"},//CS
    {1, PIN_PULL_DIS, 3, "PE.14"},//SDI
    {1, PIN_PULL_DIS, 3, "PE.12"},//CLK
    {1, PIN_PULL_DIS, 3, "PB.1"},//RST
#endif
#ifdef AIC_USING_SDMC0
    {2, PIN_PULL_UP, 7, "PB.6"},
    {2, PIN_PULL_UP, 7, "PB.7"},
    {2, PIN_PULL_UP, 7, "PB.8"},
    {2, PIN_PULL_UP, 7, "PB.9"},
    {2, PIN_PULL_UP, 7, "PB.10"},
    {2, PIN_PULL_UP, 7, "PB.11"},
#endif
#ifdef AIC_USING_SDMC1
    {2, PIN_PULL_UP, 3, "PC.0"},
    {2, PIN_PULL_UP, 3, "PC.1"},
    {2, PIN_PULL_UP, 3, "PC.2"},
    {2, PIN_PULL_UP, 3, "PC.3"},
    {2, PIN_PULL_UP, 3, "PC.4"},
    {2, PIN_PULL_UP, 3, "PC.5"},
    // {2, PIN_PULL_UP, 3, "PC.6"},
    // {5, PIN_PULL_UP, 3, "PE.11"},
#endif
#ifdef AIC_USING_CAP0
    {3, PIN_PULL_UP, 3, "PC.6"},
#endif
#ifdef AIC_USING_CAP1
    {3, PIN_PULL_UP, 3, "PC.7"},
#endif
#ifdef AIC_USING_CAP2
    {3, PIN_PULL_UP, 3, "PC.8"},
#endif
#ifdef AIC_USING_CAP3
    {3, PIN_PULL_UP, 3, "PC.9"},
#endif
#ifdef AIC_USING_CAP4
    {3, PIN_PULL_UP, 3, "PC.10"},
#endif
#ifdef AIC_USING_CAP5
    {3, PIN_PULL_UP, 3, "PC.11"},
#endif
#ifdef AIC_WIRELESS_LAN
    {1, PIN_PULL_DIS, 3, "PD.7"},  // WIFI_PWR_ON
#endif
#ifdef AIC_USING_I2C0
    {4, PIN_PULL_DIS, 3, "PD.0"}, // SCK
    {4, PIN_PULL_DIS, 3, "PD.1"}, // SDA
#endif
#ifdef AIC_PANEL_ENABLE_GPIO
    {1, PIN_PULL_DIS, 3, AIC_PANEL_ENABLE_GPIO},
#endif
#ifdef AIC_LVDS_LINK_0
    {3, PIN_PULL_DIS, 3, "PD.18"},
    {3, PIN_PULL_DIS, 3, "PD.19"},
    {3, PIN_PULL_DIS, 3, "PD.20"},
    {3, PIN_PULL_DIS, 3, "PD.21"},
    {3, PIN_PULL_DIS, 3, "PD.22"},
    {3, PIN_PULL_DIS, 3, "PD.23"},
    {3, PIN_PULL_DIS, 3, "PD.24"},
    {3, PIN_PULL_DIS, 3, "PD.25"},
    {3, PIN_PULL_DIS, 3, "PD.26"},
    {3, PIN_PULL_DIS, 3, "PD.27"},
#endif
#ifdef AIC_DISP_MIPI_DSI
    {1, PIN_PULL_DIS, 3, "PD.5"},
    {4, PIN_PULL_DIS, 3, "PD.18"},
    {4, PIN_PULL_DIS, 3, "PD.19"},
    {4, PIN_PULL_DIS, 3, "PD.20"},
    {4, PIN_PULL_DIS, 3, "PD.21"},
    {4, PIN_PULL_DIS, 3, "PD.22"},
    {4, PIN_PULL_DIS, 3, "PD.23"},
    // {4, PIN_PULL_DIS, 3, "PD.24"},
    // {4, PIN_PULL_DIS, 3, "PD.25"},
    // {4, PIN_PULL_DIS, 3, "PD.26"},
    // {4, PIN_PULL_DIS, 3, "PD.27"},
#endif
#ifdef AIC_USING_GMAC0
    /* gmac0 */
    {2, PIN_PULL_DIS, 3, "PE.0"},
    {2, PIN_PULL_DIS, 3, "PE.1"},
    {2, PIN_PULL_DIS, 3, "PE.2"},
    {2, PIN_PULL_DIS, 3, "PE.3"},
    {2, PIN_PULL_DIS, 3, "PE.4"},
    {2, PIN_PULL_DIS, 3, "PE.5"},
    {2, PIN_PULL_DIS, 3, "PE.7"},
    {2, PIN_PULL_DIS, 3, "PE.8"},
    {2, PIN_PULL_DIS, 3, "PE.9"},
    /* phy0 reset gpio */
    {1, PIN_PULL_DIS, 3, "PE.6"},
#endif
#ifdef AIC_USING_CLK_OUT0
    {6, PIN_PULL_DIS, 3, "PD.13"},
#endif
#ifdef AIC_USING_CLK_OUT1
    {2, PIN_PULL_DIS, 3, "PE.11"},
#endif
#ifdef AIC_USING_CLK_OUT2
    {2, PIN_PULL_DIS, 3, "PE.10"},
#endif
#ifdef AIC_USING_CLK_OUT3
    {7, PIN_PULL_DIS, 3, "PC.6"},
#endif
#ifdef AIC_USING_PWM1
    {3, PIN_PULL_DIS, 3, "PE.11"},
    //{3, PIN_PULL_DIS, 3, "PE.12"},
#endif
#ifdef AIC_USING_PWM2
    {3, PIN_PULL_DIS, 3, "PE.13"},
    //{3, PIN_PULL_DIS, 3, "PE.15"},
#endif
#ifdef AIC_USING_EPWM0
    {7, PIN_PULL_DIS, 3, "PD.26"},
    {7, PIN_PULL_DIS, 3, "PD.27"},
#endif
#ifdef AIC_USING_EPWM1
    {7, PIN_PULL_DIS, 3, "PD.24"},
    {7, PIN_PULL_DIS, 3, "PD.25"},
#endif
#ifdef AIC_USING_EPWM2
    {7, PIN_PULL_DIS, 3, "PD.22"},
    {7, PIN_PULL_DIS, 3, "PD.23"},
#endif
#ifdef AIC_USING_EPWM3
    {7, PIN_PULL_DIS, 3, "PD.20"},
    {7, PIN_PULL_DIS, 3, "PD.21"},
#endif
#ifdef AIC_USING_EPWM4
    {7, PIN_PULL_DIS, 3, "PD.18"},
    {7, PIN_PULL_DIS, 3, "PD.19"},
#endif
#ifdef AIC_USING_EPWM5
    {7, PIN_PULL_DIS, 3, "PD.16"},
    {7, PIN_PULL_DIS, 3, "PD.17"},
#endif
#ifdef AIC_USING_EPWM6
    {7, PIN_PULL_DIS, 3, "PD.14"},
    {7, PIN_PULL_DIS, 3, "PD.15"},
#endif
#ifdef AIC_USING_EPWM7
    {7, PIN_PULL_DIS, 3, "PD.12"},
    {7, PIN_PULL_DIS, 3, "PD.13"},
#endif
#ifdef AIC_USING_EPWM8
    {7, PIN_PULL_DIS, 3, "PD.10"},
    {7, PIN_PULL_DIS, 3, "PD.11"},
#endif
#ifdef AIC_USING_EPWM9
    {7, PIN_PULL_DIS, 3, "PD.8"},
    {7, PIN_PULL_DIS, 3, "PD.9"},
#endif
#ifdef AIC_USING_EPWM10
    {7, PIN_PULL_DIS, 3, "PD.2"},
    {7, PIN_PULL_DIS, 3, "PD.3"},
#endif
#ifdef AIC_USING_EPWM11
    {7, PIN_PULL_DIS, 3, "PD.0"},
    {7, PIN_PULL_DIS, 3, "PD.1"},
#endif
#if (defined(AIC_USING_USB0_DEVICE) || defined(AIC_USING_USB0_HOST))
    /* usb0 */
    {2, PIN_PULL_DIS, 3, "PO.0"},   // USB-DM
    {2, PIN_PULL_DIS, 3, "PO.1"},   // USB-DP
    {1, PIN_PULL_DIS, 3, "PD.8"},   // USB-ID
#endif
#ifdef AIC_USING_PSADC0
    {7, PIN_PULL_DIS, 3, "PA.0"},
#endif
#ifdef AIC_USING_PSADC1
    {7, PIN_PULL_DIS, 3, "PA.1"},
#endif
#ifdef AIC_USING_PSADC2
    {7, PIN_PULL_DIS, 3, "PA.2"},
#endif
#ifdef AIC_USING_PSADC3
    {7, PIN_PULL_DIS, 3, "PA.3"},
#endif
#ifdef AIC_USING_PSADC4
    {7, PIN_PULL_DIS, 3, "PA.4"},
#endif
#ifdef AIC_USING_PSADC5
    {7, PIN_PULL_DIS, 3, "PA.5"},
#endif
#ifdef AIC_USING_PSADC6
    {7, PIN_PULL_DIS, 3, "PA.6"},
#endif
#ifdef AIC_USING_PSADC7
    {7, PIN_PULL_DIS, 3, "PA.7"},
#endif
#ifdef AIC_USING_PSADC8
    {7, PIN_PULL_DIS, 3, "PA.8"},
#endif
#ifdef AIC_USING_PSADC9
    {7, PIN_PULL_DIS, 3, "PA.9"},
#endif
#ifdef AIC_USING_PSADC10
    {7, PIN_PULL_DIS, 3, "PA.10"},
#endif
#ifdef AIC_USING_PSADC11
    {7, PIN_PULL_DIS, 3, "PA.11"},
#endif
#ifdef AIC_USING_PSADC12
    {7, PIN_PULL_DIS, 3, "PA.12"},
#endif
#ifdef AIC_USING_PSADC13
    {7, PIN_PULL_DIS, 3, "PA.13"},
#endif
#ifdef AIC_USING_PSADC14
    {7, PIN_PULL_DIS, 3, "PA.14"},
#endif
#ifdef AIC_USING_PSADC15
    {7, PIN_PULL_DIS, 3, "PA.15"},
#endif
#ifdef AIC_USING_GPAI0
    {2, PIN_PULL_DIS, 3, "PA.0"},
#endif
#ifdef AIC_USING_GPAI1
    {2, PIN_PULL_DIS, 3, "PA.1"},
#endif
#ifdef AIC_USING_GPAI2
    {2, PIN_PULL_DIS, 3, "PA.2"},
#endif
#ifdef AIC_USING_GPAI3
    {2, PIN_PULL_DIS, 3, "PA.3"},
#endif
#ifdef AIC_USING_GPAI4
    {2, PIN_PULL_DIS, 3, "PA.4"},
#endif
#ifdef AIC_USING_GPAI5
    {2, PIN_PULL_DIS, 3, "PA.5"},
#endif
#ifdef AIC_USING_GPAI6
    {2, PIN_PULL_DIS, 3, "PA.6"},
#endif
#ifdef AIC_USING_GPAI7
    {2, PIN_PULL_DIS, 3, "PA.7"},
#endif
#ifdef AIC_USING_CTP
    {1, PIN_PULL_DIS, 3, AIC_TOUCH_PANEL_RST_PIN},
    {1, PIN_PULL_DIS, 3, AIC_TOUCH_PANEL_INT_PIN},
#endif
#ifdef AIC_USING_PM
    {1, PIN_PULL_DIS, 3, AIC_BOARD_LEVEL_POWER_PIN, FLAG_POWER_PIN},
#ifdef AIC_PM_DEMO
    {1, PIN_PULL_DIS, 3, AIC_PM_POWER_KEY_GPIO, FLAG_WAKEUP_SOURCE},
#endif
#endif
};

uint32_t aic_pinmux_config_size = ARRAY_SIZE(aic_pinmux_config);
