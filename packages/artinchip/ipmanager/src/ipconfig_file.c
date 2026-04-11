/*
 * Copyright (c) 2020-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: lv.wu <lv.wu@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "ipmanager.h"
#include "ipconfig_file.h"
#include "cJSON.h"
#include "aic_log.h"
#include <dfs_posix.h>
#include "rtconfig.h"
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

#define MAX_FILE_SIZE   512
#define DEFAULT_SET_FILE_PATH   AIC_IPMANAGER_SAVE_FILE

#define ITEM_DHCP       "dhcp4_enable"
#define ITEM_IP4        "ip4_addr"
#define ITEM_GW         "gw_addr"
#define ITEM_NETMASK    "netmask"

//#define GET_DEFAULT_NETCFG(name, config)
//        do {
//            strncpy(config->name, name, 3);
//            config->dhcp4_enable = true;
//            strncpy(config->aicip_config.ip4_addr, AIC_DEV_GMAC0_IPADDR, 15);
//            strncpy(config->aicip_config.gw_addr, AIC_DEV_GMAC0_GW, 15);
//            strncpy(config->aicip_config.netmask, AIC_DEV_GMAC0_NETMASK, 15);
//        }while(0)

static const char *ipconfig_item[] = {
    ITEM_DHCP,
    ITEM_IP4,
    ITEM_GW,
    ITEM_NETMASK
};

static void get_ipconfig_default(const char *name, aic_config_file_t *config)
{
#if defined(AIC_USING_GMAC0)
	if (!memcmp(name, "ai0", 3)) {
	    strncpy(config->name, name, 3);
	    config->dhcp4_enable = true;
	    strncpy(config->aicip_config.ip4_addr, AIC_DEV_GMAC0_IPADDR, 15);
	    strncpy(config->aicip_config.gw_addr, AIC_DEV_GMAC0_GW, 15);
	    strncpy(config->aicip_config.netmask, AIC_DEV_GMAC0_NETMASK, 15);
	}
#elif (!defined(AIC_USING_GMAC0) && defined(AIC_USING_GMAC1))
	if (!memcmp(name, "ai0", 3)) {
	    strncpy(config->name, name, 3);
	    config->dhcp4_enable = true;
	    strncpy(config->aicip_config.ip4_addr, AIC_DEV_GMAC1_IPADDR, 15);
	    strncpy(config->aicip_config.gw_addr, AIC_DEV_GMAC1_GW, 15);
	    strncpy(config->aicip_config.netmask, AIC_DEV_GMAC1_NETMASK, 15);
	}
#endif

#if defined(AIC_USING_GMAC0) && defined(AIC_USING_GMAC1)
	if (!memcmp(name, "ai1", 3)) {
	    strncpy(config->name, name, 3);
	    config->dhcp4_enable = true;
	    strncpy(config->aicip_config.ip4_addr, AIC_DEV_GMAC1_IPADDR, 15);
	    strncpy(config->aicip_config.gw_addr, AIC_DEV_GMAC1_GW, 15);
	    strncpy(config->aicip_config.netmask, AIC_DEV_GMAC1_NETMASK, 15);
	}
#endif
}

static int get_json_root(const char *file, cJSON **root)
{
    int fd;
    struct stat buf;
    bool rs = -1;
    const char* filepath = file;
    int read_size;

    fd = open(filepath, O_RDWR);
    if (fd < 0) {
        pr_info("can't find file %s\n", file);
        return -1;
    }

    fstat(fd, &buf);
    if (buf.st_size < MAX_FILE_SIZE)
        read_size = buf.st_size;
    else
        read_size = MAX_FILE_SIZE;

    char* data = malloc(read_size);
    if(data)
    {
        int size = read(fd, data, read_size);
        data[size] = '\0';
        if(size > 0)
        {
            *root = cJSON_Parse(data);
            rs = 0;
        }
        free(data);
    }
    close(fd);
    return rs;
}

static int save_ipcfg_to_file(const char *filename, cJSON *root)
{
    int fd;
    const char *filePath = filename;
    fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0)
    {
        char *jsonString = cJSON_Print(root);
        write(fd, jsonString, strlen(jsonString));
        close(fd);
        cJSON_free(jsonString);
        return 0;
    }
    else
        pr_err("open file for write failed %s\n", filename);

    return -1;
}

int aicfile_get_ipconfig(const char *name, aic_config_file_t *config)
{
    cJSON *root = NULL;
    cJSON *item_ipmanager = NULL;

    if (!name || !config) {
        pr_err("arg error\n");
        return -1;
    }

    get_json_root(DEFAULT_SET_FILE_PATH, &root);

    /* Can't get ipconfig from DEFAULT_SET_FILE_PATH, then get it from default */
    if (root == NULL) {
        pr_info("Can't get ipconfig file[%s], get ipconfig from menuconfig\n",
                 DEFAULT_SET_FILE_PATH);
        get_ipconfig_default(name, config);
        return 0;
    }

    item_ipmanager = cJSON_GetObjectItem(root, name);
    if ((item_ipmanager == NULL) || !cJSON_IsObject(item_ipmanager)) {
        goto get_default;
    }

    for (int i = 0; i < sizeof(ipconfig_item) / sizeof(ipconfig_item[0]); i++) {
        if (!cJSON_HasObjectItem(item_ipmanager, ipconfig_item[i]))
            goto get_default;
    }

    cJSON *item_ipcfg = NULL;
    char *str = NULL;

    item_ipcfg = cJSON_GetObjectItem(item_ipmanager, ITEM_DHCP);
    if (!cJSON_IsBool(item_ipcfg))
        goto get_default;
    config->dhcp4_enable = cJSON_IsTrue(item_ipcfg);

    item_ipcfg = cJSON_GetObjectItem(item_ipmanager, ITEM_IP4);
    if (!cJSON_IsString(item_ipcfg))
        goto get_default;
    str = cJSON_GetStringValue(item_ipcfg);

    strncpy(config->aicip_config.ip4_addr, str, 15);
    item_ipcfg = cJSON_GetObjectItem(item_ipmanager, ITEM_GW);
    if (!cJSON_IsString(item_ipcfg))
        goto get_default;
    str = cJSON_GetStringValue(item_ipcfg);
    strncpy(config->aicip_config.gw_addr, str, 15);

    item_ipcfg = cJSON_GetObjectItem(item_ipmanager, ITEM_NETMASK);
    if (!cJSON_IsString(item_ipcfg))
        goto get_default;
    str = cJSON_GetStringValue(item_ipcfg);
    strncpy(config->aicip_config.netmask, str, 15);

    cJSON_Delete(root);

    pr_info("Get ipconfig named %s from file[%s] SUCCESS\n", name, DEFAULT_SET_FILE_PATH);
    return 0;

