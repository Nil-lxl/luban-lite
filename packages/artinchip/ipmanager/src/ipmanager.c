/*
 * Copyright (c) 2020-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: lv.wu <lv.wu@artinchip.com>
 */
#include "stdio.h"
#include "string.h"

#include "ipmanager.h"
#include "ipconfig_file.h"
#include "aic_log.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "aic_osal.h"
#include "aic_errno.h"
#include "ipmanager_comm.h"

static const char *aicmac_name[] = {
#if defined(AIC_USING_GMAC0) || (!defined(AIC_USING_GMAC0) && defined(AIC_USING_GMAC1))
	"ai0",
#endif
#if defined(AIC_USING_GMAC0) && defined(AIC_USING_GMAC1)
	"ai1"
#endif
};

static int aic_ipmanager_dhcp(const char *name, bool enable)
{
    struct netif *netif;

    netif = netif_find(name);
    if (netif == NULL) {
        pr_err("Can't find interface name %s\n", name);
        return -1;
    }

    if (enable)
        dhcp_start(netif);
    else
        dhcp_stop(netif);

    aicfile_set_dhcp_only(name, enable);

    return 0;
}

static int aic_ipmanager_set_ip(const char *name, aicip_config_t *config)
{
    struct netif *netif;
    ip4_addr_t ip;
    ip4_addr_t gw;
    ip4_addr_t netmask;
    aic_config_file_t file_config = {0};

    aic_ipmanager_dhcp(name, false);

    netif = netif_find(name);
    if (netif == NULL) {
        pr_err("Can't find interface name %s\n", name);
        return -1;
    }

    if (!(ip4addr_aton(config->ip4_addr, &ip) && \
          ip4addr_aton(config->gw_addr, &gw) && \
          ip4addr_aton(config->netmask, &netmask))) {
        pr_err("Error ip information\n\tip:%s, gw:%s, netmask: %s\n",
               config->ip4_addr, config->gw_addr, config->netmask);
        return -1;
    }

    netif_set_down(netif);

    netif_set_gw(netif, &gw);
    netif_set_netmask(netif, &netmask);
    netif_set_ipaddr(netif, &ip);

    netif_set_up(netif);

    file_config.dhcp4_enable = false;
    memcpy(&file_config.aicip_config, config, sizeof(aicip_config_t));

    aicfile_set_ipconfig(name, &file_config);

    return 0;
}

static void aic_ipmanager_thread(void *para)
{
    aicip_comm_t comm;
    int ret;

    // todo: check dhcp client status?
    while (1) {
        memset(&comm, 0, sizeof(comm));
        ret = aicip_comm_pend_1s(&comm);
        if (!ret) {
        	pr_err("queue error, must be checked!\n");
        	continue;
        }

        if (comm.cmd == AICIP_COMM_CMD_DHCP) {
            aic_ipmanager_dhcp(comm.name, comm.config.dhcp4_enable);
            aicfile_set_dhcp_only(comm.name, comm.config.dhcp4_enable);
        } else {
            aic_ipmanager_set_ip(comm.name, &(comm.config.aicip_config));
            aicfile_set_ipconfig(comm.name, &(comm.config));
        }
    }
}

static int aic_ipmanager_start(void)
{
    aic_config_file_t config;
    int ret = 0;

    for (int i = 0; i < sizeof(aicmac_name) / sizeof(aicmac_name[0]); i++) {
        ret = aicfile_get_ipconfig(aicmac_name[i], &config);
        if (ret != 0) {
            pr_err("Can't get ip configuration\n");
            return -1;
        }

        if (config.dhcp4_enable)
            aic_ipmanager_dhcp(aicmac_name[i], true);
        else
            aic_ipmanager_set_ip(aicmac_name[i], &(config.aicip_config));
    }

    aicip_comm_init(10);

    aicos_thread_create("aic ipmanager", 4096, 15, aic_ipmanager_thread, NULL);

    return 0;
}

int ipmanager_daemon_start(void)
{
    return aic_ipmanager_start();
}

int ipmanager_dhcp(char *name, bool enable)
{
    return aicip_comm_dhcpc_enable(name, enable);
}

int ipmanager_change_ip(char *name, aicip_config_t *ip4_config)
{
    return aicip_comm_set_ip(name, ip4_config);
}

