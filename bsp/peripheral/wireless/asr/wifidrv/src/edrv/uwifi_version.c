/**
 ******************************************************************************
 *
 * @file uwifi_version.c
 *
 * @brief return asr version info
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */
#define ASR_WIFI_VERSION "ASR_RTOS_WIFI-V2.6.5"
const char *asr_get_wifi_version(void)
{
    return ASR_WIFI_VERSION;
}
