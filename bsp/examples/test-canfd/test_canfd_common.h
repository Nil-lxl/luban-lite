/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: deqiang.lin <deqiang.lin@artinchip.com>
 */
#ifndef __TEST_CANFD_COMMON_H__
#define __TEST_CANFD_COMMON_H__

#include <stdint.h>
#include <rtthread.h>

#define CANFD_DEV_NAME_LEN  16

struct canfd_dev_info;

typedef int (*canfd_rx_cb)(struct canfd_dev_info *dev_info,
                           struct rt_can_msg *msg_buf, int msg_cnt);

struct canfd_dev_info {
    char dev_name[CANFD_DEV_NAME_LEN];
    rt_device_t dev;
    struct rt_semaphore rx_sem;
    int rx_inited;
    canfd_rx_cb rx_cb;
};

struct canfd_dev_info* canfd_get_dev_info(const char *dev_name);
int canfd_setup_rx(struct canfd_dev_info *dev_info,
                   struct rt_can_filter_item *items,
                   int filter_cnt, canfd_rx_cb rx_cb);
int canfd_setup(struct canfd_dev_info *dev_info, u32 baudrate,
                u32 dbaudrate, int mode);

#endif
