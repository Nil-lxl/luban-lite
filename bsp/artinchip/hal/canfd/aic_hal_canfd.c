/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao Li <siyao.li@artinchip.com>
 */

#include "hal_canfd.h"
#include "aic_hal_clk.h"
#include "hal_dma.h"
#include "aic_dma_id.h"

#define abs(x)                  ((x) >= 0 ? (x):-(x))

/* CANFD Bus Error Type */
canfd_bus_err_msg_t bus_err_type[] = {
    {0x0, "No Error"},
    {0x1, "Bit Error"},
    {0x2, "Format Error"},
    {0x3, "Stuff Error"},
    {0x4, "ACK Error"},
    {0x5, "CRC Error"},
    {0x6, "Other Error"}
};

/* CANFD Bus Arbitration Lost */
canfd_bus_err_msg_t bus_arb_lost[] = {
    {0x00, "ID28 ArbLost"},
    {0x01, "ID27 ArbLost"},
    {0x02, "ID26 ArbLost"},
    {0x03, "ID25 ArbLost"},
    {0x04, "ID24 ArbLost"},
    {0x05, "ID23 ArbLost"},
    {0x06, "ID22 ArbLost"},
    {0x07, "ID21 ArbLost"},
    {0x08, "ID20 ArbLost"},
    {0x09, "ID19 ArbLost"},
    {0x0A, "ID18 ArbLost"},
    {0x0B, "SRTR ArbLost"},
    {0x0C, "IDE  ArbLost"},
    {0x0D, "ID17 ArbLost"},
    {0x0E, "ID16 ArbLost"},
    {0x0F, "ID15 ArbLost"},
    {0x10, "ID14 ArbLost"},
    {0x11, "ID13 ArbLost"},
    {0x12, "ID12 ArbLost"},
    {0x13, "ID11 ArbLost"},
    {0x14, "ID10 ArbLost"},
    {0x15, "ID9 ArbLost"},
    {0x16, "ID8 ArbLost"},
    {0x17, "ID7 ArbLost"},
    {0x18, "ID6 ArbLost"},
    {0x19, "ID5 ArbLost"},
    {0x1A, "ID4 ArbLost"},
    {0x1B, "ID3 ArbLost"},
    {0x1C, "ID2 ArbLost"},
    {0x1D, "ID1 ArbLost"},
    {0x1E, "ID0 ArbLost"},
    {0x1F, "RTR ArbLost"},
};

canfd_bus_err_msg_t bus_state[] = {
    {0, "active status"},
    {1, "warning status"},
    {2, "passive status"},
    {3, "bus off"},
};

struct canfd_bittiming_const btc_can = {
    .tseg1_min = 2,
    .tseg1_max = 65,
    .tseg2_min = 1,
    .tseg2_max = 8,
    .sjw_max = 16,
    .brp_min = 1,
    .brp_max = 256,
    .brp_inc = 1,
};

struct canfd_bittiming_const btc_canfd = {
    .tseg1_min = 2,
    .tseg1_max = 65,
    .tseg2_min = 1,
    .tseg2_max = 16,
    .sjw_max = 8,
    .brp_min = 1,
    .brp_max = 256,
    .brp_inc = 1,
};

struct canfd_bittiming_const dbtc_canfd = {
    .tseg1_min = 2,
    .tseg1_max = 17,
    .tseg2_min = 1,
    .tseg2_max = 8,
    .sjw_max = 16,
    .brp_min = 1,
    .brp_max = 256,
    .brp_inc = 1,
};

static const u8 canfd_dlc2len[] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 12, 16, 20, 24, 32, 48, 64
};

static const u8 canfd_len2dlc[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8,  /* 0 - 8 */
    9, 9, 9, 9,                 /* 9 - 12 */
    10, 10, 10, 10,             /* 13 - 16 */
    11, 11, 11, 11,             /* 17 - 20 */
    12, 12, 12, 12,             /* 21 - 24 */
    13, 13, 13, 13, 13, 13, 13, 13,    /* 25 - 32 */
    14, 14, 14, 14, 14, 14, 14, 14,    /* 33 - 40 */
    14, 14, 14, 14, 14, 14, 14, 14,    /* 41 - 48 */
    15, 15, 15, 15, 15, 15, 15, 15,    /* 49 - 56 */
    15, 15, 15, 15, 15, 15, 15, 15     /* 57 - 64 */
};

u8 hal_canfd_dlc2len(u8 dlc)
{
    return canfd_dlc2len[dlc & 0x0F];
}

u8 hal_canfd_len2dlc(u8 len)
{
    return canfd_len2dlc[len];
}

void canfd_reg_enable(unsigned long canfd_base, int offset, u8 bit)
{
    u8 tmp = readb(canfd_base + offset);

    tmp |= bit;
    writeb(tmp, canfd_base + offset);
}

void canfd_reg_disable(unsigned long canfd_base, int offset, u8 bit)
{
    u8 tmp = readb(canfd_base + offset);

    tmp &= ~bit;
    writeb(tmp, canfd_base + offset);
}

void hal_canfd_clr_irq_flag(canfd_handle *phandle, int offset, u8 bit)
{
    u8 tmp = readb(phandle->canfd_base + offset);
    u8 mask = 0;

    switch (offset) {
    case CANFD_RTIF_REG:
        mask = CANFD_RTIE_ALL_MASK;
        break;
    case CANFD_ERRINT_REG:
        mask = CANFD_ERRINT_ALL_IRQ_FLAG;
        break;
    case CANFD_TTCFG_REG:
        mask = CANFD_TTCFG_ALL_IRQ_FLAG;
        break;
    default:
        pr_err("Failed to clear irq flag: invalid register.\n");
        return;
    }
    tmp &= ~mask;
    tmp |= bit;
    writeb(tmp, phandle->canfd_base + offset);
}

