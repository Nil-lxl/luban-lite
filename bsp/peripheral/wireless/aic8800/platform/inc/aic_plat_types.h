/*
 * Copyright (C) 2018-2020 AICSemi Ltd.
 *
 * All Rights Reserved
 */

#ifndef _AIC_TYPES_H_
#define _AIC_TYPES_H_

#include "aic8800_config.h"
#include <stdbool.h>

#ifndef BIT
#define BIT(x)      (1ul << (x))
#endif

#ifndef NULL
#define NULL (void *)0
#endif

#include <stdint.h>

#define SYSYINT_H            1

#define true                1
#define false               0
#define TRUE                1
#define FALSE               0
#define pdTRUE              1
#define pdFALSE             0

typedef unsigned char       BOOL;
typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned int        U32;
typedef signed char         S8;
typedef signed short        S16;
typedef signed int          S32;
#if !SYSYINT_H
typedef unsigned char       bool;
#endif
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;
typedef signed long long    s64;

typedef unsigned char       BOOL_L;
typedef unsigned char       U8_L;
typedef unsigned short      U16_L;
typedef unsigned int        U32_L;
typedef signed char         S8_L;
typedef signed short        S16_L;
typedef signed int          S32_L;
typedef unsigned char       bool_l;
typedef unsigned char       u8_l;
typedef unsigned short      u16_l;
typedef unsigned int        u32_l;
typedef unsigned long long  u64_l;
typedef signed char         s8_l;
typedef signed short        s16_l;
typedef signed int          s32_l;
typedef signed long long    s64_l;

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned int        uint;
typedef signed char         int8;
typedef signed short        int16;
typedef signed int          int32;
typedef signed long long    int64;

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
//typedef unsigned int        uint32_t;

#if !SYSYINT_H
typedef unsigned long long  uint64_t;
#endif

typedef signed char         int8_t;
typedef signed short        int16_t;
//typedef signed int          int32_t;
#if !SYSYINT_H
typedef signed long long    int64_t;
#endif

typedef unsigned char       byte;    /*  unsigned 8-bit data     */
typedef unsigned short      word;    /*  unsigned 16-bit data    */
typedef unsigned long       dword;   /*  unsigned 32-bit data    */

#endif /* _AIC_TYPES_H_ */

