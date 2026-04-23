/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xiaodong.zhao@artinchip.com
 */

#include <stdio.h>
#include <rtthread.h>
#include <finsh.h>

extern int test_csi_fft(void);
extern int test_csi_fir(void);
extern int test_csi_conv(void);

static int test_csi(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: test_csi <fft|fir|conv|all>\n");
        return -1;
    }

    if (rt_strcmp(argv[1], "fft") == 0) {
        return test_csi_fft();
    } else if (rt_strcmp(argv[1], "fir") == 0) {
        return test_csi_fir();
    } else if (rt_strcmp(argv[1], "conv") == 0) {
        return test_csi_conv();
    } else if (rt_strcmp(argv[1], "all") == 0) {
        test_csi_fft();
        test_csi_fir();
        test_csi_conv();
        return 0;
    } else {
        printf("Unknown test item: %s\n", argv[1]);
        return -1;
    }
}
MSH_CMD_EXPORT_ALIAS(test_csi, test_csi, "CSI DSP test suite");