void hal_canfd_set_reset_mode(canfd_handle *phandle, canfd_reset_stat_t status)
{
    u32 val = 0;
    int ret;

    val = readb(phandle->canfd_base + CANFD_CFG_REG);

    if (status && (val & CANFD_CFG_RESET_FLAG))
        return;

    val &= ~CANFD_CFG_RESET_FLAG;
    val |= (status << CANFD_CFG_RESET_SHIFT);
    writeb(val, phandle->canfd_base + CANFD_CFG_REG);

    ret = val & CANFD_CFG_RESET_FLAG;
    while ((readb(phandle->canfd_base + CANFD_CFG_REG) & CANFD_CFG_RESET_FLAG) != ret) {
        continue;
    };
}

void hal_canfd_enable_int(canfd_handle *phandle, bool enable)
{
    if (enable) {
        /* set Warning Limit */
        canfd_reg_enable(phandle->canfd_base, CANFD_LIMIT_REG,
                         CANFD_LIMIT_AFWL_MASK | CANFD_LIMIT_EWL_MASK);
        /* Enable all error interrupts */
        canfd_reg_enable(phandle->canfd_base, CANFD_ERRINT_REG, CANFD_ERRINT_ALL_IE);
        hal_canfd_enable_interrupt(phandle);
        phandle->running = 1;
    } else {
        phandle->running = 0;
        hal_canfd_disable_interrupt(phandle);
        canfd_reg_disable(phandle->canfd_base, CANFD_ERRINT_REG, CANFD_ERRINT_ALL_IE);
        canfd_reg_disable(phandle->canfd_base, CANFD_LIMIT_REG,
                         CANFD_LIMIT_AFWL_MASK | CANFD_LIMIT_EWL_MASK);
    }
}

int hal_canfd_init(canfd_handle *phandle)
{
    int ret = 0;

    phandle->obtain_data_mode = CANFD_OBTAIN_DATA_BY_CPU;

    ret = hal_clk_enable_deassertrst(phandle->clk_id);
    if (ret < 0) {
        hal_log_err("CANFD clock and reset init error\n");
        return ret;
    }

    return ret;
}

void hal_canfd_uninit(canfd_handle *phandle)
{
    hal_clk_disable_assertrst(phandle->clk_id);
}

/* Configure CANFD bittiming, this function is called in reset mode only */
static int hal_canfd_set_bittiming(canfd_handle *phandle, struct aic_canfd_allbaud_info *bd)
{
    int ret;
    u32 reset_mode, bittiming_temp, data_bittiming_temp;

    hal_canfd_set_reset_mode(phandle, CANFD_ACT_RESET);
    reset_mode = readb(phandle->canfd_base + CANFD_CFG_REG);
    if ((reset_mode & CANFD_CFG_RESET_FLAG) == 0) {
        hal_log_err("Not in reset mode, cannot set bittiming\n");
        return -EINVAL;
    }

    bittiming_temp = ((bd->slow_baud.phase_seg1 - 2) << CANFD_SEG_1_SHIFT) |
             ((bd->slow_baud.phase_seg2 - 1) << CANFD_SEG_2_SHIFT) |
             ((bd->slow_baud.sjw - 1) << CANFD_SJW_SHIFT) |
             ((bd->slow_baud.brp - 1) << CANFD_PRESC_SHIFT);

    /*The input bittime setting is incorrect, should be correct*/
    ret = ((int)(bd->slow_baud.phase_seg1 - 2) < 0) || (((int)bd->slow_baud.phase_seg2 - 1) < 0) ||
          (((int)bd->slow_baud.sjw - 1) < 0) || (((int)(bd->slow_baud.brp) - 1) < 0);
    if (ret) {
        hal_log_err("slow bittime configuration is incorrect\n");
        return -1;
    }

    writel(bittiming_temp, phandle->canfd_base + CANFD_SSEG1_REG);

    if (bd->baud_type == CANFD_BAUD_FD) {
        data_bittiming_temp = ((bd->fast_baud.phase_seg1 - 2) << CANFD_SEG_1_SHIFT) |
                    ((bd->fast_baud.phase_seg2 - 1) << CANFD_SEG_2_SHIFT) |
                    ((bd->fast_baud.sjw - 1) << CANFD_SJW_SHIFT) |
                    ((bd->fast_baud.brp - 1) << CANFD_PRESC_SHIFT);

        ret = ((int)(bd->fast_baud.phase_seg1 - 2) < 0) || (((int)bd->fast_baud.phase_seg2 - 1) < 0) ||
            (((int)bd->fast_baud.sjw - 1) < 0) || (((int)bd->fast_baud.brp - 1) < 0);
        if (ret) {
            hal_log_err("fast bittime configuration is incorrect\n");
                return -1;
        }
        writel(data_bittiming_temp, phandle->canfd_base + CANFD_FSEG1_REG);
        canfd_reg_enable(phandle->canfd_base, CANFD_TDC_REG, bd->fast_baud.phase_seg1 + 1);
    };

    /*print configured slow and fast bit rate*/
    hal_log_debug("s_seg1 %#02x  s_seg2 %#02x  s_sjw %#02x  s_presc %#02x\n",
                 readb(phandle->canfd_base + CANFD_SSEG1_REG),
                 readb(phandle->canfd_base + CANFD_SSEG2_REG),
                 readb(phandle->canfd_base + CANFD_SSJW_REG),
                 readb(phandle->canfd_base + CANFD_SPRESC_REG)
                 );
    hal_log_debug("f_seg1 %#02x  f_seg2 %#02x  f_sjw %#02x  f_presc %#02x\n",
    readb(phandle->canfd_base + CANFD_FSEG1_REG), readb(phandle->canfd_base + CANFD_FSEG2_REG),
    readb(phandle->canfd_base + CANFD_FSJW_REG), readb(phandle->canfd_base + CANFD_FPRESC_REG));
    hal_canfd_set_reset_mode(phandle, CANFD_NO_RESET);

    return 0;
}

