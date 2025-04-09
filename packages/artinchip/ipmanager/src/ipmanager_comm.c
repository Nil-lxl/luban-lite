/*
 * Copyright (c) 2020-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: lv.wu <lv.wu@artinchip.com>
 */

#include "ipmanager_comm.h"

#include "aic_errno.h"
#include "aic_osal.h"
#include "aic_log.h"
#include <string.h>

static aicos_queue_t aicip_comm_mbox;

int aicip_comm_init(int size)
{
    aicip_comm_mbox = aicos_queue_create(sizeof(aicip_comm_t), size);
    if (aicip_comm_mbox == NULL) {
        pr_err("Can't creat queue for ip manager\n");
        return -EACCES;
    }

    return 0;
}

int aicip_comm_dhcpc_enable(char *name, bool enable)
{
    aicip_comm_t comm = {0};

    strncpy(comm.name, name, 3);
    comm.config.dhcp4_enable = enable;
    comm.cmd = AICIP_COMM_CMD_DHCP;

    return aicos_queue_send(aicip_comm_mbox, &comm);
}

int aicip_comm_set_ip(char *name, aicip_config_t *ip4_config)
{
    aicip_comm_t comm = {0};

    strncpy(comm.name, name, 3);
    comm.config.dhcp4_enable = false;
    memcpy(&comm.config.aicip_config, ip4_config, sizeof(*ip4_config));
    comm.cmd = AICIP_COMM_CMD_IP;

    return aicos_queue_send(aicip_comm_mbox, &comm);
}

int aicip_comm_pend_1s(void *buff)
{
    return aicos_queue_receive(aicip_comm_mbox, buff, AICOS_WAIT_FOREVER);
}

// ipmanager ai0 -d enable/disable
// ipmanager ai0 192.168.3.20 192.168.3.1 255.255.255.0
static int ipmanager(int argc, char *argv[])
{
	if (argc != 4 && argc != 5)
		goto usage;

	if ((!memcmp(argv[2], "-d", 2)) && (argc == 4)) {
		if (!memcmp(argv[3], "start", 5))
			aicip_comm_dhcpc_enable(argv[1], 1);
		else
			aicip_comm_dhcpc_enable(argv[1], 0);
		return 0;
	}

	if (argc == 5) {
		aicip_config_t config;
		strncpy(config.ip4_addr, argv[2], 15);
		strncpy(config.gw_addr, argv[3], 15);
		strncpy(config.netmask, argv[4], 15);
		aicip_comm_set_ip(argv[1], &config);

		return 0;
	}

usage:
	printf("ipmanager [name] -d enable/disable\t-- enable/disable dhcp client\n");
	printf("ipmanager [name] [ip] [gw] [netmask]\t-- set ip address\n");

	return -1;

}
MSH_CMD_EXPORT(ipmanager, artinchip ipmanager test cmd);
