/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */
#include <finsh.h>
#include <strings.h>
#include <getopt.h>

#include "aic_common.h"
#include "aic_core.h"
#include "drv_camera.h"

struct camera_cmd {
    char *name;
    int (*handle_int)(rt_device_t dev, u32 arg);
    int (*handle_bool)(rt_device_t dev, bool arg);
};

static struct camera_cmd g_camera_cmds[] = {
    {"channel",    camera_set_channel},
    {"fps",        camera_set_fps},
    {"contrast",   camera_set_contrast},
    {"brightness", camera_set_brightness},
    {"saturation", camera_set_saturation},
    {"hue",        camera_set_hue},
    {"sharpness",  camera_set_sharpness},
    {"denoise",    camera_set_denoise},
    {"quality",    camera_set_quality},
    {"autogain",   camera_set_autogain},
    {"aec_val",    camera_set_aec_val},
    {"exposure",   camera_set_exposure},

    {"gain_ctrl", NULL, camera_set_gain_ctrl},
    {"whitebal",  NULL, camera_set_whitebal},
    {"awb",       NULL, camera_set_awb},
    {"aec2",      NULL, camera_set_aec2},
    {"dcw",       NULL, camera_set_dcw},
    {"bpc",       NULL, camera_set_bpc},
    {"wpc",       NULL, camera_set_wpc},
    {"h_flip",    NULL, camera_set_h_flip},
    {"v_flip",    NULL, camera_set_v_flip},
    {"colorbar",  NULL, camera_set_colorbar},
    {NULL, NULL},
};

static const char sopts[] = "c:v:lh";
static const struct option lopts[] = {
    {"command",  required_argument, NULL, 'c'},
    {"value",    required_argument, NULL, 'v'},
    {"list",           no_argument, NULL, 'l'},
    {"usage",          no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

static void usage(char *program)
{
    printf("Usage: %s [options]: \n", program);
    printf("\t -c, --command\t\tioctl command of Camera device\n");
    printf("\t -v, --value\t\tthe value of the command argument\n");
    printf("\t -l, --list\t\tList all the supported command\n");
    printf("\t -h, --usage \n");
    printf("\n");
    printf("Example: %s -f nv16 -c 1\n", program);
}

struct camera_cmd *camera_scan_cmd(char *arg)
{
    struct camera_cmd *cmd = g_camera_cmds;
    int i;

    for (i = 0; i < ARRAY_SIZE(g_camera_cmds); i++, cmd++) {
        if (cmd->name && !strncasecmp(cmd->name, arg, strlen(cmd->name)))
            return cmd;
    }
    printf("Invalid command: %s\n", arg);
    return NULL;
}

void camera_list_cmds(void)
{
    struct camera_cmd *cmd = g_camera_cmds;
    int i;

    printf("Supported Camera ioctl commands:\n");
    for (i = 0; i < ARRAY_SIZE(g_camera_cmds); i++, cmd++) {
        if (cmd->name)
            printf("%2d. %s\n", i, cmd->name);
    }
    printf("\n");
}

static rt_err_t cmd_test_camera(int argc, char **argv)
{
    struct camera_cmd *cmd = NULL;
    rt_device_t dev = NULL;
    u32 val = 0;
    int c, ret = 0;

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'c':
            cmd = camera_scan_cmd(optarg);
            break;

        case 'v':
            val = atoi(optarg);
            break;

        case 'l':
            camera_list_cmds();
            return 0;

        case 'h':
            usage(argv[0]);
            return 0;

        default:
            break;
        }
    }
    if (!cmd)
        return -RT_EINVAL;

    dev = rt_device_find(CAMERA_DEV_NAME);
    if (!dev) {
        pr_err("Failed to find camera device\n");
        return -RT_ERROR;
    }
    if (rt_device_open(dev, RT_DEVICE_FLAG_RDWR) < 0) {
        pr_err("Failed to open camera device\n");
        return -RT_EBUSY;
    }

    printf("Try to set %s %d\n", cmd->name, val);
    if (cmd->handle_int)
        ret = cmd->handle_int(dev, val);
    else if (cmd->handle_bool)
        ret = cmd->handle_bool(dev, (bool)val);

    if (ret)
        pr_err("Failed to set %s %ld, return %d\n", cmd->name, val, ret);

    rt_device_close(dev);
    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(cmd_test_camera, test_camera, test Camera);