u32 hal_canfd_cal_seg_1(u32 n_tq, u32 duty_cycle)
{
    u32 temp1, temp2;
    u32 t_seg_1;

    t_seg_1 = n_tq * duty_cycle / 1000;
    //more precise to the target duty_cycle
    if (n_tq * (duty_cycle / 10) % 100) {
        temp1 = t_seg_1 * 10000 / n_tq;
        temp2 = (t_seg_1 + 1) * 10000 / n_tq;
        temp1 = abs(temp1 - duty_cycle * 10);
        temp2 = abs(temp2 - duty_cycle * 10);
        t_seg_1 = temp1 > temp2 ? t_seg_1 + 1 : t_seg_1;
    }

    return t_seg_1;
}

void hal_canfd_set_baudrate(canfd_handle *phandle, struct aic_canfd_baud_info *baud_info,
                            struct canfd_bittiming_const *bt_range)
{
    struct aic_canfd_baud_info *bd = baud_info;
    u32 mod_freq;
    u32 brp, n_tq, cur_n_tq, tseg1 = 0, tseg2 = 0;
    u32 temp1, temp2;
    u32 min_baud_diff = 0;  //minimun  baud rate error rate
    u32 min_duty_diff = 0;  //minimun  duty cycle error rate
    u32 cur_baud_diff = 0;  //current  baud rate error rate
    u32 cur_duty_diff = 0;  //current  duty cycle error rate
    u32 first_flag = 0;
    u32 sample_point_nominal = 0;

    if (bd->baudrate > 800000)
        sample_point_nominal = 750;
    else if (bd->baudrate > 500000)
        sample_point_nominal = 800;
    else
        sample_point_nominal = 875;

    mod_freq = hal_clk_get_freq(phandle->clk_id);
    hal_log_debug("mod_freq: %d\n", mod_freq);

    for (brp = 1; brp <= 256; brp++) /* brp loop from 1 to 256 */ {
        n_tq = (mod_freq / brp) / bd->baudrate;
        if( ((mod_freq / brp) % bd->baudrate) && n_tq ) {
            temp1 = (mod_freq / brp) / n_tq;
            temp2 = (mod_freq / brp) / (n_tq + 1);
            temp1 = abs(temp1 - bd->baudrate);
            temp2 = abs(temp2 - bd->baudrate);
            n_tq = temp1 > temp2 ? n_tq + 1 : n_tq;
        }

        tseg1 = hal_canfd_cal_seg_1(n_tq, sample_point_nominal);
        tseg2 = n_tq - tseg1;

        cur_n_tq = mod_freq / brp / bd->baudrate;
        if (tseg1 >= bt_range->tseg1_min && tseg1 <= bt_range->tseg1_max && tseg2 >= bt_range->tseg2_min && tseg2 <= bt_range->tseg2_max && tseg1 >= tseg2 + 1 && n_tq >= cur_n_tq) {
            cur_baud_diff = abs(((mod_freq / brp) / n_tq) - bd->baudrate);
            cur_duty_diff = abs(tseg1 * 10000 / n_tq - sample_point_nominal * 10);

            //first time to record the min baud rate and duty cycle error rate
            if (first_flag == 0) {
                min_baud_diff = cur_baud_diff;
                min_duty_diff = cur_duty_diff;
                first_flag = 1;
                bd->tq = n_tq;
                bd->prop_seg = 1;
                bd->phase_seg1 = tseg1;
                bd->phase_seg2 = tseg2;
                bd->brp = brp;
                bd->sjw = bd->phase_seg2;
            } else {
                if ((cur_baud_diff < min_baud_diff) || ((cur_baud_diff == min_baud_diff) && (min_duty_diff > cur_duty_diff))) {
                    min_baud_diff = cur_baud_diff;
                    min_duty_diff = cur_duty_diff;
                    bd->tq = n_tq;
                    bd->prop_seg = 1;
                    bd->phase_seg1 = tseg1;
                    bd->phase_seg2 = tseg2;
                    bd->brp = brp;
                    bd->sjw = bd->phase_seg2;
                }
            }
        }
    }

    return;
}

static int hal_canfd_set_run_mode(canfd_handle *phandle)
{
    switch (phandle->mode.run_mode) {
    case CANFD_RUNMODE_INTERNAL:
        canfd_reg_enable(phandle->canfd_base, CANFD_CFG_REG, CANFD_CFG_LBMI_FLAG);
        canfd_reg_disable(phandle->canfd_base, CANFD_CFG_REG, CANFD_CFG_LBME_FLAG);
        hal_log_debug("run mode is internal loopback %x\n",
        readb(phandle->canfd_base + CANFD_CFG_REG));
        break;
    case CANFD_RUNMODE_EXTERNAL:
        canfd_reg_enable(phandle->canfd_base, CANFD_CFG_REG, CANFD_CFG_LBME_FLAG);
        canfd_reg_enable(phandle->canfd_base, CANFD_RCTRL_REG, CANFD_RCTRL_SACK_FLAG);
        canfd_reg_disable(phandle->canfd_base, CANFD_CFG_REG, CANFD_CFG_LBMI_FLAG);
        hal_log_debug("run mode is external loopback %x\n",
        readb(phandle->canfd_base + CANFD_CFG_REG));
        break;
    default:
        canfd_reg_disable(phandle->canfd_base, CANFD_CFG_REG,
        CANFD_CFG_LBME_FLAG | CANFD_CFG_LBMI_FLAG);
        hal_log_debug("run mode is normal mode %x\n",
        readb(phandle->canfd_base + CANFD_CFG_REG));
        break;
    }

    return 0;
}

