/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "aic_hal_reset.h"
#include "hal_pbus.h"

/* Register of PBUS */
#define PBUS_CFG0       (PBUS_BASE + 0x000)
#define PBUS_CFG1       (PBUS_BASE + 0x004)
#define PBUS_CFG2       (PBUS_BASE + 0x008)
#define PBUS_VERSION    (PBUS_BASE + 0xFFC)
#define PBUS_DATA_BASE  (PBUS_BASE + 0x10000)

#define PBUS_OUTENABLE_POL          BIT(11)
#define PBUS_WRENABLE_POL           BIT(10)
#define PBUS_ADDRVALID_POL          BIT(9)
#define PBUS_CS_POL                 BIT(8)
#define PBUS_BUSCLK_POL             BIT(5)
#define PBUS_BUSCLK_OUTENABLE       BIT(4)
#define PBUS_BUSCLK_DIV_SHIFT       0
#define PBUS_BUSCLK_DIV_MASK        GENMASK(1, 0)

#define PBUS_WRDATA_HOLDTIME_SHIFT  28
#define PBUS_WRDATA_HOLDTIME_MASK   GENMASK(31, 28)
#define PBUS_WRDATA_DELAYTIME_SHIFT 24
#define PBUS_WRDATA_DELAYTIME_MASK  GENMASK(27, 24)
#define PBUS_ADDR_HOLDTIME_SHIFT    20
#define PBUS_ADDR_HOLDTIME_MASK     GENMASK(23, 20)
#define PBUS_ADDR_DELAYTIME_SHIFT   16
#define PBUS_ADDR_DELAYTIME_MASK    GENMASK(19, 16)
#define PBUS_CS_HOLDTIME_SHIFT      8
#define PBUS_CS_HOLDTIME_MASK       GENMASK(12, 8)
#define PBUS_CS_DELAYTIME_SHIFT     0
#define PBUS_CS_DELAYTIME_MASK      GENMASK(4, 0)

#define PBUS_OUTENABLE_HOLDTIME_SHIFT   20
#define PBUS_OUTENABLE_HOLDTIME_MASK    GENMASK(23, 20)
#define PBUS_OUTENABLE_DELAYTIME_SHIFT  16
#define PBUS_OUTENABLE_DELAYTIME_MASK   GENMASK(19, 16)
#define PBUS_WRRD_HOLDTIME_SHIFT        12
#define PBUS_WRRD_HOLDTIME_MASK         GENMASK(15, 12)
#define PBUS_WRRD_DELAYTIME_SHIFT       8
#define PBUS_WRRD_DELAYTIME_MASK        GENMASK(11, 8)
#define PBUS_ADDRVALID_HOLDTIME_SHIFT   4
#define PBUS_ADDRVALID_HOLDTIME_MASK    GENMASK(7, 4)
#define PBUS_ADDRVALID_DELAYTIME_SHIFT  0
#define PBUS_ADDRVALID_DELAYTIME_MASK   GENMASK(3, 0)

#define PBUS_BUS_WIDTH                  2
#define IS_UNALIGNED(n)                 (!IS_ALIGNED((n), PBUS_BUS_WIDTH))

#define PBUS_SET_BIT(val, bit, n)   do { \
            if (bit) \
                (val) |= (n); \
            else \
                (val) &= ~(n); \
        } while (0)
#define PBUS_SET_BITS(val, bits, NAME)  do { \
            (val) &= ~NAME##_MASK; \
            (val) |= (bits) << NAME##_SHIFT; \
        } while (0)
#define PBUS_GET_BIT(val, n)     ((val) & (n) ? 1 : 0)
#define PBUS_GET_BITS(val, NAME) ((u32)(((val) & NAME##_MASK) >> NAME##_SHIFT))

static struct aic_pbus_cfg g_pbus_cfg = {0};