get_default:

    pr_info("Can't get ipconfig named %s, get ipconfig from menuconfig\n", name);
    get_ipconfig_default(name, config);
    cJSON_Delete(root);

    return 0;
}

int aicfile_set_ipconfig(const char *name, aic_config_file_t *config)
{
    cJSON *root = NULL;
    cJSON *item_ipmanager = NULL;

    if (!name || !config) {
        pr_err("arg error\n");
        return -1;
    }

    get_json_root(DEFAULT_SET_FILE_PATH, &root);
    if (root == NULL) {
        root = cJSON_CreateObject();
        if (root == NULL) {
            pr_err("Can't creat JSON object\n");
            return -1;
        }
    }

    item_ipmanager = cJSON_GetObjectItem(root, name);
    if (item_ipmanager == NULL)
        item_ipmanager = cJSON_AddObjectToObject(root, name);

    if (item_ipmanager == NULL) {
        pr_err("Can't creat JSON object\n");
        cJSON_Delete(root);
        return -1;
    }

    for (int i = 0; i < sizeof(ipconfig_item) / sizeof(ipconfig_item[0]); i++) {
        if (cJSON_HasObjectItem(item_ipmanager, ipconfig_item[i]))
            cJSON_DeleteItemFromObject(item_ipmanager, ipconfig_item[i]);
    }

    cJSON_AddFalseToObject(item_ipmanager, ITEM_DHCP);
    cJSON_AddStringToObject(item_ipmanager, ITEM_IP4, config->aicip_config.ip4_addr);
    cJSON_AddStringToObject(item_ipmanager, ITEM_GW, config->aicip_config.gw_addr);
    cJSON_AddStringToObject(item_ipmanager, ITEM_NETMASK, config->aicip_config.netmask);

    save_ipcfg_to_file(DEFAULT_SET_FILE_PATH, root);
    cJSON_Delete(root);

    return 0;
}

int aicfile_set_dhcp_only(const char *name, uint8_t enable)
{
    cJSON *root = NULL;
    cJSON *item_ipmanager = NULL;

    if (!name) {
        pr_err("arg error\n");
        return -1;
    }

    get_json_root(DEFAULT_SET_FILE_PATH, &root);
    if (root == NULL) {
        root = cJSON_CreateObject();
        if (root == NULL) {
            pr_err("Can't creat JSON object\n");
            return -1;
        }
    }

    item_ipmanager = cJSON_GetObjectItem(root, name);
    if (item_ipmanager == NULL)
        item_ipmanager = cJSON_AddObjectToObject(root, name);

    if (item_ipmanager == NULL) {
        pr_err("Can't creat JSON object\n");
        cJSON_Delete(root);
        return -1;
    }

    if (cJSON_HasObjectItem(item_ipmanager, ITEM_DHCP))
        cJSON_DeleteItemFromObject(item_ipmanager, ITEM_DHCP);

    if (enable)
        cJSON_AddTrueToObject(item_ipmanager, ITEM_DHCP);
    else
        cJSON_AddFalseToObject(item_ipmanager, ITEM_DHCP);

    save_ipcfg_to_file(DEFAULT_SET_FILE_PATH, root);
    cJSON_Delete(root);

    return 0;
}
