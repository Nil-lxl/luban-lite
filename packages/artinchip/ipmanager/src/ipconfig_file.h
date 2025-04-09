/*
 * Copyright (c) 2020-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: lv.wu <lv.wu@artinchip.com>
 */
#ifndef __IPCONFIG_FILE_H
#define __IPCONFIG_FILE_H

#include "stdio.h"
#include "ipmanager.h"

/*
/{
    ai0 : {
        dhcp4_enable: true,
        ip4_addr: "192.168.3.20",
        gw_addr: "192.168.3.1",
        netmask: "255.255.255.0"
    },
    ai1 : {
        dhcp4_enable: true,
        ip4_addr: "192.168.3.20",
        gw_addr: "192.168.3.1",
        netmask: "255.255.255.0"
    }
}
*/

typedef struct aic_config_file {
    char name[4];
    uint8_t dhcp4_enable;
    uint8_t dhcp6_enable;
    aicip_config_t aicip_config;
}aic_config_file_t;

extern int aicfile_get_ipconfig(const char *name, aic_config_file_t *config);

extern int aicfile_set_ipconfig(const char *name, aic_config_file_t *config);

extern int aicfile_set_dhcp_only(const char *name, uint8_t enable);

#endif
