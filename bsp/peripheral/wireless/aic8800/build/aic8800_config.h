/*--------------------------------------------------------------------------------------------------------------------
# (C) Copyright 2006, 2007 Aicsemi. All Rights Reserved.
#-------------------------------------------------------------------------------------------------------------------

#=========================================================================
# File Name      : aicwifi config
# Description    : Main make file for the aicwifi.
#
# Usage          : make [-s] -f aicwifi.mak OPT_FILE=<path>/<opt_file>
#
# Notes          : The options file defines macro values defined
#                  by the environment, target, and groups. It
#                  must be included for proper package building.
#
#=========================================================================*/
#ifndef _AICWF_CONFIG_H
#define	_AICWF_CONFIG_H
#include "rtconfig.h"

#define     CONFIG_AIC8800DW_SUPPORT
#define     CONFIG_AIC8800D80_SUPPORT
#define     CONFIG_SDIO_SUPPORT
//#define     CONFIG_OOB

//// firmware bin file
#define     CONFIG_DOWNLOAD_FW
//#define     CONFIG_LOAD_FW_FROM_FLASH
#define     CONFIG_WIFI_MODE_RFTEST

//// wifi setting
#define     CONFIG_RWNX_FULLMAC
//#define     CONFIG_OFFCHANNEL
#define     USE_5G
#define     CONFIG_USE_5G

//// hardware settings
#define     CONFIG_PMIC_SETTING
#define     CONFIG_LOAD_USERCONFIG
#ifdef CONFIG_AIC8800_VRF_DCDC_MODE
#define     CONFIG_VRF_DCDC_MODE
#endif
#define     CONFIG_DPD
#define     CONFIG_FORCE_DPD_CALIB

//// SDIO and AP power save
//#define     CONFIG_SDIO_BUS_PWRCTRL
#define     CONFIG_SDIO_BUS_PWRCTRL_DYN

//// software settings
#define     CONFIG_LWIP
#define     CONFIG_REORD_FORWARD_LIST
//#define     CONFIG_RX_NOCOPY
#define     CONFIG_RX_WAIT_LWIP
//#define     CONFIG_APP_IPERF

/// WPA3 Need
#define     CONFIG_MBEDTLS

// flow ctr delay ms
#define     SDIO_FC_NARB

//// debug log
#define     CONFIG_RWNX_DBG
#define     AIC_LOG_DEBUG_ON

////rm driver one key ,just reset env
//#define     CONFIG_DRIVER_ORM

//rtthread os
//#define     PLATFORM_ASR_THREADX
#define     CONFIG_RTTHEAD_PLAT

//bt support
#ifdef CONFIG_AIC8800_BT_SUPPORT
#define CONFIG_BT_SUPPORT
#endif

#endif

