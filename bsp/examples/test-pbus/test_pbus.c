/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */
#include <strings.h>
#include <finsh.h>

#include "aic_core.h"
#include "aic_utils.h"
#include "hal_pbus.h"

#define TEST_BUF_SIZE   128

static void usage(char *this)
{
    printf("Usage: %s [read/write] [offset] [len/data]\n", this);
    printf("Example: \n");
    printf("\t%s read 0x100 512\n", this);
    printf("\t%s write 0x100 0x1234\n", this);
}

static int test_pbus_read(u32 offset, u32 len)
{
    u8 buf[TEST_BUF_SIZE] = {0};
    u32 remain = len, size = 0;
    u32 i;

    if (offset + len > AIC_PBUS_SIZE) {
        pr_err("Reading 0x%#x + %d is out of PBUS range\n", offset, len);
        return -RT_EINVAL;
    }

    for (i = 0; i < (len / TEST_BUF_SIZE + 1); i++) {
        if (remain > TEST_BUF_SIZE)
            size = TEST_BUF_SIZE;
        else
            size = remain;

        if (hal_pbus_read(offset + TEST_BUF_SIZE * i, buf, size) < 0) {
            pr_err("Failed to read PBUS: [0x%#x] %d\n",
                   offset + TEST_BUF_SIZE * i, size);
            return -1;
        }
        hexdump(buf, size, 1);
    }

    return 0;
}

static int test_pbus_write(u32 offset, u32 data)
{
    u16 val = (u16)data;

    if (offset + 2 > AIC_PBUS_SIZE) {
        pr_err("Writing 0x%#x is out of PBUS range\n", offset);
        return -RT_EINVAL;
    }

    if (hal_pbus_write(offset, (u8 *)&val, 2) < 0) {
        pr_err("Failed to write PBUS: [0x%#x] 0x%x\n", offset, val);
        return -1;
    }

    return 0;
}

static rt_err_t cmd_test_pbus(int argc, char **argv)
{
    static bool inited = false;
    u32 offset = 0;
    u32 temp = 0;

    if (argc != 4) {
        usage(argv[0]);
        return -RT_EINVAL;
    }

    if (!inited) {
        hal_pbus_init();
        inited = true;
    }

    offset = strtol(argv[2], NULL, 16);
    if (offset > AIC_PBUS_SIZE) {
        pr_err("Offset 0x%#x is out of PBUS range\n", offset);
        return -RT_EINVAL;
    }

    temp = atoi(argv[3]);
    if (strncasecmp(argv[1], "read", strlen(argv[1])) == 0)
        return test_pbus_read(offset, temp);
    if (strncasecmp(argv[1], "write", strlen(argv[1])) == 0)
        return test_pbus_write(offset, temp);

    printf("Invalid operator: %s\n", argv[1]);
    return -RT_EINVAL;
}
MSH_CMD_EXPORT_ALIAS(cmd_test_pbus, test_pbus, test PBUS);
