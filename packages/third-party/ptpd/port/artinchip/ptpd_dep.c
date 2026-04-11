/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Authors: lv.wu <lv.wu@artinchip.com>      
 */

#include <aic_mac.h>

#include "ptpd_dep.h"

/* Examples of subsecond increment and addend values using ptp-clok = 50MHz

 Addend * Increment = 2^32 / ptp-clock

 ptp_tick = Increment * 10^9 / 2^31

 +-----------+-----------+------------+
 | ptp tick  | Increment | Addend     |
 +-----------+-----------+------------+
 |   40 ns   |    40     | 0x7FFFFFFF |
 +-----------+-----------+------------+
*/

#define AICMAC_PTP_DEBUG_ADDEND       0    // debug for fine clock
#define AICMAC_PTP_PORT               0

#define AICMAC_PTP_ADJUSTTIME_ADD  ((uint32_t)(0 << 31))
#define AICMAC_PTP_ADJUSTTIME_SUB  ((uint32_t)(1 << 31))

#if AICMAC_PTP_DEBUG_ADDEND
#include "rtthread.h"
volatile uint32_t AICMAC_PTP_DEFAULT_ADDEND=0x7FFFFFFF;
#else
#define AICMAC_PTP_DEFAULT_ADDEND  0x7FFFFFFF
#endif

/*------------------------artinchip gmac PTP options--------------------------*/

uint32_t aicmac_get_timestamp_added(void)
{
    return readl(MAC(AICMAC_PTP_PORT, tmsmpaddend));
}

static void aicmac_set_incr(uint32_t incr)
{
    writel((incr & 0xFF), MAC(AICMAC_PTP_PORT, subsecincr));
}

static void aicmac_config_addend(uint32_t addend)
{
    uint32_t reg_val;

    writel(addend, MAC(AICMAC_PTP_PORT, tmsmpaddend));

    reg_val = readl(MAC(AICMAC_PTP_PORT, tmstmpctl));
    reg_val |= (1 << 5);
    writel(reg_val, MAC(AICMAC_PTP_PORT, tmstmpctl));

    /* wait for addend updated */
    while(readl(MAC(AICMAC_PTP_PORT, tmstmpctl)) & (1 << 5));
}

static void aicmac_adjust_time(int32_t sec, int32_t nsec, int32_t sign)
{
    uint32_t reg_val;

    writel(sec, MAC(AICMAC_PTP_PORT, updttimesec));

    writel(nanosecond2subsecond(nsec) | sign, MAC(AICMAC_PTP_PORT, updttimenanosec));

    /* Set Time stamp control register bit 3 (Time stamp update). */
    reg_val = readl(MAC(AICMAC_PTP_PORT, tmstmpctl));
    reg_val |= (1 << 3);
    writel(reg_val, MAC(AICMAC_PTP_PORT, tmstmpctl));

    /* wait for systime updated */
    while (readl(MAC(AICMAC_PTP_PORT, tmstmpctl)) & (1 << 3));
}

static void aicmac_init_time(int32_t sec, int32_t nsec)
{
    uint32_t reg_val;

    writel(sec, MAC(AICMAC_PTP_PORT, updttimesec));

    writel(nanosecond2subsecond(nsec), MAC(AICMAC_PTP_PORT, updttimenanosec));

    /* Set Time stamp control register bit 2 (Time stamp init). */
    reg_val = readl(MAC(AICMAC_PTP_PORT, tmstmpctl));
    reg_val |= (1 << 2);
    writel(reg_val, MAC(AICMAC_PTP_PORT, tmstmpctl));

    /* wait for systime updated */
    while (readl(MAC(AICMAC_PTP_PORT, tmstmpctl)) & (1 << 2));
}

#if AICMAC_PTP_DEBUG_ADDEND
void change_added(int argc, char *argv[])
{
    uint32_t tmp;

    tmp = strtoul(argv[1], RT_NULL, 0);
    printf("change addend is 0x%x\n", tmp);
    __disable_irq();
    AICMAC_PTP_DEFAULT_ADDEND = tmp;

    __enable_irq();
    aicmac_config_addend(tmp);

    if (argv[2] == NULL)
        return;
    tmp = strtoul(argv[2], RT_NULL, 0);
    aicmac_set_incr(tmp);

}
MSH_CMD_EXPORT_ALIAS(change_added, add, change added);

