/**
 ****************************************************************************************
 *
 * @file asr_types.h
 *
 * @brief define basic type
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>
#include <stdbool.h>

#include "typecheck.h"

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef BIT
#define BIT(n)           (1<<n)
#endif

#define CO_BIT(pos)      (1U<<(pos))

/// structure of a list element header
struct co_list_hdr
{
    /// Pointer to the next element in the list
    struct co_list_hdr *next;
};

inline static uint16_t  ___swap16(uint16_t x)
{
    uint16_t __x = x;
    return ((uint16_t)((((uint16_t)(__x) & (uint16_t)0x00ffU) << 8) |
           (((uint16_t)(__x) & (uint16_t)0xff00U) >> 8)));
}

inline static uint16_t __arch__swap16(uint16_t x)
{
    return ___swap16(x);
}

inline static uint16_t __fswap16(uint16_t x)
{
    return __arch__swap16(x);
}

#define __swap16(x) __fswap16(x)

#define __cpu_to_be16(x) __swap16(x)
#define __be16_to_cpu(x) __swap16(x)

#define wlan_htons(x) __cpu_to_be16(x)
#define wlan_ntohs(x) __be16_to_cpu(x)

#endif //_TYPES_H_


