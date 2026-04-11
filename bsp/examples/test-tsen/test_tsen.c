/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <fcntl.h>

#define DBG_TAG  "tsen.cmd"

#include "aic_core.h"
#include "aic_log.h"
#include "sensor.h"
#include "hal_tsen.h"
#include <unistd.h>
#include <time.h>

/* Global macro and variables */
#define THREAD_PRIORITY                 25
#define THREAD_STACK_SIZE               4096
#define THREAD_TIMESLICE                50
#define AIC_CONFIG_FOLDER_PERMISSION    0755
#define AIC_CONFIG_PATH                 "/data/config"
#define AIC_TSEN_TEMP_PATH             "/data/config/tsen_temp_records"

static rt_sensor_t g_sensor;
static rt_device_t g_tsen_dev = RT_NULL;
static rt_thread_t  g_tsen_thread_polling = RT_NULL;
static rt_thread_t  g_tsen_thread_recording = RT_NULL;

/* Common configuration for various modes */
struct test_tsen_mod_cfg {
    int sensor_id;
    int mode_id;
    int samp_cnt;
    int polling_time;
};

/* Special configuration for recording modes */
struct test_tsen_record_cfg {
    int record_time;
    int record_data_size;
    int temp_diff_th;
};

enum test_tsen_mod {
    TEST_TSEN_MODE_READ = 0,
    TEST_TSEN_MODE_RECORD,
    TEST_TSEN_MODE_GET_RECORD
};

static struct test_tsen_mod_cfg g_modcfg = {0, -1, 10, 10};
static struct test_tsen_record_cfg g_recfg = {100, 0, 20};

static const char sopts[] = "m:s:n:t:r:d:h";
static const struct option lopts[] = {
    {"mode", required_argument,                  NULL, 'm'},
    {"sensor_id", required_argument,             NULL, 's'},
    {"samples_num", required_argument,           NULL, 'n'},
    {"polling_interval", required_argument,      NULL, 't'},
    {"record_time", required_argument,           NULL, 'r'},
    {"temp_diff_threshold", required_argument,   NULL, 'd'},
    {"help", no_argument,                        NULL, 'h'},
    {0, 0, 0, 0}
    };

static void cmd_tsen_usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -m, --mode_id\t\t\tSelect and start the reading/recording mode\n");
    printf("\t -s, --sensor_id\t\tSelect the sensor, default is 0\n");
    printf("\t -n, --samples_number\t\tSet the samples number, default is 10\n");
    printf("\t -t, --polling_interval\t\tSet the polling interval (ms), default is 10ms\n");
    printf("\t -r, --recording_time\t\tSet the recording time (ms), default is 100ms\n");
    printf("\t -d, --temp_diff_threshold\tSet the temperature diff threshold (0.1℃ ), default is 20(2℃ )\n");
    printf("\t -h, --help \n");
    printf("\n");

    printf("Example: \n");
    printf("\t %s -n 5 -t 10 -d 2 -r 20\n", program);
    printf("\t %s -s 0 -m 1\n", program);
    printf("\t %s -m 2\n", program);
    printf("Mode ID:\n");
    printf("\t[0] read temperature\n");
    printf("\t[1] record temperature\n");
    printf("\t[2] get recording temperature\n");
    printf("Sensor ID:\n");
    printf("\t[0] sensor_cpu\n");
    printf("\t[1] sensor_gpai\n");
}


static void test_tsen_single_mode(void) {
    int num = 0;
    struct rt_sensor_data data;

    for (num = 0; num < g_modcfg.samp_cnt; num++) {
        if (rt_device_read(g_tsen_dev, 0, &data, 1) == 1)
            rt_kprintf("[%d] temp:%3d.%d C, timestamp:%5d\n",
                  num, data.data.temp / 10,(rt_uint32_t)data.data.temp % 10,
                  data.timestamp);
        else
            rt_kprintf("read data failed!");
    }
    rt_device_close(g_tsen_dev);

}