static int hal_canfd_set_xmit_mode(canfd_handle *phandle)
{
    switch (phandle->mode.tx_mode) {
    case CANFD_TXMODE_FULL:
        canfd_reg_enable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_TTTBM_FLAG);
        hal_log_debug("Full can mode\n");
        break;
    case CANFD_TXMODE_STB_FIFO:
        canfd_reg_disable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_TTTBM_FLAG);
        canfd_reg_disable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_TSMODE_FLAG);
        canfd_reg_enable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_FD_ISO_FLAG);
        hal_log_debug("FIFO mode\n");
        break;
    case CANFD_TXMODE_STB_PRIO:
        canfd_reg_disable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_TTTBM_FLAG);
        /* set TSMODE as 1->Priority mode */
        canfd_reg_enable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_TSMODE_FLAG);
        canfd_reg_enable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_FD_ISO_FLAG);
        hal_log_debug("Priority mode\n");
        break;
    case CANFD_TXMODE_PTB:
        canfd_reg_disable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_FD_ISO_FLAG);
        hal_log_debug("PTB mode\n");
        break;
    default:
        break;
    }
    hal_log_debug("CAN_FD_TCTRL:%x\n", readb(phandle->canfd_base + CANFD_TCTRL_REG));
    hal_log_debug("CAN_FD_TCMD:%x\n", readb(phandle->canfd_base + CANFD_TCMD_REG));

    return 0;
}

int canfd_tx_active(canfd_handle *phandle)
{
    unsigned long base = phandle->canfd_base;
    int ret = 0;

    hal_log_debug("reset:%d\n",
                  (readb(phandle->canfd_base + CANFD_CFG_REG) & CANFD_CFG_RESET_FLAG));

    if (phandle->mode.tx_mode == CANFD_TXMODE_PTB) {
        canfd_reg_enable(base, CANFD_CFG_REG, CANFD_CFG_TPSS_FLAG);
        if (phandle->mode.tx_type != CANFD_TXTYPE_TTCAN) {
            canfd_reg_enable(base, CANFD_TCMD_REG, CANFD_TCMD_TPE_FLAG);
        }
        return 0;
    }

    if (phandle->mode.tx_type == CANFD_TXTYPE_TSONE) {
        canfd_reg_enable(base, CANFD_TCMD_REG, CANFD_TCMD_TSONE_FLAG);
        hal_log_debug("CANFD_TXTYPE_TSONE:%x\n", readb(phandle->canfd_base + CANFD_TCMD_REG));
        return 0;
    }

    if (phandle->mode.tx_mode == CANFD_TXMODE_STB_FIFO ||
        phandle->mode.tx_mode == CANFD_TXMODE_STB_PRIO) {
        ret = readb(base + CANFD_TCMD_REG) & CANFD_TCMD_TSALL_FLAG;
        if (ret == 0)
            canfd_reg_enable(base, CANFD_TCMD_REG, CANFD_TCMD_TSALL_FLAG);
        hal_log_debug("CANFD_TXMODE_STB_FIFO:%x\n", readb(phandle->canfd_base + CANFD_TCMD_REG));
        return ret;
    }

    return 0;
}

static int hal_canfd_set_tx_mode(canfd_handle *phandle)
{
    if (phandle->mode.tx_type == CANFD_TXTYPE_TTCAN) {
        /* set TTTBM as 1->full TTCAN mode */
        canfd_reg_enable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_TTTBM_FLAG);
    } else {
        hal_canfd_set_xmit_mode(phandle);

        /*standby mode off*/
        canfd_reg_disable(phandle->canfd_base, CANFD_TCMD_REG, CANFD_TCMD_STBY_FLAG);

        if (phandle->mode.tx_mode == CANFD_TXMODE_PTB) {
            canfd_reg_disable(phandle->canfd_base, CANFD_TCMD_REG, CANFD_TCMD_TBSEL_FLAG);
        } else {
            canfd_reg_enable(phandle->canfd_base, CANFD_TCMD_REG, CANFD_TCMD_TBSEL_FLAG);
        }

        // shall be switched only if the STB if empty
        if (readb(phandle->canfd_base + CANFD_TCTRL_REG) & CANFD_TCTRL_TSSTAT_MASK) {
            hal_log_err("cannot modify fifo mode, wait...\n");
            while ((readb(phandle->canfd_base + CANFD_TCTRL_REG) & CANFD_TCTRL_TSSTAT_MASK) != 0) {
                continue;
            };
        }

        if (phandle->mode.tx_mode == CANFD_TXMODE_STB_FIFO) {
            canfd_reg_disable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_TSMODE_FLAG);
        } else {
            /* set TSMODE as 1->Priority mode */
            canfd_reg_enable(phandle->canfd_base, CANFD_TCTRL_REG, CANFD_TCTRL_TSMODE_FLAG);
        }
    }

    hal_log_debug("CAN_FD_TCTRL:%x\n", readb(phandle->canfd_base + CANFD_TCTRL_REG));
    hal_log_debug("CAN_FD_TCMD:%x\n", readb(phandle->canfd_base + CANFD_TCMD_REG));

    return 0;
}

static void hal_canfd_bus_error_msg(canfd_handle *phandle)
{
    u8 i;
    u8 errinfo = readb(phandle->canfd_base + CANFD_EALCAP_REG);
    u8 errtype = (errinfo & CANFD_EALCAP_KOER_MASK) >> CANFD_EALCAP_KOER_SHIFT;

    for (i = 0; i < ARRAY_SIZE(bus_err_type); i++) {
        if (errtype == bus_err_type[i].code) {
            hal_log_debug("%s, ", bus_err_type[i].msg);
            switch (i) {
            case 1:
                phandle->status.biterrcnt++;
                break;
            case 2:
                phandle->status.formaterrcnt++;
                break;
            case 3:
                phandle->status.stufferrcnt++;
                break;
            case 4:
                phandle->status.ackerrcnt++;
                break;
            case 5:
                phandle->status.crcerrcnt++;
                break;
            default:
                phandle->status.othererrcnt++;
                break;
            }
            break;
        }
    }
}

static void hal_canfd_arblost_msg(canfd_handle *phandle)
{
    u8 i;
    u8 arbinfo = readb(phandle->canfd_base + CANFD_EALCAP_REG) & CANFD_EALCAP_ALC_MASK;

    for (i = 0; i < ARRAY_SIZE(bus_arb_lost); i++) {
        if (arbinfo == bus_arb_lost[i].code) {
            hal_log_debug("%s, ", bus_arb_lost[i].msg);
            phandle->status.arblostcnt++;
            phandle->status.snderrcnt++;
            break;
        }
    }
}

