/**
  ******************************************************************************
  * @file    jy6311_user_cfg.h
  * @author  Amplore
  * @brief   Header file for jy6311 driver user config
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 Amplore.
  * All rights reserved.</center></h2>
  *
  * This software is licensed by Amplore under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _JY6311_USER_CFG_H_
#define _JY6311_USER_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>


/** @addtogroup JY6311_Driver
  * @{
  */


/* Exported Constants --------------------------------------------------------*/
/** @addtogroup JY6311_Exported_Constants
  * @{
  */

/** @defgroup JY6311_User_Config_Constants JY6311 User Config Constants
  * @brief    JY6311 User Config Constants
  * @{
  */

#define JY6311_MODULE_ENABLED               /*!< JY6311 Module Enable   */

/**
  * @brief JY6311 debug level macro definition
  */
#define JY6311_DBG_NONE            (0)      /*!< Debug None             */
#define JY6311_DBG_ERROR           (1)      /*!< Debug Error            */
#define JY6311_DBG_WARNING         (2)      /*!< Debug Warning          */
#define JY6311_DBG_INFO            (3)      /*!< Debug Information      */
#define JY6311_DBG_LOG             (4)      /*!< Debug Log              */

/**
  * @brief JY6311 debug level user select
  * @note The debug information which is less than or equal to this debug level will be printed
  */
#define JY6311_DBG_LVL             (JY6311_DBG_WARNING)

/**
  * @}
  */

/**
  * @}
  */


#ifdef JY6311_MODULE_ENABLED

/* Exported Macros -----------------------------------------------------------*/
/** @addtogroup JY6311_Exported_Macros
  * @{
  */

/** @defgroup JY6311_User_Config_Macros JY6311 User Config Macros
  * @brief    JY6311 User Config Macros
  * @{
  */

/**
  * @brief JY6311 debug print user macro definition
  */
#define JY6311_DBG_PRINT_USER(fmt, ...)             printf(fmt, ##__VA_ARGS__)


/**
  * @brief JY6311 delay ms user macro definition
  */
#define JY6311_DELAY_MS_USER(ms)                    jy6311_delay_ms(ms)

/**
  * @brief  JY6311 I2C read byte function macro definition
  * @param  i2c_addr JY6311 I2C address
  * @param  reg JY6311 register address to read
  * @return register read value
  */
#define JY6311_I2C_READ_BYTE(i2c_addr, reg)         jy6311_i2c_read_byte(i2c_addr, reg)

/**
  * @brief  JY6311 I2C write byte function macro definition
  * @param  i2c_addr JY6311 I2C address
  * @param  reg JY6311 register address to write
  * @param  val register value
  * @retval 0      Register write Success
  * @retval others Register write Failed
  */
#define JY6311_I2C_WRITE_BYTE(i2c_addr, reg, val)   jy6311_i2c_write_byte(i2c_addr, reg, val)

/**
  * @}
  */

/**
  * @}
  */


/* Exported Types ------------------------------------------------------------*/
/** @addtogroup JY6311_Exported_Types
  * @{
  */

#if 0
/** @defgroup JY6311_User_Config_Types JY6311 User Config Types
  * @brief    JY6311 User Config Types
  * @{
  */

/**
  * @brief JY6311 data type definition
  */
typedef   signed       char int8_t;         /*!< int8_t typedef         */
typedef   signed short  int int16_t;        /*!< int16_t typedef        */
typedef   signed        int int32_t;        /*!< int32_t typedef        */
typedef unsigned       char uint8_t;        /*!< uint8_t typedef        */
typedef unsigned short  int uint16_t;       /*!< uint16_t typedef       */
typedef unsigned        int uint32_t;       /*!< uint32_t typedef       */

/**
  * @}
  */
#endif

/**
  * @}
  */


/* Exported Variables --------------------------------------------------------*/
/* Exported Functions --------------------------------------------------------*/
/** @addtogroup JY6311_Exported_Functions
  * @{
  */

/** @defgroup JY6311_User_Config_Functions JY6311 User Config Functions
  * @brief    JY6311 User Config Functions
  * @{
  */

/**
  * @brief  JY6311 delay ms
  * @param  ms millisecond
  */
void jy6311_delay_ms(int ms);

/**
  * @brief  JY6311 I2C read byte
  * @param  i2c_addr JY6311 I2C address
  * @param  reg JY6311 register address to read
  * @return register read value
  */
unsigned char jy6311_i2c_read_byte(unsigned char i2c_addr, unsigned char reg);

/**
  * @brief  JY6311 I2C write byte
  * @param  i2c_addr JY6311 I2C address
  * @param  reg JY6311 register address to write
  * @param  val register value
  * @retval 0      Register write Success
  * @retval others Register write Failed
  */
signed char jy6311_i2c_write_byte(unsigned char i2c_addr, unsigned char reg, unsigned char val);


void jy6311_i2c_get_client(void);
/**
  * @}
  */

/**
  * @}
  */


#endif  /* JY6311_MODULE_ENABLED */


/**
  * @}
  */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _JY6311_USER_CFG_H_ */

/************************* (C) COPYRIGHT Amplore *****END OF FILE***********/