static void hal_pbus_set_cfg0(struct aic_pbus_cfg *cfg)
{
    u32 val = readl(PBUS_CFG0);

    PBUS_SET_BIT(val, g_pbus_cfg.out_enable_pol, PBUS_OUTENABLE_POL);
    PBUS_SET_BIT(val, g_pbus_cfg.wr_enable_pol, PBUS_WRENABLE_POL);
    PBUS_SET_BIT(val, g_pbus_cfg.addr_valid_pol, PBUS_ADDRVALID_POL);
    PBUS_SET_BIT(val, g_pbus_cfg.cs_pol, PBUS_CS_POL);
    PBUS_SET_BIT(val, g_pbus_cfg.bus_clk_pol, PBUS_BUSCLK_POL);

    PBUS_SET_BIT(val, g_pbus_cfg.bus_clk_enable, PBUS_BUSCLK_OUTENABLE);
    PBUS_SET_BITS(val, g_pbus_cfg.bus_clk_div, PBUS_BUSCLK_DIV);

    writel(val, PBUS_CFG0);
    pr_debug("Set CFG0: %#x\n", val);
}

static void hal_pbus_set_cfg1(struct aic_pbus_cfg *cfg)
{
    u32 val = readl(PBUS_CFG1);

    PBUS_SET_BITS(val, g_pbus_cfg.wr_data.holdtime, PBUS_WRDATA_HOLDTIME);
    PBUS_SET_BITS(val, g_pbus_cfg.wr_data.delaytime, PBUS_WRDATA_DELAYTIME);

    PBUS_SET_BITS(val, g_pbus_cfg.addr.holdtime, PBUS_ADDR_HOLDTIME);
    PBUS_SET_BITS(val, g_pbus_cfg.addr.delaytime, PBUS_ADDR_DELAYTIME);

    PBUS_SET_BITS(val, g_pbus_cfg.cs.holdtime, PBUS_CS_HOLDTIME);
    PBUS_SET_BITS(val, g_pbus_cfg.cs.delaytime, PBUS_CS_DELAYTIME);

    writel(val, PBUS_CFG1);
    pr_debug("Set CFG1: %#x\n", val);
}

static void hal_pbus_set_cfg2(struct aic_pbus_cfg *cfg)
{
    u32 val = readl(PBUS_CFG2);

    PBUS_SET_BITS(val, g_pbus_cfg.out_enable.holdtime, PBUS_OUTENABLE_HOLDTIME);
    PBUS_SET_BITS(val, g_pbus_cfg.out_enable.delaytime, PBUS_OUTENABLE_DELAYTIME);

    PBUS_SET_BITS(val, g_pbus_cfg.wr_rd.holdtime, PBUS_WRRD_HOLDTIME);
    PBUS_SET_BITS(val, g_pbus_cfg.wr_rd.delaytime, PBUS_WRRD_DELAYTIME);

    PBUS_SET_BITS(val, g_pbus_cfg.addr_valid.holdtime, PBUS_ADDRVALID_HOLDTIME);
    PBUS_SET_BITS(val, g_pbus_cfg.addr_valid.delaytime, PBUS_ADDRVALID_DELAYTIME);

    writel(val, PBUS_CFG2);
    pr_debug("Set CFG2: %#x\n", val);
}

int hal_pbus_get_cfg(struct aic_pbus_cfg *cfg)
{
    if (!cfg)
        return -1;

    memcpy(cfg, &g_pbus_cfg, sizeof(struct aic_pbus_cfg));
    return 0;
}

int hal_pbus_set_cfg(struct aic_pbus_cfg *cfg)
{
    if (!cfg)
        return -1;

    memcpy(&g_pbus_cfg, cfg, sizeof(struct aic_pbus_cfg));

    hal_pbus_set_cfg0(cfg);
    hal_pbus_set_cfg1(cfg);
    hal_pbus_set_cfg2(cfg);
    return 0;
}

