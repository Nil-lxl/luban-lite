/*
 * Copyright (c) 2020-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: lv.wu <lv.wu@artinchip.com>
 */
#ifndef __IPMANAGER_H
#define __IPMANAGER_H

#include <stdbool.h>

typedef struct aicip_config {
    char ip4_addr[16];
    char gw_addr[16];
    char netmask[16];
}aicip_config_t;

extern int ipmanager_daemon_start(void);
extern int ipmanager_dhcp(char *name, bool enable);
extern int ipmanager_change_ip(char *name, aicip_config_t *ip4_config);

#endif
