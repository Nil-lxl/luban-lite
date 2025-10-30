/**
 ****************************************************************************************
 *
 * @file dbg_assert.h
 *
 * @brief File containing the definitions of the assertion macros.
 *
 ****************************************************************************************
 */

#ifndef _DBG_ASSERT_H_
#define _DBG_ASSERT_H_

/**
 ****************************************************************************************
 * @defgroup ASSERT ASSERT
 * @ingroup DEBUG
 * @brief Assertion management module
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#ifdef PLATFORM_ASR_THREADX
#include "global_types.h"

/// Assertions showing a critical error that could require a full system reset
#define AIC_ASSERT_ERR(cond)  ASSERT(cond)
#endif

#ifdef  CONFIG_RTTHEAD_PLAT
#include "ulog_def.h"
#define AIC_ASSERT_ERR  ASSERT
#endif

#endif // _DBG_ASSERT_H_