static void hal_canfd_error_handle(canfd_handle *phandle, u32 err_status)
{
    u32 can_status;

    can_status = readl(phandle->canfd_base + CANFD_CFG_REG);
    phandle->status.rxerrcnt = readb(phandle->canfd_base + CANFD_RECNT_REG);
    phandle->status.txerrcnt = readb(phandle->canfd_base + CANFD_TECNT_REG);

    if (err_status & CANFD_ERRINT_BEIF_FLAG)
        hal_canfd_bus_error_msg(phandle);

    if (err_status & CANFD_ERRINT_ALIF_FLAG)
        hal_canfd_arblost_msg(phandle);

    if (err_status & CANFD_ERRINT_EPIF_FLAG) {
        // Error Passive Interrupt
        if (phandle->status.current_state == CANFD_STATE_PASSIVE)
            phandle->status.current_state = CANFD_STATE_WARNING;
        else
            phandle->status.current_state = CANFD_STATE_PASSIVE;
    }

    if (can_status & CANFD_CFG_BUSOFF_FLAG)
        phandle->status.current_state = CANFD_STATE_BUS_OFF;

    if (err_status & CANFD_ERRINT_EWARN_FLAG)
        phandle->status.current_state = CANFD_STATE_WARNING;
    else
        phandle->status.current_state = CANFD_STATE_ACTIVE;
}

int hal_canfd_attach_callback(canfd_handle *phandle, void *callback, void *arg)
{
    CHECK_PARAM(phandle != NULL, -EINVAL);

    phandle->callback = callback;
    phandle->arg = arg;
    return 0;
}

void hal_canfd_detach_callback(canfd_handle *phandle)
{
    CHECK_PARAM_RET(phandle);

    phandle->callback = NULL;
    phandle->arg = NULL;
}

void hal_canfd_rx_frame(u32 reg_base, canfd_msg_t *msg)
{
    u32 i, temp;
    u32 rbuf1_val;
    u8 rx_status = 0;
    unsigned long canfd_base = reg_base;

    CHECK_PARAM_RET(msg);

    rx_status = readb(canfd_base + CANFD_RCTRL_REG);
    if (!(rx_status & CANFD_RCTRL_RSTAT_MASK))
        return;

    rbuf1_val = readl(canfd_base + CANFD_RBUF1_REG);
    msg->ide = (rbuf1_val >> CANFD_RBUF1_IDE_SHIFT) & 1;
    msg->rtr = (rbuf1_val >> CANFD_RBUF1_RTR_SHIFT) & 1;
    msg->fdf = (rbuf1_val >> CANFD_RBUF1_FDF_SHIFT) & 1;
    msg->dlc = rbuf1_val & CANFD_RBUF1_DLC_MASK;

    if (msg->ide) {
        /* extended frame */
        msg->id = readl(canfd_base + CANFD_RBUF0_REG) & CANFD_RBUF0_ID_EXT_MASK;
    } else {
        /* standard frame */
        msg->id = readl(canfd_base + CANFD_RBUF0_REG) & CANFD_RBUF0_ID_STAND_MASK;
    }

    if (!msg->rtr)
        for (i = 0; i < hal_canfd_dlc2len(msg->dlc); i += 4) {
            temp = readl(canfd_base + CANFD_RBUF0_REG + 8 + i);
            msg->data[i] = temp & 0xff;
            msg->data[i + 1] = temp >> 8 & 0xff;
            msg->data[i + 2] = temp >> 16 & 0xff;
            msg->data[i + 3] = temp >> 24 & 0xff;
        }

    canfd_reg_enable(canfd_base, CANFD_RCTRL_REG, CANFD_RCTRL_RREL_FLAG);
    return;
}

void hal_canfd_tx_frame(canfd_handle *phandle, canfd_msg_t * msg)
{
    CHECK_PARAM_RET(phandle);
    CHECK_PARAM_RET(msg);
    u32 i;
    unsigned long canfd_base = phandle->canfd_base;
    u8 ide = msg->ide;
    u8 rtr = msg->rtr;
    u8 dlc = msg->dlc;
    u32 tbuf1_val = dlc;
    u32 id;
    u8 len = 0;
    u32 temp = 0;

    if (rtr)
        tbuf1_val |= CANFD_TBUF1_RTR_FLAG;

    if (ide) {
        /* extended frame */
        tbuf1_val |= CANFD_TBUF1_IDE_FLAG;
        id = msg->id & CANFD_EFF_MASK;
    } else {
        /* standard frame */
        id = msg->id & CANFD_SFF_MASK;
    }

    if (msg->fdf == CANFD_TYPE) {
        tbuf1_val |= CANFD_TBUF1_FDF_FLAG;
        tbuf1_val |= CANFD_TBUF1_BRS_FLAG;
    }

    if (phandle->obtain_data_mode == CANFD_OBTAIN_DATA_BY_CPU) {
        writel(id | CANFD_TBUF0_TTSEN_FLAG, canfd_base + CANFD_TBUF0_REG);
        writel(tbuf1_val, canfd_base + CANFD_TBUF1_REG);
    } else if (phandle->obtain_data_mode == CANFD_OBTAIN_DATA_BY_DMA) {
        ((u32 *)phandle->dma_tx_info.buf)[0] = id | CANFD_TBUF0_TTSEN_FLAG;
        ((u32 *)phandle->dma_tx_info.buf)[1] = tbuf1_val;
        hal_log_debug("dma_tx_info.buf%p\n",(u32 *)phandle->dma_tx_info.buf);
    }

    if (rtr == 0) {
        len = hal_canfd_dlc2len(dlc);

        for (i = 0; i < (len / 4) + (len % 4); i++) {
            temp = (msg->data[i * 4] | (msg->data[i * 4 + 1] << 8) |
                   (msg->data[i * 4 + 2] << 16) | (msg->data[i * 4 + 3] << 24));
            if (phandle->obtain_data_mode == CANFD_OBTAIN_DATA_BY_CPU) {
                writel(temp, canfd_base + CANFD_TBUF0_REG + 8 + i * 4);
                hal_log_debug("[%d] %x\n", i, temp);
            } else {
                ((u32 *)phandle->dma_tx_info.buf)[i + 2] = temp;
                hal_log_debug("[%d] %x\n", i, ((u32 *)phandle->dma_tx_info.buf)[i + 2]);
            }
        }
    }
}