void get_systime(int argc, char *argv[])
{
    struct ptptime_t timestamp;

    ETH_PTPTime_GetTime(&timestamp);
    printf("added == 0x%x, incr == %d\nsystime :: %d sec, %d, nsec\n",
            aicmac_get_timestamp_added(),
            readl(MAC(AICMAC_PTP_PORT, subsecincr)),
            timestamp.tv_sec,
            timestamp.tv_nsec);
}
MSH_CMD_EXPORT_ALIAS(get_systime, ptptime, get ptp time para);
#endif
/*------------------------------Port for PTP----------------------------------*/
/*******************************************************************************
* Function Name  : ETH_PTPStart
* Description    : Initialize timestamping ability of ETH
* Output         : None
* Return         : None
*******************************************************************************/
void ETH_PTPStart(void)
{
    uint32_t reg_val;

    /* Program Time stamp register bit 0 to enable time stamping. */
    reg_val = readl(MAC(AICMAC_PTP_PORT, tmstmpctl));
    reg_val |= (1 << 0);
    writel(reg_val, MAC(AICMAC_PTP_PORT, tmstmpctl));

    /* Program the Subsecond increment register based on the PTP clock frequency. */
    /* to achieve 20 ns accuracy, the value is 20 */
#if AICMAC_PTP_NANO_SEC_REG_SET
    aicmac_set_incr(40);
#else
    aicmac_set_incr(86);
#endif
    aicmac_config_addend(AICMAC_PTP_DEFAULT_ADDEND);

    /* To select the Fine correction method (if required),
     * program Time stamp control register  bit 1. */
    reg_val = readl(MAC(AICMAC_PTP_PORT, tmstmpctl));
    reg_val |= (1 << 1);
    writel(reg_val, MAC(AICMAC_PTP_PORT, tmstmpctl));

    /* Enable timestamp for all rx frames */
    reg_val = readl(MAC(AICMAC_PTP_PORT, tmstmpctl));
    reg_val |= (1 << 8);
    writel(reg_val, MAC(AICMAC_PTP_PORT, tmstmpctl));

#if AICMAC_PTP_NANO_SEC_REG_SET
    /* Set nano second register 1 ns */
    reg_val = readl(MAC(AICMAC_PTP_PORT, tmstmpctl));
    reg_val |= (1 << 9);
    writel(reg_val, MAC(AICMAC_PTP_PORT, tmstmpctl));
#endif

    /* enable auxiliary snapshot0s */
    reg_val = readl(MAC(AICMAC_PTP_PORT, tmstmpctl));
    reg_val |= (1 << 25);
    writel(reg_val, MAC(AICMAC_PTP_PORT, tmstmpctl));

    /* Program the Time stamp high update and Time stamp low update registers
     * with the appropriate time value. */
    aicmac_init_time(0, 0);
}

/*******************************************************************************
* Function Name  : ETH_PTPTimeStampSetTime
* Description    : Initialize time base
* Input          : Time with sign
* Output         : None
* Return         : None
*******************************************************************************/
void ETH_PTPTime_SetTime(struct ptptime_t * timestamp)
{
    aicmac_init_time(timestamp->tv_sec, timestamp->tv_nsec);
}

/*******************************************************************************
* Function Name  : ETH_PTPTime_GetTime
* Description    : Get currunt system time
* Input          : None
* Output         : Currunt system time
* Return         : None
*******************************************************************************/
void ETH_PTPTime_GetTime(struct ptptime_t * timestamp)
{
    timestamp->tv_sec = readl(MAC(AICMAC_PTP_PORT, systimesec));
    timestamp->tv_nsec = subsecond2nanosecond(readl(MAC(AICMAC_PTP_PORT, systimenanosec)));

    /* in case nanosecond register overflow */
    if(timestamp->tv_sec != readl(MAC(AICMAC_PTP_PORT, systimesec)))
    {
        timestamp->tv_sec = readl(MAC(AICMAC_PTP_PORT, systimesec));
        timestamp->tv_nsec = subsecond2nanosecond(readl(MAC(AICMAC_PTP_PORT, systimenanosec)));
    }
}

/*******************************************************************************
* Function Name  : ETH_PTPTimeStampAdjFreq
* Description    : Updates time stamp addend register
* Input          : Correction value in thousandth of ppm (Adj*10^9)
* Output         : None
* Return         : None
*******************************************************************************/
void ETH_PTPTime_AdjFreq(int32_t Adj)
{
    u32 addend;

    addend = Adj * 1.7 + AICMAC_PTP_DEFAULT_ADDEND;

    aicmac_config_addend(addend);
}

/*******************************************************************************
* Function Name  : ETH_PTPTimeStampUpdateOffset
* Description    : Updates time base offset
* Input          : Time offset with sign
* Output         : None
* Return         : None
*******************************************************************************/
void ETH_PTPTime_UpdateOffset(struct ptptime_t * timeoffset)
{
    uint32_t Sign;
    uint32_t SecondValue;
    uint32_t NanoSecondValue;
    uint32_t addend;

    /* determine sign and correct Second and Nanosecond values */
    if(timeoffset->tv_sec < 0 || (timeoffset->tv_sec == 0 && timeoffset->tv_nsec < 0))
    {
        Sign = AICMAC_PTP_ADJUSTTIME_SUB;
        SecondValue = -timeoffset->tv_sec;
        NanoSecondValue = -timeoffset->tv_nsec;
    }
    else
    {
        Sign = AICMAC_PTP_ADJUSTTIME_ADD;
        SecondValue = timeoffset->tv_sec;
        NanoSecondValue = timeoffset->tv_nsec;
    }

    /* read old addend register value*/
    addend = aicmac_get_timestamp_added();

    /* Write the offset (positive or negative) in the Time stamp update high and low registers. */
    aicmac_adjust_time(SecondValue, NanoSecondValue, Sign);

    /* Write back old addend register value. */
    aicmac_config_addend(addend);
}
