/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */
#define DBG_ENABLE
#define DBG_SECTION_NAME "absystem"
#define DBG_COLOR
#include <rtconfig.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aic_core.h>
#include <env.h>
#include <absystem_os.h>
#include <dfs.h>
#include <dfs_fs.h>
#include <boot_param.h>

#define MOUNT_POINT_NAME_MAX 32

bool os_status = 0;
bool rodatafs_status = 0;
bool datafs_status = 0;

void aic_set_upgrade_status(char *file_name)
{
    if (strncmp("rodata.fatfs", file_name, 12) == 0) {
        rodatafs_status = 1;
    } else if (strncmp("data.fatfs", file_name, 10) == 0) {
        datafs_status = 1;
    } else if (strstr(file_name, ".itb") != NULL) {
        os_status = 1;
    } else {
        printf("file name %s not found!.\n", file_name);
    }
}

int aic_ota_status_update(void)
{
    char *status = NULL;
    int ret = 0;

    if (fw_env_open()) {
        pr_err("Open env failed\n");
        return -1;
    }

    status = fw_getenv("upgrade_available");
#ifdef AIC_ENV_DEBUG
    printf("upgrade_available = %s\n", status);
#endif
    if (strncmp(status, "1", 2) == 0) {

        ret = fw_env_write("upgrade_available", "0");
        if (ret) {
            pr_err("Env write fail\n");
            goto aic_get_upgrade_status_err;
        }

        fw_env_flush();
    } else if (strncmp(status, "0", 2) == 0) {
        ret = 0;
        goto aic_get_upgrade_status_err;
    } else {
        pr_err("Invalid upgrade_available\n");
        ret = -1;
        goto aic_get_upgrade_status_err;
    }

aic_get_upgrade_status_err:
    fw_env_close();
    return ret;
}

int aic_upgrade_end(void)
{
    char *now = NULL;
    int ret = 0;

    if (fw_env_open())
        return -1;

    /* update os */
    if (os_status) {
        now = fw_getenv("osAB_now");
        if (strncmp(now, "A", 1) == 0) {
            ret = fw_env_write("osAB_next", "B");
            LOG_I("os Next startup in B system");
        } else if (strncmp(now, "B", 1) == 0) {
            ret = fw_env_write("osAB_next", "A");
            LOG_I("os Next startup in A system");
        } else {
            LOG_E("invalid osAB_now");
            ret = -1;
        }

        if (ret)
            goto aic_upgrade_end_out;
    }
    /* update rodatafatfs */
    if (rodatafs_status) {
        now = fw_getenv("rodataAB_now");
        if (strncmp(now, "A", 1) == 0) {
            ret = fw_env_write("rodataAB_next", "B");
            LOG_I("rodata Next mount in B system");
        } else if (strncmp(now, "B", 1) == 0) {
            ret = fw_env_write("rodataAB_next", "A");
            LOG_I("rodata Next mount in A system");
        } else {
            LOG_E("invalid rodataAB_now");
            ret = -1;
        }

        if (ret)
            goto aic_upgrade_end_out;
    }
    /* update datafatfs */
    if (datafs_status) {
        now = fw_getenv("dataAB_now");
        if (strncmp(now, "A", 1) == 0) {
            ret = fw_env_write("dataAB_next", "B");
            LOG_I("data Next mount in B system");
        } else if (strncmp(now, "B", 1) == 0) {
            ret = fw_env_write("dataAB_next", "A");
            LOG_I("data Next mount in A system");
        } else {
            LOG_E("invalid dataAB_now");
            ret = -1;
        }

        if (ret)
            goto aic_upgrade_end_out;
    }
    ret = fw_env_write("bootcount", "0");
    if (ret)
        goto aic_upgrade_end_out;

    ret = fw_env_write("upgrade_available", "1");
    if (ret)
        goto aic_upgrade_end_out;

    /* flush to flash */
    fw_env_flush();

aic_upgrade_end_out:

    fw_env_close();

    return ret;
}

int aic_get_rodata_to_mount(char *target_rodata)
{
    char *now = NULL;
    char *rodata = NULL;
    int ret = 0;

    if (fw_env_open()) {
        pr_err("Open env failed\n");
        return -1;
    }

    now = fw_getenv("rodataAB_now");
#ifdef AIC_ENV_DEBUG
    printf("rodataAB_now = %s\n", now);
#endif
    if (strncmp(now, "A", 2) == 0) {
        rodata = fw_getenv("rodata_partname");
        if (rodata == NULL) {
            pr_err("failed to get rodata partname\n");
            return -1;
        }
        strncpy(target_rodata, rodata, MOUNT_POINT_NAME_MAX - 1);
    } else if (strncmp(now, "B", 2) == 0) {
        rodata = fw_getenv("rodata_partname_r");
        if (rodata == NULL) {
            pr_err("failed to get rodata partname\n");
            return -1;
        }
        strncpy(target_rodata, rodata, MOUNT_POINT_NAME_MAX - 1);
    } else {
        ret = -1;
        pr_err("invalid rodataAB_now\n");
    }

    fw_env_close();

    return ret;
}

int aic_get_data_to_mount(char *target_data)
{
    char *now = NULL;
    char *data = NULL;
    int ret = 0;

    if (fw_env_open()) {
        pr_err("Open env failed\n");
        return -1;
    }

    now = fw_getenv("dataAB_now");
#ifdef AIC_ENV_DEBUG
    printf("dataAB_now = %s\n", now);
#endif
    if (strncmp(now, "A", 2) == 0) {
        data = fw_getenv("data_partname");
        if (data == NULL) {
            pr_err("failed to get data partname\n");
            return -1;
        }
        strncpy(target_data, data, MOUNT_POINT_NAME_MAX - 1);
    } else if (strncmp(now, "B", 2) == 0) {
        data = fw_getenv("data_partname_r");
        if (data == NULL) {
            pr_err("failed to get data partname\n");
            return -1;
        }
        strncpy(target_data, data, MOUNT_POINT_NAME_MAX - 1);
    } else {
        ret = -1;
        pr_err("invalid dataAB_now\n");
    }

    fw_env_close();

    return ret;
}

#ifdef RT_USING_DFS_MNTTABLE
int aic_absystem_mount_fs(unsigned int prio)
{
    char target[MOUNT_POINT_NAME_MAX] = {0};
    enum boot_device boot_dev = aic_get_boot_device();

    if (boot_dev != BD_SDMC0) {
        aic_ota_status_update();

        if (prio == 0) {
            aic_get_rodata_to_mount(target);
            printf("Mount APP in blk %s\n", target);

            if (dfs_mount(target, "/rodata", "elm", 0, 0) < 0)
                printf("Failed to mount elm\n");

        } else {
            aic_get_data_to_mount(target);
            printf("Mount APP in blk %s\n", target);

            if (dfs_mount(target, "/data", "elm", 0, 0) < 0)
                printf("Failed to mount elm\n");
        }
    }

    return 0;
}

int aic_absystem_mount_fs_prio0(void)
{
    return aic_absystem_mount_fs(0);
}

int aic_absystem_mount_fs_prio1(void)
{
    return aic_absystem_mount_fs(1);
}

INIT_ENV_EXPORT(aic_absystem_mount_fs_prio0);
INIT_LATE_APP_EXPORT(aic_absystem_mount_fs_prio1);
#endif