static void hal_canfd_tx_interrupt(canfd_handle *phandle, u8 isr)
{
    while (isr & (CANFD_RTIF_TPIF_FLAG | CANFD_RTIF_TSIF_FLAG)) {
        if (isr & CANFD_RTIF_TPIF_FLAG)
            hal_canfd_clr_irq_flag(phandle, CANFD_RTIF_REG, CANFD_RTIF_TPIF_FLAG);
        if (isr & CANFD_RTIF_TSIF_FLAG)
            hal_canfd_clr_irq_flag(phandle, CANFD_RTIF_REG, CANFD_RTIF_TSIF_FLAG);
        isr = readb(phandle->canfd_base + CANFD_RTIF_REG);
    }
}

irqreturn_t hal_canfd_isr_handler(int irq_num, void *arg)
{
    u8 rtif_sta, errint_sta, tcan;
    int temp_val;
    int rtif_handle = 0;
    int errint_handle = 0;
    canfd_handle *phandle = (canfd_handle *)arg;

    rtif_sta = readb(phandle->canfd_base + CANFD_RTIF_REG);
    errint_sta = readb(phandle->canfd_base + CANFD_ERRINT_REG);
    hal_log_debug("rtif_sta%#x errint_sta%#x\n", rtif_sta, errint_sta);

    /* check for tx interrupt and processing it */
    if (rtif_sta & (CANFD_RTIF_TPIF_FLAG | CANFD_RTIF_TSIF_FLAG)) {
        hal_log_debug("tx interrupt\n");
        phandle->status.sndpkgcnt++;
        hal_canfd_tx_interrupt(phandle, rtif_sta);
        if (phandle->callback)
            phandle->callback(phandle, (void *)CAN_EVENT_TX_DONE);
        rtif_handle |= (CANFD_RTIF_TPIF_FLAG | CANFD_RTIF_TSIF_FLAG);
    }

    /* check for abort interrupt and processing it */
    if (rtif_sta & CANFD_RTIF_AIF_FLAG) {
        hal_canfd_clr_irq_flag(phandle, CANFD_RTIF_REG, CANFD_RTIF_AIF_FLAG);
        rtif_handle |= CANFD_RTIF_AIF_FLAG;
    }

    if (rtif_sta & (CANFD_RTIF_RAFIF_FLAG | CANFD_RTIF_RFIF_FLAG  | CANFD_RTIF_ROIF_FLAG)) {
        temp_val = CANFD_RTIF_RAFIF_FLAG | CANFD_RTIF_RFIF_FLAG | CANFD_RTIF_ROIF_FLAG;
        hal_canfd_clr_irq_flag(phandle, CANFD_RTIF_REG, temp_val);
        rtif_handle |= (CANFD_RTIF_RAFIF_FLAG | CANFD_RTIF_RFIF_FLAG | CANFD_RTIF_ROIF_FLAG);
        if (phandle->callback)
            phandle->callback(phandle, (void *)CAN_EVENT_RXOF_IND);
        phandle->status.othererrcnt++;
        phandle->status.recverrcnt++;
    }

    tcan = readb(phandle->canfd_base + CANFD_TTCFG_REG);
    if (tcan & CANFD_TTCFG_TTIE_FLAG)
        hal_canfd_clr_irq_flag(phandle, CANFD_TTCFG_REG, CANFD_TTCFG_TTIE_FLAG);
    if (tcan & CANFD_TTCFG_TEIF_FLAG)
        hal_canfd_clr_irq_flag(phandle, CANFD_TTCFG_REG, CANFD_TTCFG_TEIF_FLAG);

    /*check for rx interrupt and processing it*/
    if (rtif_sta & CANFD_RTIF_RIF_FLAG) {
#ifdef AIC_CANFD_GET_DATA_BY_CPU
        hal_canfd_clr_irq_flag(phandle, CANFD_RTIF_REG, CANFD_RTIF_RIF_FLAG);
        hal_canfd_rx_frame(phandle->canfd_base, &phandle->msg);
        if (phandle->callback)
            phandle->callback(phandle, (void *)CAN_EVENT_RX_IND);
        rtif_handle |= CANFD_RTIF_RIF_FLAG;
#endif
#ifdef AIC_CANFD_GET_DATA_BY_DMA
        hal_canfd_config_dma_rx(phandle);
#endif
        hal_canfd_clr_irq_flag(phandle, CANFD_RTIF_REG, CANFD_RTIF_RIF_FLAG);
    }

    if (rtif_sta & CANFD_RTIF_EIF_FLAG) {
        hal_log_err("error interrupt\n");
        hal_canfd_clr_irq_flag(phandle, CANFD_RTIF_REG, CANFD_RTIF_EIF_FLAG);
        rtif_handle |= CANFD_RTIF_EIF_FLAG;
    }

    temp_val = CANFD_ERRINT_EPIF_FLAG | CANFD_ERRINT_BEIF_FLAG |
               CANFD_ERRINT_EWARN_FLAG | CANFD_ERRINT_ALIF_FLAG;
    if (errint_sta & temp_val) {
        hal_log_err("tx failed\n");
        hal_canfd_clr_irq_flag(phandle, CANFD_ERRINT_REG, errint_sta & temp_val);
        hal_canfd_error_handle(phandle, errint_sta);
        errint_handle |= temp_val;
        if (phandle->callback)
            phandle->callback(phandle, (void *)CAN_EVENT_TX_FAIL);
    }

    if (rtif_handle == 0 && errint_handle == 0) {
        return IRQ_NONE;
    }

    return IRQ_HANDLED;
}

