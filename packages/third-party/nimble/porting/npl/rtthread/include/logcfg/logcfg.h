/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_LOGCFG_H
#define BLE_LOGCFG_H

#include <stdio.h>
#include <aic_core.h>

#define CONFIG_BLE_PRINTF(...) printf(__VA_ARGS__)

#define CONFIG_BLE_DBG_LEVEL LPKG_NIMBLE_DEBUG_LEVEL
#ifndef CONFIG_BLE_DBG_LEVEL
#define CONFIG_BLE_DBG_LEVEL BLE_DBG_INFO
#endif

/* Enable print with color */
#define CONFIG_BLE_PRINTF_COLOR_ENABLE

/* DEBUG level */
#define BLE_DBG_NONE    0
#define BLE_DBG_ERROR   3
#define BLE_DBG_WARNING 4
#define BLE_DBG_INFO    6
#define BLE_DBG_LOG     7

#ifndef BLE_DBG_TAG
#define BLE_DBG_TAG "BLE"
#endif

/*
 * The color for terminal (foreground)
 * BLACK    30
 * RED      31
 * GREEN    32
 * YELLOW   33
 * BLUE     34
 * PURPLE   35
 * CYAN     36
 * WHITE    37
 */

#ifdef  CONFIG_BLE_PRINTF_COLOR_ENABLE
#define _BLE_DBG_COLOR(n) CONFIG_BLE_PRINTF("\033[" #n "m")
#define _BLE_DBG_LOG_HDR(lvl_name, color_n, tag) \
    CONFIG_BLE_PRINTF("\033[" #color_n "m[" lvl_name "/" BLE_DBG_TAG "/" tag "] ")
#define _BLE_DBG_LOG_X_END \
    CONFIG_BLE_PRINTF("\033[0m")
#else
#define _BLE_DBG_COLOR(n)
#define _BLE_DBG_LOG_HDR(lvl_name, color_n, tag) \
    CONFIG_BLE_PRINTF("[" lvl_name "/" BLE_DBG_TAG "/" tag "] ")
#define _BLE_DBG_LOG_X_END
#endif

#define ble_dbg_log_line(lvl, color_n, tag, fmt, ...) \
    do {                                              \
        _BLE_DBG_LOG_HDR(lvl, color_n, tag);          \
        CONFIG_BLE_PRINTF(fmt, ##__VA_ARGS__);        \
        _BLE_DBG_LOG_X_END;                           \
    } while (0)

#if (CONFIG_BLE_DBG_LEVEL >= BLE_DBG_LOG)
#define BLE_LOG_RAW_DEBUG(fmt, ...)  CONFIG_BLE_PRINTF(fmt, ##__VA_ARGS__)
#define MODLOG_DFLT_DEBUG(fmt, ...)  ble_dbg_log_line("D", 0, "", fmt, ##__VA_ARGS__)
#define BLE_HS_LOG_DEBUG(fmt, ...)  ble_dbg_log_line("D", 0, "HS", fmt, ##__VA_ARGS__)
#define BLE_EATT_LOG_DEBUG(fmt, ...)  ble_dbg_log_line("D", 0, "EATT", fmt, ##__VA_ARGS__)
#else
#define BLE_LOG_RAW_DEBUG(...)  {}
#define MODLOG_DFLT_DEBUG(...)  {}
#define BLE_HS_LOG_DEBUG(...)  {}
#define BLE_EATT_LOG_DEBUG(...)  {}
#endif

#if (CONFIG_BLE_DBG_LEVEL >= BLE_DBG_INFO)
#define BLE_LOG_RAW_INFO(fmt, ...)  CONFIG_BLE_PRINTF(fmt, ##__VA_ARGS__)
#define MODLOG_DFLT_INFO(fmt, ...)  ble_dbg_log_line("I", 0, "", fmt, ##__VA_ARGS__)
#define BLE_HS_LOG_INFO(fmt, ...)  ble_dbg_log_line("I", 0, "HS", fmt, ##__VA_ARGS__)
#define BLE_EATT_LOG_INFO(fmt, ...)  ble_dbg_log_line("I", 0, "EATT", fmt, ##__VA_ARGS__)
#else
#define BLE_LOG_RAW_INFO(...)  {}
#define MODLOG_DFLT_INFO(...)  {}
#define BLE_HS_LOG_INFO(...)  {}
#define BLE_EATT_LOG_INFO(...)  {}
#endif

#if (CONFIG_BLE_DBG_LEVEL >= BLE_DBG_WARNING)
#define BLE_LOG_RAW_WARN(fmt, ...)  CONFIG_BLE_PRINTF(fmt, ##__VA_ARGS__)
#define MODLOG_DFLT_WARN(fmt, ...)  ble_dbg_log_line("W", 0, "", fmt, ##__VA_ARGS__)
#define BLE_HS_LOG_WARN(fmt, ...)  ble_dbg_log_line("W", 0, "HS", fmt, ##__VA_ARGS__)
#define BLE_EATT_LOG_WARN(fmt, ...)  ble_dbg_log_line("W", 0, "EATT", fmt, ##__VA_ARGS__)
#else
#define BLE_LOG_RAW_WARN(...)  {}
#define MODLOG_DFLT_WARN(...)  {}
#define BLE_HS_LOG_WARN(...)  {}
#define BLE_EATT_LOG_WARN(...)  {}
#endif

#if (CONFIG_BLE_DBG_LEVEL >= BLE_DBG_ERROR)
#define BLE_LOG_RAW_ERROR(fmt, ...)  CONFIG_BLE_PRINTF(fmt, ##__VA_ARGS__)
#define MODLOG_DFLT_ERROR(fmt, ...)  ble_dbg_log_line("E", 0, "", fmt, ##__VA_ARGS__)
#define BLE_HS_LOG_ERROR(fmt, ...)  ble_dbg_log_line("E", 0, "HS", fmt, ##__VA_ARGS__)
#define BLE_EATT_LOG_ERROR(fmt, ...)  ble_dbg_log_line("E", 0, "EATT", fmt, ##__VA_ARGS__)
#else
#define BLE_LOG_RAW_DEBUG(...)  {}
#define MODLOG_DFLT_ERROR(...)  {}
#define BLE_HS_LOG_ERROR(...)  {}
#define BLE_EATT_LOG_ERROR(...)  {}
#endif

#define BLE_LOG_RAW(...) CONFIG_BLE_PRINTF(__VA_ARGS__)

#define BLE_HS_LOG(ml_lvl_, ...) BLE_LOG_RAW_ ## ml_lvl_(__VA_ARGS__)
#define BLE_EATT_LOG(ml_lvl_, ...) BLE_LOG_RAW__ ## ml_lvl_(__VA_ARGS__)
#define MODLOG_DFLT(ml_lvl_, ...) BLE_LOG_RAW_ ## ml_lvl_(__VA_ARGS__)
#define MODLOG_DFLT_LOG_BUFFER_HEX(msg, buf, len) \
    do {                                          \
        hexdump_msg(msg, buf, len, 1);            \
    } while (0)

#endif
