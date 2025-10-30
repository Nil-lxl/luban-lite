/**
 ****************************************************************************************
 *
 * @file wifi_config.h
 *
 * @brief mac configuration.
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#define CFG_AMSDU_TEST

//#include "asr_config.h"
#ifndef CFG_STA_MAX
#define CFG_STA_MAX 5  //default max client is 5 for AP mode
#endif

/// Max number of peer devices managed
#define NX_REMOTE_STA_MAX CFG_STA_MAX


#ifdef CFG_UWIFI_TEST
#define UWIFI_TEST 1
#else
#define UWIFI_TEST 0
#endif

#endif  //WIFI_CONFIG_H