static int hal_can_get_baudrate(canfd_handle *phandle, struct aic_canfd_baud_info *baud_info)
{
    u8 brp, tseg1, tseg2;
    u32 mod_freq;

    CHECK_PARAM(phandle, -EINVAL);
    CHECK_PARAM(baud_info, -EINVAL);

    mod_freq = hal_clk_get_freq(phandle->clk_id);
    brp = readb(phandle->canfd_base + CANFD_SPRESC_REG);
    tseg1 = readb(phandle->canfd_base + CANFD_SSEG1_REG);
    tseg2 = readb(phandle->canfd_base + CANFD_SSEG2_REG);

    baud_info->baudrate = (mod_freq / (brp + 1)) / (tseg1 + tseg2);
    baud_info->duty = tseg1 /(tseg1 +tseg2);
    return 0;
}


s32 hal_canfd_filter_cfg(canfd_handle *phandle, u8 acfadr, u8 selmask, u32 val)
{
    u8 temp = 0;

    temp = acfadr | ((selmask & 0x01) << 5);
    writel(temp, phandle->canfd_base + CANFD_ACFCTRL_REG);
    writel(val, phandle->canfd_base + CANFD_ACODEx_REG);

    hal_log_debug("acfctrl:%x\n", readb(phandle->canfd_base + CANFD_ACFCTRL_REG));
    hal_log_debug("acf:%x\n", readl(phandle->canfd_base + CANFD_ACODEx_REG));
    return 0;
}

s32 hal_canfd_filter_en(canfd_handle *phandle, u8 acfadr, u8 en)
{
    u32 addr, val, pos;
    if (acfadr >= CANFD_ACF_NUM) {
        return -1;
    }
    if (acfadr < 8) {
        addr = CANFD_EN0_REG;
        pos  = acfadr;
    } else {
        addr = CANFD_EN1_REG;
        pos  = acfadr - 8;
    }
    val = readb(phandle->canfd_base + addr);
    val &= ~(0x01 << pos);
    val |= ((en & 0x01) << pos);
    canfd_reg_enable(phandle->canfd_base, addr, val);

    return 0;
}

static int hal_canfd_set_filter(canfd_handle *phandle, canfd_filter_config_t *cfg)
{
    u32 i, reset_mode;

    CHECK_PARAM(phandle, -EINVAL);

    hal_canfd_set_reset_mode(phandle, CANFD_ACT_RESET);
    reset_mode = readb(phandle->canfd_base + CANFD_CFG_REG);
    if ((reset_mode & CANFD_CFG_RESET_FLAG) == 0) {
        hal_log_err("Not in reset mode, cannot set filter\n");
        return -EINVAL;
    }

    for (i = 0; i < cfg->filter_chan; i++) {
        hal_canfd_filter_cfg(phandle, i, CANFD_FILTER_CODE, cfg->items[i].id);
        hal_canfd_filter_cfg(phandle, i, CANFD_FILTER_MASK, cfg->items[i].mask);
        hal_canfd_filter_en(phandle, i, 1);
    }
    while (i < AIC_CANFD_FILTER_MAX_CNT) {
        hal_canfd_filter_en(phandle, i, 0);
        i++;
    }

    hal_canfd_set_reset_mode(phandle, CANFD_NO_RESET);
    hal_log_info("Set filter finish!\n");

    return 0;
}

#ifdef AIC_CANFD_GET_DATA_BY_DMA
void hal_canfd_stop_dma(canfd_handle *phandle)
{
    hal_dma_chan_stop(phandle->dma_rx_info.dma_chan);
    hal_release_dma_chan(phandle->dma_rx_info.dma_chan);
}

void hal_canfd_set_dma_request(canfd_handle *phandle, u32 frame_data_len)
{
    u32 val = 0;
    u32 frame_head_len = 8;

    val |= CANFD_DMAC_DST_EN_FLAG;
    val |= (frame_data_len + frame_head_len) << CANFD_DST_EN_SHIFT;
    val |= CANFD_SCR_EN_FLAG << CANFD_SCR_EN_SHIFT;
    val |= (frame_data_len + frame_head_len) << CANFD_SCR_THR_SHIFT;

    writel(val, phandle->canfd_base + CANFD_DMAC_REG);
}


/* INTERRUPT Context */
static void hal_canfd_dma_tx_callback(void *arg)
{
    canfd_handle *phandle = {0};
    struct canfd_dma_transfer_info *tx_info = {0};

    void *dma_cb_data = NULL;
    dma_callback dma_cb = NULL;
    phandle = (struct canfd_handle *)arg;
    tx_info = &phandle->dma_tx_info;

    if (phandle->mode.tx_type != CANFD_TXTYPE_TTCAN) {
        canfd_tx_active(phandle);
    } else {
        canfd_reg_enable(phandle->canfd_base, CANFD_TCMD_REG, CANFD_TCMD_TSALL_FLAG);
    }

    dma_cb = phandle->dma_tx_info.callback;
    dma_cb_data = phandle->dma_tx_info.callback_param;
    if (dma_cb) {
        dma_cb(dma_cb_data);
    }

    hal_dma_chan_stop(tx_info->dma_chan);
    hal_release_dma_chan(tx_info->dma_chan);
}

