/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <ota.h>
#include <env.h>
#include <absystem_os.h>

#define BUFFER_SIZE 256 //Need to be less than 2048 bytes

char buffer[BUFFER_SIZE];

int test_ota()
{
    FILE *file;
    int size;
    int ret;

    //update file ota.cpio is placed in the sdcard
    file = fopen("/sdcard/ota.cpio", "rb");
    if (file == NULL) {
        printf("Failed to open the file.\n");
        return -1;
    }

    //1.Buffer allocation required by OTA
    ret = ota_init();
    if (ret != RT_EOK) {
        printf("ota initialization failed.");
        goto __exit;
    }

    //2.Read BUFFER_SIZE each time and update it into flash
    while (!feof(file)) {
        size = fread(buffer, 1, BUFFER_SIZE, file);

        if (size > 0) {
            if(ota_shard_download_fun(buffer, size) < 0) {
                printf ("ota download failed.");
                goto __exit;
            }
        }
    }

    //3.Update the environment variables
    ret = aic_upgrade_end();
    if (ret) {
        printf("Aic upgrade end");
    }

    //4. Reset the device, Start new firmware
    extern void rt_hw_cpu_reset(void);
    rt_hw_cpu_reset();

__exit:
    fclose(file);

    ota_deinit();

    return 0;
}
MSH_CMD_EXPORT_ALIAS(test_ota, test_ota, Test OTA);