static void test_tsen_polling_mode(void *parameter)
{
    int num = 0;
    struct rt_sensor_data data;
    int sampling_cnt = 0;

    while (1) {
        for (num = 0; num < g_modcfg.samp_cnt; num++) {
            if (rt_device_read(g_tsen_dev, 0, &data, 1) == 1) {
                rt_kprintf("[%d] temp:%3d.%d C, timestamp:%5d\n", num,
                           data.data.temp / 10,
                           (rt_uint32_t)data.data.temp % 10, data.timestamp);
                sampling_cnt++;
            }
            else
                rt_kprintf("read data failed!");
        }
        rt_thread_mdelay(g_modcfg.polling_time);
    }

}

static void test_tsen_start_polling(void)
{
    g_tsen_thread_polling = rt_thread_create("tsen_polling",
                                             test_tsen_polling_mode, RT_NULL,
                                             THREAD_STACK_SIZE, THREAD_PRIORITY,
                                            THREAD_TIMESLICE);
    if (g_tsen_thread_polling != RT_NULL)
        rt_thread_startup(g_tsen_thread_polling);

    return;
}

static int test_tsen_get_record_temp(void)
{
    int fd;
    int line_cnt =0;
    int y_play;
    int temp_buf[g_recfg.record_data_size];

    memset(temp_buf, 0, sizeof(int) * g_recfg.record_data_size);

    fd = open(AIC_TSEN_TEMP_PATH, O_RDONLY);
    if (fd < 0)
        return -1;

    read(fd, temp_buf, sizeof(int) * g_recfg.record_data_size);

    while(line_cnt < g_recfg.record_data_size) {
        y_play = temp_buf[line_cnt];
        rt_kprintf("[%d] temp:%3d.%d C\n", line_cnt, y_play / 10,
                   (rt_uint32_t)y_play % 10);
        line_cnt++;
    }
    close(fd);
    return 0;
}

static void test_tsen_record_mode(void *parameter)
{
    int fd;
    int num = 0;
    struct rt_sensor_data data;
    int sampling_cnt = 0;
    int temp = 0;
    int cur_temp = 0;
    int last_temp = 0;
    int abs_temp = 0;
    int temp_buf_size = 0;
    int samp_cnt = g_modcfg.samp_cnt;
    int record_cnt = 0;

    int polling_cnt = g_recfg.record_time / g_modcfg.polling_time;
    if (samp_cnt <= 0)
        return;

    if (open(AIC_CONFIG_PATH, O_RDONLY) < 0)
        mkdir(AIC_CONFIG_PATH, AIC_CONFIG_FOLDER_PERMISSION);
    fd = open(AIC_TSEN_TEMP_PATH, O_CREAT | O_WRONLY);
    if (fd > 0) {
        temp_buf_size = samp_cnt * polling_cnt;
        int temp_buf[temp_buf_size];
        memset(temp_buf, 0, sizeof(int) * temp_buf_size);

        while (polling_cnt) {
            polling_cnt--;
            for (num = 0; num < samp_cnt; num++) {
                if(num == 0)
                    rt_kprintf("=========[%d]=========\n",
                                g_recfg.record_time / g_modcfg.polling_time -
                                polling_cnt);

                if (rt_device_read(g_tsen_dev, 0, &data, 1) == 1) {
                    rt_kprintf("[%d] temp:%3d.%d C, timestamp:%5d\n",
                               sampling_cnt, data.data.temp / 10,
                               (rt_uint32_t)data.data.temp % 10,
                               data.timestamp);
                    temp += data.data.temp;
                    sampling_cnt++;
                } else {
                    rt_kprintf("read data failed!");
                }
            }
            cur_temp = temp / samp_cnt;
            abs_temp = abs(cur_temp - last_temp);
            if (abs_temp > g_recfg.temp_diff_th) {
                memcpy(temp_buf + record_cnt, &cur_temp, sizeof(int));
                record_cnt++;
            }

            last_temp = cur_temp;
            rt_thread_mdelay(g_modcfg.polling_time);
            temp = 0;
        }
        write(fd, temp_buf, sizeof(int) * record_cnt);
        g_recfg.record_data_size = record_cnt;
        rt_kprintf("total record count:%d\n", g_recfg.record_data_size);
        close(fd);
        rt_device_close(g_tsen_dev);
        rt_thread_delete(g_tsen_thread_recording);
    } else {
        rt_kprintf("open file failed!\n");
    }
}