void hal_canfd_config_dma_tx(canfd_handle *phandle)
{
    struct dma_slave_config config = {0};
    struct canfd_dma_transfer_info *tx_info;

    tx_info = &phandle->dma_tx_info;

    config.direction = DMA_MEM_TO_DEV;
    config.dst_addr = phandle->canfd_base + CANFD_TBUF0_REG;
    config.slave_id = DMA_ID_CANFD0;
    config.dst_maxburst = 18;
    config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    config.src_addr = (ulong)tx_info->buf;
    config.src_maxburst = 18;

    tx_info->dma_chan = hal_request_dma_chan();
    if (!tx_info->dma_chan) {
        hal_log_err("DMA request dma channel error\n");
        return;
    }
    hal_dma_chan_register_cb(tx_info->dma_chan, hal_canfd_dma_tx_callback,
                             (void *)phandle);
    hal_dma_chan_config(tx_info->dma_chan, &config);
    hal_dma_chan_prep_device(tx_info->dma_chan, config.dst_addr,
                             (ulong)tx_info->buf, tx_info->buf_size,
                             DMA_MEM_TO_DEV);
    hal_dma_chan_start(phandle->dma_tx_info.dma_chan);
    return;
}

void hal_canfd_dma_rx_frame(u32 reg_base, void *buf, canfd_msg_t *msg) {
    u32 rbuf1_val;
    unsigned long canfd_base = reg_base;

    CHECK_PARAM_RET(msg);

    rbuf1_val = *(u32*)(buf + 4);
    msg->ide = (rbuf1_val >> CANFD_RBUF1_IDE_SHIFT) & 1;
    msg->rtr = (rbuf1_val >> CANFD_RBUF1_RTR_SHIFT) & 1;
    msg->fdf = (rbuf1_val >> CANFD_RBUF1_FDF_SHIFT) & 1;
    msg->dlc = rbuf1_val & CANFD_RBUF1_DLC_MASK;

    if (msg->ide) {
        /* extended frame */
        msg->id = *(u32*)(buf) & CANFD_RBUF0_ID_EXT_MASK;
    } else {
        /* standard frame */
        msg->id =  *(u32*)(buf) & CANFD_RBUF0_ID_STAND_MASK;
    }

    canfd_reg_enable(canfd_base, CANFD_RCTRL_REG, CANFD_RCTRL_RREL_FLAG);
    return;
}

static void hal_canfd_dma_rx_callback(void *arg)
{
    canfd_handle *phandle = {0};
    struct canfd_dma_transfer_info *rx_info = {0};

    void *dma_cb_data = NULL;
    dma_callback dma_cb = NULL;
    phandle = (struct canfd_handle *)arg;
    rx_info = &phandle->dma_rx_info;

    dma_cb = phandle->dma_rx_info.callback;
    dma_cb_data = phandle->dma_rx_info.callback_param;
    if (dma_cb) {
        dma_cb(dma_cb_data);
    }

    aicos_dcache_invalid_range(phandle->dma_rx_info.buf, phandle->dma_rx_info.buf_size);
    hal_canfd_dma_rx_frame(phandle->canfd_base, phandle->dma_rx_info.buf, &phandle->msg);
    if (phandle->callback) {
        phandle->callback(phandle, (void *)CAN_EVENT_RX_IND);
    }

    hal_dma_chan_stop(rx_info->dma_chan);
    hal_release_dma_chan(rx_info->dma_chan);
}

void hal_canfd_config_dma_rx(canfd_handle *phandle)
{
    struct dma_slave_config config = {0};
    struct canfd_dma_transfer_info *rx_info;

    rx_info = &phandle->dma_rx_info;

    config.direction = DMA_DEV_TO_MEM;
    config.slave_id = DMA_ID_CANFD0;
    config.src_addr = phandle->canfd_base + CANFD_RBUF0_REG;
    config.src_maxburst = 18;
    config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    config.dst_addr = (ulong)rx_info->buf;
    config.dst_maxburst = 18;

    hal_canfd_set_dma_request(phandle, 16);

    rx_info->dma_chan = hal_request_dma_chan();
    if (!rx_info->dma_chan) {
        hal_log_err("DMA request dma channel error\n");
        return;
    }
    hal_dma_chan_register_cb(rx_info->dma_chan, hal_canfd_dma_rx_callback,
                             (void *)phandle);
    hal_dma_chan_config(rx_info->dma_chan, &config);
    hal_dma_chan_prep_device(rx_info->dma_chan, (ulong)rx_info->buf,
                             config.src_addr, rx_info->buf_size,
                             DMA_DEV_TO_MEM);
    hal_dma_chan_start(phandle->dma_rx_info.dma_chan);
    return;
}
#endif

int hal_canfd_ioctl(canfd_handle *phandle, int cmd, void *arg)
{
    int ret = 0;
    struct aic_canfd_allbaud_info *baud_info = {0};
    canfd_filter_config_t *cfg = {0};

    switch (cmd) {
    case CANFD_IOCTL_SET_MODE:
        hal_canfd_set_run_mode(phandle);
        hal_canfd_set_tx_mode(phandle);
        break;
    case CANFD_IOCTL_SET_BAUDRATE:
        baud_info = (struct aic_canfd_allbaud_info *)arg;
        phandle->can_type = baud_info->baud_type;
        phandle->s_baud = baud_info->slow_baud.baudrate;
        phandle->f_baud = baud_info->fast_baud.baudrate;

        if (baud_info->baud_type == CANFD_BAUD_FD) {
            hal_canfd_set_baudrate(phandle, &baud_info->slow_baud, &btc_canfd);
            hal_canfd_set_baudrate(phandle, &baud_info->fast_baud, &dbtc_canfd);
        } else {
            hal_canfd_set_baudrate(phandle, &baud_info->slow_baud, &btc_can);
        }
        hal_canfd_set_bittiming(phandle, baud_info);
        break;
    case CANFD_IOCTL_GET_BAUDRATE:
        ret = hal_can_get_baudrate(phandle, (struct aic_canfd_baud_info *)arg);
        break;
    case CANFD_IOCTL_SET_FILTER:
        cfg = (canfd_filter_config_t *)arg;
        ret = hal_canfd_set_filter(phandle, cfg);
        break;
    case CANFD_IOCTL_RELEASE_MODE:
        break;
    default:
        return -EOPNOTSUPP;
    }

    return ret;
}
