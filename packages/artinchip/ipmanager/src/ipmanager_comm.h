/*
 * Copyright (c) 2020-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: lv.wu <lv.wu@artinchip.com>
 */

#ifndef __IPMANAGER_COMM_H
#define __IPMANAGER_COMM_H

#include "ipmanager.h"
#include "ipconfig_file.h"
#include <stdbool.h>

typedef enum AICIP_COMM_CMD {
    AICIP_COMM_CMD_DHCP = 0,
    AICIP_COMM_CMD_IP,
}aicip_cmd_t;

typedef struct aicip_comm {
    aicip_cmd_t cmd;
    char name[4];
    aic_config_file_t config;
}aicip_comm_t;

extern int aicip_comm_init(int size);
extern int aicip_comm_dhcpc_enable(char *name, bool enable);
extern int aicip_comm_set_ip(char *name, aicip_config_t *ip4_config);
extern int aicip_comm_pend_1s(void *buff);

#endif
