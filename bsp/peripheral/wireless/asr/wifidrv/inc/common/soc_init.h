/**
 ****************************************************************************************
 *
 * @file soc_init.h
 *
 * @brief soc init
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */

#ifndef __SOC_INIT__
#define __SOC_INIT__

/** @brief  soc basic init, call at the beginning of main function
 */
int soc_pre_init(void);

/** @brief  soc and rtos basic init, call after soc_pre_init()
 */
int soc_init(void);

/** @brief  register uart for printf log.
 */
void printf_uart_register(uint8_t uart_idx);

#endif //__SOC_INIT__