static void hal_pbus_gen_default_cfg(void)
{
    g_pbus_cfg.bus_clk_div = AIC_PBUS_CLK_DIV_2;

    g_pbus_cfg.wr_data.holdtime = 2;
    g_pbus_cfg.wr_data.delaytime = 6;
    g_pbus_cfg.addr.holdtime = 3;
    g_pbus_cfg.addr.delaytime = 1;
    g_pbus_cfg.cs.holdtime = 8;
    g_pbus_cfg.cs.delaytime = 2;
    g_pbus_cfg.out_enable.holdtime = 2;
    g_pbus_cfg.out_enable.delaytime = 5;
    g_pbus_cfg.wr_rd.holdtime = 6;
    g_pbus_cfg.wr_rd.delaytime = 1;
    g_pbus_cfg.addr_valid.holdtime = 2;
    g_pbus_cfg.addr_valid.delaytime = 1;
}

int hal_pbus_init(void)
{
    if (hal_clk_enable(CLK_PBUS)) {
        pr_err("PBUS clk enable failed!");
        return -1;
    }

    if (hal_reset_deassert(RESET_PBUS)) {
        pr_err("PBUS reset deassert failed.");
        return -1;
    }

    hal_pbus_gen_default_cfg();
    return 0;
}

int hal_pbus_deinit(void)
{
    if (hal_reset_assert(RESET_PBUS)) {
        pr_err("PBUS reset assert failed.");
        return -1;
    }

    if (hal_clk_disable(CLK_PBUS)) {
        pr_err("PBUS clk disable failed!");
        return -1;
    }

    return 0;
}

static u16 hal_pbus_read_16bit(u32 offset)
{
    return *(u16 *)(PBUS_DATA_BASE + offset);
}

static void hal_pbus_write_16bit(u32 offset, u16 val)
{
    *(u16 *)(PBUS_DATA_BASE + offset) = val;
}

int hal_pbus_read(u32 offset, u8 *buf, u32 len)
{
    u32 remain = len;
    u32 pos = offset;
    u8 *ptr = buf;

    if (!buf || !len)
        return -1;

    if (len + offset > AIC_PBUS_SIZE) {
        pr_err("Invalid data length: %d + %d\n", offset, len);
        return -1;
    }

    if (IS_UNALIGNED(offset)) {
        *ptr++ = hal_pbus_read_16bit(ALIGN_DOWN(offset, 2)) >> 8;
        remain -= 1;
        pos += 1;
    }

    if (IS_UNALIGNED(remain)) {
        remain = ALIGN_DOWN(remain, 2);
        memcpy(ptr, (void *)(PBUS_DATA_BASE + pos), remain);
        ptr += remain;
        pos += remain;

        *ptr = hal_pbus_read_16bit(pos) & 0xFF;
    } else {
        memcpy(ptr, (void *)(PBUS_DATA_BASE + pos), remain);
    }

    return len;
}

int hal_pbus_write(u32 offset, u8 *buf, u32 len)
{
    u32 remain = len;
    u32 pos = offset;
    u8 *ptr = buf;
    u16 val = 0;

    if (!buf || !len)
        return -1;

    if (len + offset > AIC_PBUS_SIZE) {
        pr_err("Invalid data length: %d + %d\n", offset, len);
        return -1;
    }

    if (IS_UNALIGNED(offset)) {
        pos = ALIGN_DOWN(offset, 2);
        val = hal_pbus_read_16bit(pos) & 0xFF;
        val = val | (*ptr++ << 8);
        hal_pbus_write_16bit(pos, val);

        pos += 2;
        remain -= 1;
    }

    if (IS_UNALIGNED(remain)) {
        remain = ALIGN_DOWN(remain, 2);
        memcpy((void *)(PBUS_DATA_BASE + pos), ptr, remain);
        ptr += remain;
        pos += remain;

        val = hal_pbus_read_16bit(pos) >> 8;
        val = (val << 8) | *ptr;
        hal_pbus_write_16bit(pos, val);
    } else {
        memcpy((void *)(PBUS_DATA_BASE + pos), ptr, remain);
    }

    return len;
}