static void test_tsen_start_recording(void)
{
    g_tsen_thread_recording = rt_thread_create("tsen_recording",
                                               test_tsen_record_mode, RT_NULL,
                                               THREAD_STACK_SIZE,
                                               THREAD_PRIORITY,
                                               THREAD_TIMESLICE);
    if (g_tsen_thread_recording != RT_NULL)
        rt_thread_startup(g_tsen_thread_recording);

    return;
}

static char* test_tsen_sensor_selected(int sensor_id)
{
    switch (sensor_id) {
    case AIC_TSEN_SENSOR_CPU:
        return "temp_tsen_cpu";
    case AIC_TSEN_SENSOR_GPAI:
        return "temp_tsen_gpai";
    default:
        return NULL;
    }

}

static int cmd_test_tsen(int argc, char **argv)
{
    int c, ret = -1;
    struct aic_tsen_ch tsen_info;
    char *aic_tsen_name = NULL;

    if (argc < 2) {
        cmd_tsen_usage(argv[0]);
        goto __exit;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 's':
            g_modcfg.sensor_id = atoi(optarg);
            continue;
        case 'n':
            g_modcfg.samp_cnt = atoi(optarg);
            continue;
        case 't':
            g_modcfg.polling_time = atoi(optarg);
            continue;
        case 'r':
            g_recfg.record_time = atoi(optarg);
            continue;
        case 'd':
            g_recfg.temp_diff_th = atoi(optarg);
            continue;
        case 'm':
            g_modcfg.mode_id = atoi(optarg);
            continue;
        case 'h':
            cmd_tsen_usage(argv[0]);
            goto __exit;
        default:
            break;
        }
    }

    if (g_modcfg.mode_id < 0)
        goto __exit;

    g_sensor = (rt_sensor_t)g_tsen_dev;
    aic_tsen_name = test_tsen_sensor_selected(g_modcfg.sensor_id);

    if (aic_tsen_name == NULL) {
        rt_kprintf("The sensor_id is error\n");
        goto __exit;
    }

    g_tsen_dev = rt_device_find(aic_tsen_name);
    if (g_tsen_dev == RT_NULL) {
        rt_kprintf("Failed to find %s device\n", aic_tsen_name);
        goto __exit;
    }

    ret = rt_device_open(g_tsen_dev, RT_DEVICE_FLAG_RDONLY);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to open %s device\n", aic_tsen_name);
        goto __exit;
    }

    rt_device_control(g_tsen_dev, RT_SENSOR_CTRL_GET_CH_INFO, &tsen_info);

    switch (g_modcfg.mode_id) {
        case TEST_TSEN_MODE_READ:
            if (tsen_info.soft_mode == AIC_TSEN_SOFT_MODE_SINGLE) {
                rt_kprintf("[%s] Starting single mode\n", aic_tsen_name);
                test_tsen_single_mode();
            } else if (tsen_info.soft_mode == AIC_TSEN_SOFT_MODE_POLLING) {
                rt_kprintf("[%s] Starting polling mode\n", aic_tsen_name);
                test_tsen_start_polling();
            } else {
                rt_kprintf("Unknown read mode %d\n", tsen_info.mode);
                goto __exit;
            }
            break;
        case TEST_TSEN_MODE_RECORD:
            if (tsen_info.soft_mode != AIC_TSEN_SOFT_MODE_POLLING) {
                rt_kprintf("Recording way needs select polling mode\n");
                goto __exit;
            }
            test_tsen_start_recording();
            break;
        case TEST_TSEN_MODE_GET_RECORD:
            test_tsen_get_record_temp();
            rt_device_close(g_tsen_dev);
            break;
        default:
            rt_device_close(g_tsen_dev);
            rt_kprintf("Unknown mode %d\n", g_modcfg.mode_id);
            goto __exit;
    }
    g_modcfg.mode_id = -1;
    ret = 0;

__exit:
    return ret;

}

MSH_CMD_EXPORT_ALIAS(cmd_test_tsen, test_tsen, tsen device sample);
